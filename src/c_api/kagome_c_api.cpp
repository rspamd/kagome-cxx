#include "kagome/c_api/kagome_c_api.h"
#include "kagome/tokenizer/tokenizer.hpp"
#include "kagome/tokenizer/token.hpp"
#include "kagome/dict/dict.hpp"

#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <filesystem>
#include <dlfcn.h>
#include <unicode/utf8.h>
#include <unicode/ustring.h>
#include <unicode/uscript.h>

namespace {
// Global tokenizer instance
std::unique_ptr<kagome::tokenizer::Tokenizer> g_tokenizer;

// Helper function to get the directory of the current shared library
std::string get_library_directory()
{
	Dl_info dl_info;
	if (dladdr(reinterpret_cast<void *>(kagome_init), &dl_info) != 0 && dl_info.dli_fname != nullptr) {
		std::filesystem::path lib_path(dl_info.dli_fname);
		return lib_path.parent_path().string();
	}
	return "";
}

// Helper function to detect Japanese characters
bool contains_japanese_chars(const char *text, size_t len)
{
	const char *pos = text;
	const char *end = text + len;

	while (pos < end) {
		UChar32 ch;
		int32_t offset = 0;
		U8_NEXT(pos, offset, end - pos, ch);
		if (offset <= 0) break;
		pos += offset;

		// Check for Hiragana, Katakana, or Kanji scripts
		UErrorCode error = U_ZERO_ERROR;
		UScriptCode script = uscript_getScript(ch, &error);
		if (U_SUCCESS(error)) {
			if (script == USCRIPT_HIRAGANA ||
				script == USCRIPT_KATAKANA ||
				script == USCRIPT_HAN) {
				return true;
			}
		}
	}
	return false;
}

// Helper function to allocate C strings safely
char *strdup_safe(const std::string &str)
{
	if (str.empty()) {
		char *result = static_cast<char *>(malloc(1));
		if (result) result[0] = '\0';
		return result;
	}

	char *result = static_cast<char *>(malloc(str.length() + 1));
	if (result) {
		std::memcpy(result, str.c_str(), str.length() + 1);
	}
	return result;
}

// Helper function to convert UTF-8 to UTF-32
std::vector<uint32_t> utf8_to_utf32(const std::string &utf8_str)
{
	std::vector<uint32_t> result;
	const char *pos = utf8_str.c_str();
	const char *end = pos + utf8_str.length();

	while (pos < end) {
		UChar32 ch;
		int32_t offset = 0;
		U8_NEXT(pos, offset, end - pos, ch);
		if (offset <= 0) break;
		pos += offset;
		if (ch >= 0) {
			result.push_back(static_cast<uint32_t>(ch));
		}
	}
	return result;
}
}// namespace

extern "C" {

int kagome_init(const ucl_object_t * /* config */, char *error_buf, size_t error_buf_size)
{
	try {
		// For now, ignore config and use default IPA dictionary
		// TODO: In future, rspamd can pass preprocessed config as environment variables
		// or through a different mechanism
		kagome::tokenizer::DictType dict_type = kagome::tokenizer::DictType::IPA;

		std::unique_ptr<kagome::dict::Dict> dictionary;

		// Search for dictionary in various locations
		{
			// Fallback to searching common paths
			std::vector<std::string> potential_paths;

			// Get library directory for relative paths
			std::string lib_dir = get_library_directory();

			if (dict_type == kagome::tokenizer::DictType::IPA) {
				potential_paths = {
					"data/ipa/ipa.dict",
					"../data/ipa/ipa.dict",
					"../../data/ipa/ipa.dict",
					"/usr/local/share/kagome/ipa.dict",
					"/usr/share/kagome/ipa.dict",
					"/opt/kagome/ipa.dict"};

				// Add library-relative paths
				if (!lib_dir.empty()) {
					potential_paths.insert(potential_paths.begin(), {lib_dir + "/ipa.dict",
																	 lib_dir + "/data/ipa/ipa.dict"});
				}
			}
			else if (dict_type == kagome::tokenizer::DictType::UniDic) {
				potential_paths = {
					"data/uni/uni.dict",
					"../data/uni/uni.dict",
					"../../data/uni/uni.dict",
					"/usr/local/share/kagome/uni.dict",
					"/usr/share/kagome/uni.dict",
					"/opt/kagome/uni.dict"};

				// Add library-relative paths
				if (!lib_dir.empty()) {
					potential_paths.insert(potential_paths.begin(), {lib_dir + "/uni.dict",
																	 lib_dir + "/data/uni/uni.dict"});
				}
			}

			// Try to load from paths with better error handling
			std::string last_error;
			for (const auto &path: potential_paths) {
				if (std::filesystem::exists(path)) {
					try {
						// Add extra safety check for file size
						auto file_size = std::filesystem::file_size(path);
						if (file_size == 0 || file_size > 500 * 1024 * 1024) {// Max 500MB
							last_error = "Dictionary file size invalid: " + std::to_string(file_size);
							continue;
						}

						dictionary = kagome::dict::DictLoader::load_from_zip(path, true);
						if (dictionary) {
							break;
						}
					} catch (const std::exception &e) {
						last_error = std::string("Failed to load ") + path + ": " + e.what();
						// Continue trying other paths
						continue;
					} catch (...) {
						last_error = std::string("Unknown error loading ") + path;
						continue;
					}
				}
			}

			// If all dictionary loading failed, try to create a minimal fallback
			if (!dictionary) {
				try {
					dictionary = kagome::dict::DictLoader::create_fallback_dict();
					if (dictionary) {
						if (error_buf && error_buf_size > 0) {
							std::strncpy(error_buf, "Warning: Using fallback dictionary. "
													"For full functionality, place ipa.dict next to the library.",
										 error_buf_size - 1);
							error_buf[error_buf_size - 1] = '\0';
						}
						// Don't return error, continue with fallback
					}
				} catch (const std::exception &e) {
					if (error_buf && error_buf_size > 0) {
						std::snprintf(error_buf, error_buf_size,
									  "Could not load any dictionary. Last error: %s",
									  last_error.empty() ? "Unknown" : last_error.c_str());
					}
					return -1;
				}
			}

			if (!dictionary) {
				if (error_buf && error_buf_size > 0) {
					std::snprintf(error_buf, error_buf_size,
								  "Could not create fallback dictionary. Last error: %s",
								  last_error.empty() ? "Unknown" : last_error.c_str());
				}
				return -1;
			}
		}

		// Create tokenizer with loaded dictionary
		kagome::tokenizer::TokenizerConfig tokenizer_config{};
		tokenizer_config.default_mode = kagome::tokenizer::TokenizeMode::Normal;

		g_tokenizer = std::make_unique<kagome::tokenizer::Tokenizer>(std::move(dictionary), tokenizer_config);

		if (!g_tokenizer) {
			if (error_buf && error_buf_size > 0) {
				std::strncpy(error_buf, "Failed to create kagome tokenizer", error_buf_size - 1);
				error_buf[error_buf_size - 1] = '\0';
			}
			return -1;
		}

		return 0;
	} catch (const std::exception &e) {
		if (error_buf && error_buf_size > 0) {
			std::snprintf(error_buf, error_buf_size, "Exception in kagome_init: %s", e.what());
		}
		return -1;
	} catch (...) {
		if (error_buf && error_buf_size > 0) {
			std::strncpy(error_buf, "Unknown exception in kagome_init", error_buf_size - 1);
			error_buf[error_buf_size - 1] = '\0';
		}
		return -1;
	}
}

void kagome_deinit(void)
{
	g_tokenizer.reset();
}

double kagome_detect_language(const char *text, size_t len)
{
	if (!text || len == 0) {
		return -1.0;
	}

	// Simple Japanese detection based on character scripts
	if (contains_japanese_chars(text, len)) {
		// Calculate confidence based on Japanese character density
		const char *pos = text;
		const char *end = text + len;
		size_t total_chars = 0;
		size_t japanese_chars = 0;

		while (pos < end) {
			UChar32 ch;
			int32_t offset = 0;
			U8_NEXT(pos, offset, end - pos, ch);
			if (offset <= 0) break;
			pos += offset;
			total_chars++;

			UErrorCode error = U_ZERO_ERROR;
			UScriptCode script = uscript_getScript(ch, &error);
			if (U_SUCCESS(error)) {
				if (script == USCRIPT_HIRAGANA ||
					script == USCRIPT_KATAKANA ||
					script == USCRIPT_HAN) {
					japanese_chars++;
				}
			}
		}

		if (total_chars > 0) {
			double ratio = static_cast<double>(japanese_chars) / total_chars;
			// Return confidence between 0.3 and 0.95 based on Japanese character ratio
			return std::max(0.3, std::min(0.95, 0.3 + ratio * 0.65));
		}
	}

	return -1.0;// Cannot handle non-Japanese text
}

int kagome_tokenize(const char *text, size_t len, rspamd_words_t *result)
{
	if (!text || len == 0 || !result || !g_tokenizer) {
		return -1;
	}

	try {
		std::string input(text, len);
		auto tokens = g_tokenizer->tokenize(input);

		// Pre-process to find valid tokens that exist in original text
		std::vector<std::pair<size_t, const kagome::tokenizer::Token *>> valid_tokens;

		for (const auto &token: tokens) {
			const std::string &surface = token.surface();

			// Skip empty tokens (BOS/EOS markers)
			if (surface.empty()) {
				continue;
			}

			// Additional safety: skip tokens with invalid surface length
			if (surface.length() == 0 || surface.length() > len) {
				continue;
			}

			// Use the token's actual start position from the tokenizer
			std::int32_t raw_start = token.start();

			// Safety check: ensure token position is non-negative and reasonable
			if (raw_start < 0 || static_cast<size_t>(raw_start) >= len) {
				// Token position is invalid, try fallback search
				bool found = false;
				if (len >= surface.length()) {
					for (size_t pos = 0; pos <= len - surface.length(); pos++) {
						if (std::memcmp(text + pos, surface.c_str(), surface.length()) == 0) {
							// Verify this is a proper UTF-8 boundary
							if (pos == 0 || !U8_IS_TRAIL(text[pos])) {
								valid_tokens.push_back({pos, &token});
								found = true;
								break;
							}
						}
					}
				}
				if (!found) {
					continue;
				}
			}
			else {
				size_t token_start = static_cast<size_t>(raw_start);

				// Verify the token position is valid and the surface matches
				if (token_start < len &&
					token_start + surface.length() <= len &&
					std::memcmp(text + token_start, surface.c_str(), surface.length()) == 0) {
					// Verify this is a proper UTF-8 boundary
					if (token_start == 0 || !U8_IS_TRAIL(text[token_start])) {
						valid_tokens.push_back({token_start, &token});
						continue;
					}
				}

				// If position validation fails, try fallback search
				bool found = false;
				if (len >= surface.length()) {
					for (size_t pos = 0; pos <= len - surface.length(); pos++) {
						if (std::memcmp(text + pos, surface.c_str(), surface.length()) == 0) {
							// Verify this is a proper boundary
							if (pos == 0 || !U8_IS_TRAIL(text[pos])) {
								valid_tokens.push_back({pos, &token});
								found = true;
								break;
							}
						}
					}
				}
				// Only skip if we absolutely cannot find the token
				if (!found) {
					continue;
				}
			}
		}

		// Allocate array for valid tokens only
		if (valid_tokens.empty()) {
			result->a = nullptr;
			result->n = 0;
			result->m = 0;
			return 0;
		}

		result->a = static_cast<rspamd_word_t *>(calloc(valid_tokens.size(), sizeof(rspamd_word_t)));
		if (!result->a) {
			return -1;
		}

		result->n = 0;
		result->m = valid_tokens.size();

		// Process only valid tokens
		for (const auto &[pos, token_ptr]: valid_tokens) {
			// Additional safety checks
			if (!token_ptr || pos >= len) {
				continue;
			}

			rspamd_word_t &word = result->a[result->n];
			const std::string &surface = token_ptr->surface();

			// Safety check: ensure we don't go beyond buffer bounds
			if (pos + surface.length() > len) {
				continue;
			}

			// CRITICAL: Always point to original text buffer
			word.original.begin = text + pos;
			word.original.len = surface.length();
			word.flags = RSPAMD_WORD_FLAG_TEXT | RSPAMD_WORD_FLAG_UTF | RSPAMD_WORD_FLAG_NORMALISED;

			// Get base form once to avoid multiple string copies
			std::string base_form;
			try {
				base_form = token_ptr->base_form();
			} catch (...) {
				// If base_form() throws, use surface as fallback
				base_form = surface;
			}

			const std::string *normalized_source;

			// Use base form if available and meaningful, otherwise use surface
			if (!base_form.empty() && base_form != "*") {
				normalized_source = &base_form;
			}
			else {
				normalized_source = &surface;
			}

			// Japanese Part-of-Speech filtering and classification
			// This determines how rspamd should treat different types of morphemes
			std::vector<std::string> pos_vec;
			try {
				pos_vec = token_ptr->pos();
			} catch (...) {
				// If pos() throws, continue with empty vector
				pos_vec.clear();
			}

			bool is_punctuation = false;
			bool is_particle_or_auxiliary [[maybe_unused]] = false;

			if (!pos_vec.empty()) {
				const std::string &main_pos = pos_vec[0];

				// 記号 = symbols/punctuation (。、！？etc.)
				// These should be marked as exceptions to skip them in statistical analysis
				if (main_pos == "記号") {
					is_punctuation = true;
					word.flags |= RSPAMD_WORD_FLAG_EXCEPTION;
				}
				// 助詞 = particles (は、が、を、に、etc.) - grammatical but less semantic value
				// 助動詞 = auxiliary verbs (だ、である、ます、etc.) - grammatical function
				// These are stop words - they carry grammatical info but less semantic weight
				else if (main_pos == "助詞" || main_pos == "助動詞") {
					is_particle_or_auxiliary = true;
					word.flags |= RSPAMD_WORD_FLAG_STOP_WORD;
				}
				// TODO: Consider also marking very common words like それ、これ、あれ as stop words
			}

			// Convert to UTF-32 for unicode field (only if not punctuation to save memory)
			if (!is_punctuation) {
				auto utf32_chars = utf8_to_utf32(surface);
				if (!utf32_chars.empty()) {
					uint32_t *unicode_copy = static_cast<uint32_t *>(malloc(utf32_chars.size() * sizeof(uint32_t)));
					if (unicode_copy) {
						std::memcpy(unicode_copy, utf32_chars.data(), utf32_chars.size() * sizeof(uint32_t));
						word.unicode.begin = unicode_copy;
						word.unicode.len = utf32_chars.size();
					}
				}
			}

			// Allocate normalized and stemmed forms (single allocation each)
			char *normalized_copy = strdup_safe(*normalized_source);
			if (normalized_copy) {
				word.normalized.begin = normalized_copy;
				word.normalized.len = normalized_source->length();

				// For Japanese, stemmed form is the same as normalized (no further stemming needed)
				char *stemmed_copy = strdup_safe(*normalized_source);
				if (stemmed_copy) {
					word.stemmed.begin = stemmed_copy;
					word.stemmed.len = normalized_source->length();
				}
			}

			result->n++;
		}

		return 0;
	} catch (const std::exception &e) {
		if (result->a) {
			kagome_cleanup_result(result);
		}
		return -1;
	}
}

void kagome_cleanup_result(rspamd_words_t *result)
{
	if (!result || !result->a) {
		return;
	}

	for (size_t i = 0; i < result->n; i++) {
		rspamd_word_t &word = result->a[i];

		// NEVER free original.begin - it always points to the original text buffer
		// This was the source of the segfault!

		if (word.unicode.begin) {
			free(const_cast<uint32_t *>(word.unicode.begin));
			word.unicode.begin = nullptr;
		}
		if (word.normalized.begin) {
			free(const_cast<char *>(word.normalized.begin));
			word.normalized.begin = nullptr;
		}
		if (word.stemmed.begin) {
			free(const_cast<char *>(word.stemmed.begin));
			word.stemmed.begin = nullptr;
		}
	}

	free(result->a);
	result->a = nullptr;
	result->n = 0;
	result->m = 0;
}

const char *kagome_get_language_hint(void)
{
	return "ja";
}

double kagome_get_min_confidence(void)
{
	return 0.3;// Minimum confidence for Japanese detection
}

}// extern "C"