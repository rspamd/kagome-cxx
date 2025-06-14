#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <format>
#include <memory>

#include "kagome/dict/dict.hpp"

namespace kagome::tokenizer {

/// Token classification types
enum class TokenClass : std::uint8_t {
    /// Dummy token (BOS/EOS)
    Dummy = 0,
    /// Known word in system dictionary
    Known = 1,
    /// Unknown word 
    Unknown = 2,
    /// User dictionary word
    User = 3
};

/// Convert TokenClass to string representation
constexpr std::string_view to_string(TokenClass cls) noexcept {
    switch (cls) {
        case TokenClass::Dummy: return "DUMMY";
        case TokenClass::Known: return "KNOWN";
        case TokenClass::Unknown: return "UNKNOWN";
        case TokenClass::User: return "USER";
        default: return "INVALID";
    }
}

/// Extra data for user dictionary tokens
struct UserExtra {
    std::vector<std::string> tokens;
    std::vector<std::string> readings;
    
    UserExtra() = default;
    UserExtra(std::vector<std::string> t, std::vector<std::string> r)
        : tokens(std::move(t)), readings(std::move(r)) {}
};

/// Complete token data for JSON serialization
struct TokenData {
    std::int32_t id;
    std::int32_t start;
    std::int32_t end;
    std::string surface;
    std::string token_class;
    std::vector<std::string> pos;
    std::string base_form;
    std::string reading;
    std::string pronunciation;
    std::vector<std::string> features;
};

/// Main token class representing a morphological unit
class Token {
public:
    /// Default constructor
    Token() = default;
    
    /// Construct a token with all required fields
    Token(std::int32_t index, std::int32_t id, TokenClass token_class,
          std::int32_t position, std::int32_t start, std::int32_t end,
          std::string surface, std::shared_ptr<dict::Dict> dict,
          std::shared_ptr<dict::UserDict> user_dict = nullptr);
    
    /// Simplified constructor from lattice node data
    Token(std::string surface, std::int32_t id, std::int32_t start,
          std::shared_ptr<dict::Dict> dict,
          std::shared_ptr<dict::UserDict> user_dict = nullptr);
    
    /// Copy and move constructors/assignment
    Token(const Token&) = default;
    Token& operator=(const Token&) = default;
    Token(Token&&) = default;
    Token& operator=(Token&&) = default;
    
    ~Token() = default;
    
    // Accessors
    [[nodiscard]] std::int32_t index() const noexcept { return index_; }
    [[nodiscard]] std::int32_t id() const noexcept { return id_; }
    [[nodiscard]] TokenClass token_class() const noexcept { return class_; }
    [[nodiscard]] std::int32_t position() const noexcept { return position_; }
    [[nodiscard]] std::int32_t start() const noexcept { return start_; }
    [[nodiscard]] std::int32_t end() const noexcept { return end_; }
    [[nodiscard]] const std::string& surface() const noexcept { return surface_; }
    
    /// Get all morphological features
    [[nodiscard]] std::vector<std::string> features() const;
    
    /// Get feature at specific index
    [[nodiscard]] std::optional<std::string> feature_at(std::size_t index) const;
    
    /// Get POS (parts of speech) tags
    [[nodiscard]] std::vector<std::string> pos() const;
    
    /// Extract inflectional type feature
    [[nodiscard]] std::string inflectional_type() const;
    
    /// Extract inflectional form feature  
    [[nodiscard]] std::string inflectional_form() const;
    
    /// Extract base form feature
    [[nodiscard]] std::string base_form() const;
    
    /// Extract reading (yomi) feature
    [[nodiscard]] std::string reading() const;
    
    /// Extract pronunciation feature
    [[nodiscard]] std::string pronunciation() const;
    
    /// Get user dictionary extra data (only for user tokens)
    [[nodiscard]] std::optional<UserExtra> user_extra() const;
    
    /// Check if tokens have equal features
    [[nodiscard]] bool equal_features(const Token& other) const;
    
    /// Check if tokens have equal POS
    [[nodiscard]] bool equal_pos(const Token& other) const;
    
    /// Convert to complete token data for serialization
    [[nodiscard]] TokenData to_token_data() const;
    
    /// String representation
    [[nodiscard]] std::string to_string() const;
    
    /// Equality comparison
    [[nodiscard]] bool operator==(const Token& other) const noexcept;
    [[nodiscard]] bool operator!=(const Token& other) const noexcept;

private:
    std::int32_t index_ = 0;
    std::int32_t id_ = 0;
    TokenClass class_ = TokenClass::Dummy;
    std::int32_t position_ = 0;
    std::int32_t start_ = 0;
    std::int32_t end_ = 0;
    std::string surface_;
    std::shared_ptr<dict::Dict> dict_;
    std::shared_ptr<dict::UserDict> user_dict_;
    
    /// Helper to get feature by dictionary key
    [[nodiscard]] std::optional<std::string> 
    pickup_from_features(std::string_view key) const;
};

/// Utility functions
namespace utils {

/// Check if two feature vectors are equal
[[nodiscard]] bool equal_features(const std::vector<std::string>& lhs,
                                 const std::vector<std::string>& rhs) noexcept;

} // namespace utils

} // namespace kagome::tokenizer

/// Support for std::format
template<>
struct std::formatter<kagome::tokenizer::Token> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }
    
    auto format(const kagome::tokenizer::Token& token, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}:{} ({}: {}, {}) {} [{}]",
                             token.index(), token.surface(), token.position(),
                             token.start(), token.end(), 
                             to_string(token.token_class()), token.id());
    }
};

template<>
struct std::formatter<kagome::tokenizer::TokenClass> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }
    
    auto format(kagome::tokenizer::TokenClass cls, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", to_string(cls));
    }
}; 