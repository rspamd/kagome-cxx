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
#include <unicode/utf8.h>
#include <unicode/ustring.h>
#include <unicode/uscript.h>

namespace {
    // Global tokenizer instance
    std::unique_ptr<kagome::tokenizer::Tokenizer> g_tokenizer;
    
    // Helper function to detect Japanese characters
    bool contains_japanese_chars(const char* text, size_t len) {
        const char* pos = text;
        const char* end = text + len;
        
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
    char* strdup_safe(const std::string& str) {
        if (str.empty()) {
            char* result = static_cast<char*>(malloc(1));
            if (result) result[0] = '\0';
            return result;
        }
        
        char* result = static_cast<char*>(malloc(str.length() + 1));
        if (result) {
            std::memcpy(result, str.c_str(), str.length() + 1);
        }
        return result;
    }
    
    // Helper function to convert UTF-8 to UTF-32
    std::vector<uint32_t> utf8_to_utf32(const std::string& utf8_str) {
        std::vector<uint32_t> result;
        const char* pos = utf8_str.c_str();
        const char* end = pos + utf8_str.length();
        
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
}

extern "C" {

int kagome_init(const ucl_object_t *config __attribute__((unused)), char *error_buf, size_t error_buf_size) {
    try {
        // Try to create tokenizer with IPA dictionary
        g_tokenizer = kagome::tokenizer::factory::create_tokenizer(
            kagome::tokenizer::TokenizerType::Normal,
            kagome::tokenizer::DictType::IPA
        );
        
        if (!g_tokenizer) {
            if (error_buf && error_buf_size > 0) {
                std::strncpy(error_buf, "Failed to create kagome tokenizer", error_buf_size - 1);
                error_buf[error_buf_size - 1] = '\0';
            }
            return -1;
        }
        
        return 0;
    } catch (const std::exception& e) {
        if (error_buf && error_buf_size > 0) {
            std::strncpy(error_buf, e.what(), error_buf_size - 1);
            error_buf[error_buf_size - 1] = '\0';
        }
        return -1;
    }
}

void kagome_deinit(void) {
    g_tokenizer.reset();
}

double kagome_detect_language(const char *text, size_t len) {
    if (!text || len == 0) {
        return -1.0;
    }
    
    // Simple Japanese detection based on character scripts
    if (contains_japanese_chars(text, len)) {
        // Calculate confidence based on Japanese character density
        const char* pos = text;
        const char* end = text + len;
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
    
    return -1.0; // Cannot handle non-Japanese text
}

int kagome_tokenize(const char *text, size_t len, rspamd_words_t *result) {
    if (!text || len == 0 || !result || !g_tokenizer) {
        return -1;
    }
    
    try {
        std::string input(text, len);
        auto tokens = g_tokenizer->tokenize(input);
        
        // Allocate array for words
        result->a = static_cast<rspamd_word_t*>(calloc(tokens.size(), sizeof(rspamd_word_t)));
        if (!result->a) {
            return -1;
        }
        
        result->n = 0;
        result->m = tokens.size();
        
        for (const auto& token : tokens) {
            // Skip tokens with empty surface (including dummy BOS/EOS tokens)
            if (token.surface().empty()) {
                continue;
            }
            
            rspamd_word_t& word = result->a[result->n];
            
            // Set original surface form
            char* surface_copy = strdup_safe(token.surface());
            if (!surface_copy) {
                kagome_cleanup_result(result);
                return -1;
            }
            word.original.begin = surface_copy;
            word.original.len = token.surface().length();
            
            // Convert to UTF-32 for unicode field
            auto utf32_chars = utf8_to_utf32(token.surface());
            if (!utf32_chars.empty()) {
                uint32_t* unicode_copy = static_cast<uint32_t*>(malloc(utf32_chars.size() * sizeof(uint32_t)));
                if (unicode_copy) {
                    std::memcpy(unicode_copy, utf32_chars.data(), utf32_chars.size() * sizeof(uint32_t));
                    word.unicode.begin = unicode_copy;
                    word.unicode.len = utf32_chars.size();
                }
            }
            
            // Set normalized form (use base form if available, otherwise surface)
            std::string normalized = token.base_form();
            if (normalized.empty() || normalized == "*") {
                normalized = token.surface();
            }
            char* normalized_copy = strdup_safe(normalized);
            if (normalized_copy) {
                word.normalized.begin = normalized_copy;
                word.normalized.len = normalized.length();
            }
            
            // Set stemmed form (same as normalized for Japanese)
            char* stemmed_copy = strdup_safe(normalized);
            if (stemmed_copy) {
                word.stemmed.begin = stemmed_copy;
                word.stemmed.len = normalized.length();
            }
            
            // Set flags
            word.flags = RSPAMD_WORD_FLAG_TEXT | RSPAMD_WORD_FLAG_UTF | RSPAMD_WORD_FLAG_NORMALISED;
            
            // Check if it's a stop word (very basic heuristic)
            auto pos_vec = token.pos();
            if (!pos_vec.empty()) {
                const std::string& pos = pos_vec[0];
                if (pos == "助詞" || pos == "助動詞" || pos == "記号") {
                    word.flags |= RSPAMD_WORD_FLAG_STOP_WORD;
                }
            }
            
            result->n++;
        }
        
        return 0;
    } catch (const std::exception& e) {
        if (result->a) {
            kagome_cleanup_result(result);
        }
        return -1;
    }
}

void kagome_cleanup_result(rspamd_words_t *result) {
    if (!result || !result->a) {
        return;
    }
    
    for (size_t i = 0; i < result->n; i++) {
        rspamd_word_t& word = result->a[i];
        
        // Free allocated strings
        if (word.original.begin) {
            free(const_cast<char*>(word.original.begin));
            word.original.begin = nullptr;
        }
        if (word.unicode.begin) {
            free(const_cast<uint32_t*>(word.unicode.begin));
            word.unicode.begin = nullptr;
        }
        if (word.normalized.begin) {
            free(const_cast<char*>(word.normalized.begin));
            word.normalized.begin = nullptr;
        }
        if (word.stemmed.begin) {
            free(const_cast<char*>(word.stemmed.begin));
            word.stemmed.begin = nullptr;
        }
    }
    
    free(result->a);
    result->a = nullptr;
    result->n = 0;
    result->m = 0;
}

const char *kagome_get_language_hint(void) {
    return "ja";
}

double kagome_get_min_confidence(void) {
    return 0.3; // Minimum confidence for Japanese detection
}

} // extern "C" 