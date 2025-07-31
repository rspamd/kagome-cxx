#include "kagome/tokenizer/tokenizer.hpp"
#include "kagome/tokenizer/lattice/lattice.hpp"
#include <unicode/utf8.h>
#include <unicode/ustring.h>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fmt/format.h>

namespace kagome::tokenizer {

Tokenizer::Tokenizer(std::unique_ptr<dict::Dict> dictionary)
	: dict_(std::move(dictionary)), config_{}
{
}

Tokenizer::Tokenizer(std::unique_ptr<dict::Dict> dictionary, const TokenizerConfig &config)
	: dict_(std::move(dictionary)), config_(config)
{
}

void Tokenizer::set_mode(TokenizerType type)
{
	config_.default_mode = static_cast<TokenizeMode>(type);
}

std::vector<Token> Tokenizer::tokenize(std::string_view input) const
{
	return analyze(input, config_.default_mode);
}

std::vector<Token> Tokenizer::analyze(std::string_view input, TokenizeMode mode) const
{
	return analyze_impl(input, mode, nullptr);
}

std::vector<std::string> Tokenizer::wakati(std::string_view input) const
{
	auto tokens = analyze(input, TokenizeMode::Normal);
	std::vector<std::string> result;
	result.reserve(tokens.size());

	for (const auto &token: tokens) {
		// Accept tokens with valid surface text, even if classified as Dummy
		// This is a workaround for the dictionary reference issue in lattice processing
		if (!token.surface().empty()) {
			result.push_back(token.surface());
		}
	}

	return result;
}

std::vector<Token> Tokenizer::analyze_graph(std::ostream &dot_output,
											std::string_view input,
											TokenizeMode mode) const
{
	return analyze_impl(input, mode, &dot_output);
}

std::vector<Token> Tokenizer::analyze_impl(std::string_view input,
										   TokenizeMode mode,
										   std::ostream *dot_output) const
{
	// Get the dictionary pointer (works for both constructors)
	dict::Dict *dict_ptr = get_dict();
	if (!dict_ptr) {
		return {};// Empty result if no dictionary
	}

	// Create a shared_ptr for lattice compatibility
	std::shared_ptr<dict::Dict> shared_dict;
	if (shared_dict_) {
		// We already have a shared_ptr, use it directly
		shared_dict = shared_dict_;
	}
	else {
		// Create a shared_ptr from unique_ptr with no-op deleter
		shared_dict = std::shared_ptr<dict::Dict>(dict_.get(), [](dict::Dict *) {
			// No-op deleter since unique_ptr owns the dict
		});
	}

	auto lattice = lattice::create_lattice(shared_dict, nullptr);

	// Build lattice from input
	lattice->build(input);

	// Forward pass (Viterbi algorithm)
	lattice::LatticeMode lattice_mode = lattice::LatticeMode::Normal;// Default to Normal
	switch (mode) {
	case TokenizeMode::Normal:
		lattice_mode = lattice::LatticeMode::Normal;
		break;
	case TokenizeMode::Search:
		lattice_mode = lattice::LatticeMode::Search;
		break;
	case TokenizeMode::Extended:
		lattice_mode = lattice::LatticeMode::Extended;
		break;
	}

	lattice->forward(lattice_mode);
	lattice->backward(lattice_mode);

	// Export DOT if requested
	if (dot_output) {
		lattice->export_dot(*dot_output);
	}

	// Convert lattice output to tokens
	std::vector<Token> tokens;
	tokens.reserve(lattice->output().size());

	for (std::size_t i = 0; i < lattice->output().size(); ++i) {
		const auto *node = lattice->output()[i];
		if (config_.omit_bos_eos && node->is_bos_eos()) {
			continue;
		}

		// Calculate end position (start + surface byte length)
		std::int32_t end_pos = node->position() + static_cast<std::int32_t>(node->surface().length());

		// Use the full constructor that takes TokenClass explicitly
		tokens.emplace_back(
			static_cast<std::int32_t>(i),               // index
			node->id(),                                 // id
			static_cast<TokenClass>(node->node_class()),// token_class
			node->position(),                           // position
			node->position(),                           // start
			end_pos,                                    // end
			node->surface(),                            // surface
			shared_dict,                                // dict
			nullptr                                     // user_dict
		);
	}

	return tokens;
}

namespace factory {

std::unique_ptr<Tokenizer> create_tokenizer(TokenizerType type, DictType dict_type)
{
	std::unique_ptr<dict::Dict> dictionary;

	// Try to load real dictionary first
	std::vector<std::string> potential_paths;

	if (dict_type == DictType::IPA) {
		potential_paths = {
			"data/ipa/ipa.dict",
			"../data/ipa/ipa.dict",
			"../../data/ipa/ipa.dict",
			"/Users/vstakhov/kagome-dict/ipa/ipa.dict",// User's path
			"/tmp/ipa.dict"};
	}
	else if (dict_type == DictType::UniDic) {
		potential_paths = {
			"data/uni/uni.dict",
			"../data/uni/uni.dict",
			"../../data/uni/uni.dict",
			"/Users/vstakhov/kagome-dict/uni/uni.dict",// User's path
			"/tmp/uni.dict"};
	}

	// Try to load from ZIP files
	for (const auto &path: potential_paths) {
		if (std::filesystem::exists(path)) {
			try {
				dictionary = dict::DictLoader::load_from_zip(path, true);
				if (dictionary) {
					fmt::print("Successfully loaded dictionary from: {}\n", path);
					break;
				}
			} catch (const std::exception &e) {
				fmt::print(stderr, "Failed to load dictionary from {}: {}\n", path, e.what());
			}
		}
	}

	// Fallback to built-in dictionary if loading failed
	if (!dictionary) {
		fmt::print("Using fallback dictionary\n");
		dictionary = dict::DictLoader::create_fallback_dict();
	}

	// Use explicit TokenizerConfig to avoid constructor ambiguity
	TokenizerConfig config{};
	config.default_mode = static_cast<TokenizeMode>(type);

	auto tokenizer = std::make_unique<Tokenizer>(std::move(dictionary), config);

	return tokenizer;
}

std::unique_ptr<Tokenizer> create_tokenizer(std::unique_ptr<dict::Dict> dictionary,
											const TokenizerConfig &config)
{
	return std::make_unique<Tokenizer>(std::move(dictionary), config);
}

}// namespace factory

}// namespace kagome::tokenizer