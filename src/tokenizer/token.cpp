#include "kagome/tokenizer/token.hpp"
#include <unicode/ustring.h>
#include <unicode/utf8.h>
#include "kagome/common/format.hpp"
#include <algorithm>

namespace kagome::tokenizer {

Token::Token(std::int32_t index, std::int32_t id, TokenClass token_class,
             std::int32_t position, std::int32_t start, std::int32_t end,
             std::string surface, std::shared_ptr<dict::Dict> dict,
             std::shared_ptr<dict::UserDict> user_dict)
    : index_(index), id_(id), class_(token_class), position_(position),
      start_(start), end_(end), surface_(std::move(surface)),
      dict_(std::move(dict)), user_dict_(std::move(user_dict)) {}

Token::Token(std::string surface, std::int32_t id, std::int32_t start,
             std::shared_ptr<dict::Dict> dict,
             std::shared_ptr<dict::UserDict> user_dict)
    : index_(0), id_(id), class_(TokenClass::Known), position_(start),
      start_(start), end_(start + static_cast<std::int32_t>(surface.length())),
      surface_(std::move(surface)), dict_(std::move(dict)), user_dict_(std::move(user_dict)) {
    
    // Determine token class based on dictionary content
    if (user_dict && static_cast<std::size_t>(id) < user_dict->contents.size()) {
        class_ = TokenClass::User;
    } else if (dict && static_cast<std::size_t>(id) < dict->unk_dict.morphs.size()) {
        class_ = TokenClass::Unknown;
    } else if (dict && static_cast<std::size_t>(id) < dict->morphs.size()) {
        class_ = TokenClass::Known;
    } else {
        class_ = TokenClass::Dummy;
    }
}

std::vector<std::string> Token::features() const {
    switch (class_) {
        case TokenClass::Known: {
            if (!dict_) return {};
            
            std::vector<std::string> features;
            
            // Add POS features
            if (static_cast<std::size_t>(id_) < dict_->pos_table.pos_entries.size()) {
                const auto& pos_ids = dict_->pos_table.pos_entries[id_];
                for (auto pos_id : pos_ids) {
                    if (pos_id < dict_->pos_table.name_list.size()) {
                        features.push_back(dict_->pos_table.name_list[pos_id]);
                    }
                }
            }
            
            // Add content features
            if (static_cast<std::size_t>(id_) < dict_->contents.size()) {
                const auto& content = dict_->contents[id_];
                features.insert(features.end(), content.begin(), content.end());
            }
            
            return features;
        }
        
        case TokenClass::Unknown: {
            if (!dict_ || static_cast<std::size_t>(id_) >= dict_->unk_dict.contents.size()) {
                return {};
            }
            
            const auto& content = dict_->unk_dict.contents[id_];
            return std::vector<std::string>(content.begin(), content.end());
        }
        
        case TokenClass::User: {
            if (!user_dict_ || static_cast<std::size_t>(id_) >= user_dict_->contents.size()) {
                return {};
            }
            
            const auto& entry = user_dict_->contents[id_];
            std::vector<std::string> features;
            features.push_back(entry.pos);
            
            // Join tokens with "/"
            if (!entry.tokens.empty()) {
                std::string tokens = entry.tokens[0];
                for (std::size_t i = 1; i < entry.tokens.size(); ++i) {
                    tokens += "/" + entry.tokens[i];
                }
                features.push_back(std::move(tokens));
            }
            
            // Join yomi with "/"
            if (!entry.yomi.empty()) {
                std::string yomi = entry.yomi[0];
                for (std::size_t i = 1; i < entry.yomi.size(); ++i) {
                    yomi += "/" + entry.yomi[i];
                }
                features.push_back(std::move(yomi));
            }
            
            return features;
        }
        
        case TokenClass::Dummy:
        default:
            return {};
    }
}

std::optional<std::string> Token::feature_at(std::size_t index) const {
    const auto features = this->features();
    if (index >= features.size()) {
        return std::nullopt;
    }
    return features[index];
}

std::vector<std::string> Token::pos() const {
    switch (class_) {
        case TokenClass::Known: {
            // Try POS table lookup first
            if (dict_ && static_cast<std::size_t>(id_) < dict_->pos_table.pos_entries.size()) {
                const auto& pos_ids = dict_->pos_table.pos_entries[id_];
                std::vector<std::string> pos_names;
                pos_names.reserve(pos_ids.size());
                
                for (auto pos_id : pos_ids) {
                    if (pos_id < dict_->pos_table.name_list.size()) {
                        pos_names.push_back(dict_->pos_table.name_list[pos_id]);
                    }
                }
                
                if (!pos_names.empty()) {
                    return pos_names;
                }
            }
            
            // Fallback to direct feature access for IPA dictionary format
            // In IPA format: [0]=pos1, [1]=pos2, [2]=base_form, [3]=reading, [4]=pronunciation
            std::vector<std::string> pos_features;
            auto pos1 = feature_at(0);
            auto pos2 = feature_at(1);
            
            if (pos1 && *pos1 != "*") {
                pos_features.push_back(*pos1);
            }
            if (pos2 && *pos2 != "*") {
                pos_features.push_back(*pos2);
            }
            
            return pos_features;
        }
        
        case TokenClass::Unknown: {
            if (!dict_) return {};
            
            auto start_it = dict_->unk_dict.contents_meta.find(
                std::string(dict::POS_START_INDEX));
            auto hierarchy_it = dict_->unk_dict.contents_meta.find(
                std::string(dict::POS_HIERARCHY));
            
            std::size_t start = (start_it != dict_->unk_dict.contents_meta.end()) 
                                ? start_it->second : 0;
            std::size_t hierarchy = (hierarchy_it != dict_->unk_dict.contents_meta.end()) 
                                    ? hierarchy_it->second : 1;
            std::size_t end = start + hierarchy;
            
            if (static_cast<std::size_t>(id_) >= dict_->unk_dict.contents.size()) {
                return {};
            }
            
            const auto& feature = dict_->unk_dict.contents[id_];
            if (start >= end || end > feature.size()) {
                return {};
            }
            
            std::vector<std::string> pos_features;
            pos_features.reserve(end - start);
            for (std::size_t i = start; i < end; ++i) {
                pos_features.push_back(feature[i]);
            }
            
            return pos_features;
        }
        
        case TokenClass::User: {
            if (!user_dict_ || static_cast<std::size_t>(id_) >= user_dict_->contents.size()) {
                return {};
            }
            
            return {user_dict_->contents[id_].pos};
        }
        
        case TokenClass::Dummy:
        default:
            return {};
    }
}

std::optional<std::string> Token::pickup_from_features(std::string_view key) const {
    ankerl::unordered_dense::map<std::string, std::uint32_t>* meta = nullptr;
    
    switch (class_) {
        case TokenClass::Known:
            if (dict_) {
                meta = &dict_->contents_meta;
            }
            break;
        case TokenClass::Unknown:
            if (dict_) {
                meta = &dict_->unk_dict.contents_meta;
            }
            break;
        case TokenClass::Dummy:
        case TokenClass::User:
        default:
            return std::nullopt;
    }
    
    if (!meta) return std::nullopt;
    
    auto it = meta->find(std::string(key));
    if (it == meta->end()) {
        return std::nullopt;
    }
    
    return feature_at(static_cast<std::size_t>(it->second));
}

std::string Token::inflectional_type() const {
    auto result = pickup_from_features(dict::INFLECTIONAL_TYPE);
    return result.value_or("*");
}

std::string Token::inflectional_form() const {
    auto result = pickup_from_features(dict::INFLECTIONAL_FORM);
    return result.value_or("*");
}

std::string Token::base_form() const {
    // Try metadata lookup first
    auto result = pickup_from_features(dict::BASE_FORM_INDEX);
    if (result && *result != "*") {
        return *result;
    }
    
    // Fallback to direct index access for IPA dictionary format
    // In IPA format: [0]=pos1, [1]=pos2, [2]=base_form, [3]=reading, [4]=pronunciation
    auto feature = feature_at(2);
    return feature.value_or("*");
}

std::string Token::reading() const {
    // Try metadata lookup first
    auto result = pickup_from_features(dict::READING_INDEX);
    if (result && *result != "*") {
        return *result;
    }
    
    // Fallback to direct index access for IPA dictionary format
    auto feature = feature_at(3);
    return feature.value_or("*");
}

std::string Token::pronunciation() const {
    // Try metadata lookup first
    auto result = pickup_from_features(dict::PRONUNCIATION_INDEX);
    if (result && *result != "*") {
        return *result;
    }
    
    // Fallback to direct index access for IPA dictionary format
    auto feature = feature_at(4);
    return feature.value_or("*");
}

std::optional<UserExtra> Token::user_extra() const {
    if (class_ != TokenClass::User || !user_dict_ || 
        static_cast<std::size_t>(id_) >= user_dict_->contents.size()) {
        return std::nullopt;
    }
    
    const auto& entry = user_dict_->contents[id_];
    return UserExtra{entry.tokens, entry.yomi};
}

bool Token::equal_features(const Token& other) const {
    return utils::equal_features(this->features(), other.features());
}

bool Token::equal_pos(const Token& other) const {
    return utils::equal_features(this->pos(), other.pos());
}

TokenData Token::to_token_data() const {
    TokenData data;
    data.id = id_;
    data.start = start_;
    data.end = end_;
    data.surface = surface_;
    data.token_class = std::string(kagome::tokenizer::to_string(class_));
    data.pos = pos();
    data.features = features();
    
    // Since these now return strings directly, just assign them
    data.base_form = base_form();
    data.reading = reading();
    data.pronunciation = pronunciation();
    
    return data;
}

std::string Token::to_string() const {
    return kagome::format("{}:\"{}\" ({}: {}, {}) {} [{}]",
                      index_, surface_, position_, start_, end_, 
                      kagome::tokenizer::to_string(class_), id_);
}

bool Token::operator==(const Token& other) const noexcept {
    return id_ == other.id_ && 
           class_ == other.class_ && 
           surface_ == other.surface_;
}

bool Token::operator!=(const Token& other) const noexcept {
    return !(*this == other);
}

namespace utils {

bool equal_features(const std::vector<std::string>& lhs,
                   const std::vector<std::string>& rhs) noexcept {
    return lhs.size() == rhs.size() && 
           std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

} // namespace utils

} // namespace kagome::tokenizer 