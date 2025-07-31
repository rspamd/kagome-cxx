#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <concepts>

#include "kagome/tokenizer/token.hpp"
#include "kagome/dict/dict.hpp"

namespace kagome::tokenizer {

/// Tokenization modes for different use cases
enum class TokenizeMode : std::uint8_t {
	/// Regular segmentation
	Normal = 1,
	/// Use heuristic for additional segmentation useful for search
	Search = 2,
	/// Similar to search mode, but also unigram unknown words
	Extended = 3
};

/// Tokenizer types (alias for modes)
enum class TokenizerType : std::uint8_t {
	Normal = static_cast<std::uint8_t>(TokenizeMode::Normal),
	Search = static_cast<std::uint8_t>(TokenizeMode::Search),
	Extended = static_cast<std::uint8_t>(TokenizeMode::Extended)
};

/// Dictionary types
enum class DictType : std::uint8_t {
	IPA = 1,
	UniDic = 2
};

/// Configuration options for the tokenizer
struct TokenizerConfig {
	/// Whether to omit BOS/EOS tokens from output
	bool omit_bos_eos = false;
	/// Default tokenization mode
	TokenizeMode default_mode = TokenizeMode::Normal;
};

/// Forward declarations
class Lattice;

/// Main tokenizer interface for Japanese morphological analysis
class Tokenizer {
public:
	/// Create a new tokenizer with the given dictionary
	explicit Tokenizer(std::unique_ptr<dict::Dict> dictionary);

	/// Create a new tokenizer with the given dictionary and configuration
	Tokenizer(std::unique_ptr<dict::Dict> dictionary,
			  const TokenizerConfig &config = {});

	/// Construct tokenizer with shared dictionary and config (for compatibility)
	Tokenizer(std::shared_ptr<dict::Dict> dictionary,
			  const TokenizerConfig &config = {})
		: dict_(nullptr), user_dict_(nullptr), shared_dict_(std::move(dictionary)), config_(config)
	{
		// Keep the shared_ptr and use raw pointer for internal access
	}

	~Tokenizer() = default;

	// Non-copyable but movable
	Tokenizer(const Tokenizer &) = delete;
	Tokenizer &operator=(const Tokenizer &) = delete;
	Tokenizer(Tokenizer &&) = default;
	Tokenizer &operator=(Tokenizer &&) = default;

	/// Set the tokenization mode
	void set_mode(TokenizerType type);

	/// Tokenize input text using the default mode
	[[nodiscard]] std::vector<Token> tokenize(std::string_view input) const;

	/// Tokenize input text using the specified mode
	[[nodiscard]] std::vector<Token> analyze(std::string_view input, TokenizeMode mode) const;

	/// Wakati tokenization - returns only surface strings
	[[nodiscard]] std::vector<std::string> wakati(std::string_view input) const;

	/// Export lattice graph in DOT format for debugging
	[[nodiscard]] std::vector<Token> analyze_graph(std::ostream &dot_output,
												   std::string_view input,
												   TokenizeMode mode) const;

private:
	std::unique_ptr<dict::Dict> dict_;
	std::shared_ptr<dict::UserDict> user_dict_;
	std::shared_ptr<dict::Dict> shared_dict_;// For shared_ptr compatibility
	TokenizerConfig config_;

	/// Internal tokenization implementation
	std::vector<Token> analyze_impl(std::string_view input, TokenizeMode mode,
									std::ostream *dot_output = nullptr) const;

	/// Get the dictionary pointer (works with both unique_ptr and shared_ptr constructors)
	dict::Dict *get_dict() const
	{
		return dict_ ? dict_.get() : shared_dict_.get();
	}
};

/// Factory functions for creating tokenizers
namespace factory {

/// Create a tokenizer with specified type and dictionary
[[nodiscard]] std::unique_ptr<Tokenizer> create_tokenizer(TokenizerType type, DictType dict_type);

/// Create a tokenizer with system dictionary only
[[nodiscard]] std::unique_ptr<Tokenizer>
create_tokenizer(std::unique_ptr<dict::Dict> dictionary,
				 const TokenizerConfig &config = {});

}// namespace factory

}// namespace kagome::tokenizer