#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <memory>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <fstream>

#include <ankerl/unordered_dense.h>

namespace kagome {
namespace dict {

// Dictionary file names (matching Go constants)
constexpr const char* MORPH_DICT_FILENAME = "morph.dict";
constexpr const char* POS_DICT_FILENAME = "pos.dict";
constexpr const char* CONTENT_META_FILENAME = "content.meta";
constexpr const char* CONTENT_DICT_FILENAME = "content.dict";
constexpr const char* INDEX_DICT_FILENAME = "index.dict";
constexpr const char* CONNECTION_DICT_FILENAME = "connection.dict";
constexpr const char* CHAR_DEF_DICT_FILENAME = "chardef.dict";
constexpr const char* UNK_DICT_FILENAME = "unk.dict";
constexpr const char* DICT_INFO_FILENAME = "dict.info";

// Content metadata keys for feature extraction
constexpr const char* POS_START_INDEX = "_pos_start";
constexpr const char* POS_HIERARCHY = "_pos_hierarchy";
constexpr const char* INFLECTIONAL_TYPE = "_inflectional_type";
constexpr const char* INFLECTIONAL_FORM = "_inflectional_form";
constexpr const char* BASE_FORM_INDEX = "_base";
constexpr const char* READING_INDEX = "_reading";
constexpr const char* PRONUNCIATION_INDEX = "_pronunciation";

// DictInfo represents the dictionary info
struct DictInfo {
    std::string name;
    std::string src;
};

/// Morphological information for dictionary entries
struct Morph {
    std::int16_t left_id = 0;
    std::int16_t right_id = 0;
    std::int16_t weight = 0;
    
    Morph() = default;
    Morph(std::int16_t l, std::int16_t r, std::int16_t w) 
        : left_id(l), right_id(r), weight(w) {}
};

// Double Array Trie node
struct DANode {
    int32_t base;
    int32_t check;
    
    DANode() : base(0), check(0) {}
    DANode(int32_t b, int32_t c) : base(b), check(c) {}
};

// IndexTable represents a dictionary index using double array trie
class IndexTable {
public:
    std::vector<DANode> da;  // Double Array
    std::unordered_map<int32_t, int32_t> dup;  // Duplicate mappings
    
    // Search functions
    std::vector<int> search(const std::string& input) const;
    std::vector<std::pair<std::vector<int>, int>> common_prefix_search(const std::string& input) const;
    void common_prefix_search_callback(const std::string& input, 
                                     std::function<void(int, int)> callback) const;
    
private:
    std::pair<int, bool> find_internal(const std::string& input) const;
};

// ConnectionTable represents a connection matrix of morphs
struct ConnectionTable {
    int64_t row;
    int64_t col;
    std::vector<int16_t> vec;
    
    ConnectionTable() : row(0), col(0) {}
    
    // At returns the connection cost of matrix[row, col]
    int16_t at(int row_idx, int col_idx) const {
        if (row_idx >= 0 && col_idx >= 0 && 
            row * col_idx + row_idx < static_cast<int64_t>(vec.size())) {
            return vec[row * col_idx + row_idx];  // matrix is transposed
        }
        return 0;
    }
};

/// POS (Parts of Speech) table
struct POSTable {
    /// List of POS names
    std::vector<std::string> name_list;
    /// POS IDs for each entry
    std::vector<std::vector<std::uint32_t>> pos_entries;
};

/// Common prefix search index using trie structure
class PrefixIndex {
public:
    PrefixIndex() = default;
    
    /// Build index from dictionary entries
    void build(const std::vector<std::string>& entries);
    
    /// Search for common prefixes and call callback for each match
    /// Callback signature: void(int id, int length)
    template<typename Callback>
    void common_prefix_search_callback(std::string_view query, Callback&& callback) const;
    
private:
    struct TrieNode {
        ankerl::unordered_dense::map<char, std::unique_ptr<TrieNode>> children;
        std::vector<std::pair<std::int32_t, std::int32_t>> entries; // (id, length)
    };
    
    std::unique_ptr<TrieNode> root_;
    
    void insert(std::string_view str, std::int32_t id);
};

/// Character category classification for unknown word processing
enum class CharacterCategory : std::uint8_t {
    Default = 0,
    Space = 1,
    Numeric = 2,
    Alpha = 3,
    Symbol = 4,
    Hiragana = 5,
    Katakana = 6,
    Kanji = 7,
    Greek = 8,
    Cyrillic = 9
};

/// Unknown word dictionary
struct UnknownDict {
    /// Content metadata
    ankerl::unordered_dense::map<std::string, std::uint32_t> contents_meta;
    /// Content features for unknown words
    std::vector<std::vector<std::string>> contents;
    /// Morphological information
    std::vector<Morph> morphs;
    /// Index mapping from character category to morph ID
    std::vector<std::int32_t> index;
    /// Duplicate count for each category
    std::vector<std::int32_t> index_dup;
};

/// User dictionary entry
struct UserEntry {
    std::string pos;
    std::vector<std::string> tokens;
    std::vector<std::string> yomi;
    
    UserEntry() = default;
    UserEntry(std::string p, std::vector<std::string> t, std::vector<std::string> y)
        : pos(std::move(p)), tokens(std::move(t)), yomi(std::move(y)) {}
};

/// User dictionary
struct UserDict {
    /// Dictionary entries
    std::vector<UserEntry> contents;
    /// Prefix search index (using PrefixIndex class)
    PrefixIndex index;
};

/// Main dictionary class
class Dict {
private:
    std::unique_ptr<DictInfo> dict_info_;

public:
    /// Morphological information for each entry
    std::vector<Morph> morphs;
    
    /// POS table
    POSTable pos_table;
    
    /// Content metadata for feature extraction
    ankerl::unordered_dense::map<std::string, std::uint32_t> contents_meta;
    
    /// Dictionary contents (features for each entry)
    std::vector<std::vector<std::string>> contents;
    
    /// Connection cost matrix
    ConnectionTable connection;
    
    /// Prefix search index using new IndexTable
    IndexTable index;
    
    /// Character definition structures
    std::vector<std::string> char_class;
    std::vector<uint8_t> char_category;
    std::vector<bool> invoke_list;
    std::vector<bool> group_list;
    
    /// Unknown word dictionary
    struct UnkDict {
        std::vector<Morph> morphs;
        std::unordered_map<int32_t, int32_t> index;
        std::unordered_map<int32_t, int32_t> index_dup;
        ankerl::unordered_dense::map<std::string, std::uint32_t> contents_meta;
        std::vector<std::vector<std::string>> contents;
    } unk_dict;
    
    Dict() = default;
    ~Dict() = default;
    
    // Move-only semantics
    Dict(const Dict&) = delete;
    Dict& operator=(const Dict&) = delete;
    Dict(Dict&&) = default;
    Dict& operator=(Dict&&) = default;
    
    void set_info(std::unique_ptr<DictInfo> info) {
        dict_info_ = std::move(info);
    }
    
    const DictInfo* info() const {
        return dict_info_.get();
    }
    
    /// Character category classification
    [[nodiscard]] CharacterCategory character_category(char32_t ch) const {
        if (static_cast<size_t>(ch) < char_category.size()) {
            return static_cast<CharacterCategory>(char_category[ch]);
        }
        return CharacterCategory::Default;
    }
    
    /// Check if character category should invoke unknown word processing
    [[nodiscard]] bool should_invoke(CharacterCategory category) const {
        size_t idx = static_cast<size_t>(category);
        return idx < invoke_list.size() ? invoke_list[idx] : true;
    }
    
    /// Check if character category should be grouped
    [[nodiscard]] bool should_group(CharacterCategory category) const {
        size_t idx = static_cast<size_t>(category);
        return idx < group_list.size() ? group_list[idx] : false;
    }
    
    /// Load dictionary from file/data (legacy)
    bool load_from_file(const std::string& filepath);
    
    /// Initialize character categories (legacy)
    void init_character_categories();

private:
    /// Character category classification table (legacy)
    ankerl::unordered_dense::map<char32_t, CharacterCategory> char_category_map_;
};

// Dictionary factory and loading functions
class DictLoader {
public:
    static std::unique_ptr<Dict> load_from_zip(const std::string& zip_path, bool full = true);
    static std::unique_ptr<Dict> create_fallback_dict();
    
private:
    static bool load_morphs_dict(Dict& dict, std::istream& stream);
    static bool load_pos_dict(Dict& dict, std::istream& stream);
    static bool load_contents_meta(Dict& dict, std::istream& stream);
    static bool load_contents_dict(Dict& dict, std::istream& stream);
    static bool load_index_dict(Dict& dict, std::istream& stream);
    static bool load_connection_dict(Dict& dict, std::istream& stream);
    static bool load_char_def_dict(Dict& dict, std::istream& stream);
    static bool load_unk_dict(Dict& dict, std::istream& stream);
    static bool load_dict_info(Dict& dict, std::istream& stream);
};

/// Factory functions for dictionary creation
namespace factory {

/// Create a dictionary from IPA dictionary data
[[nodiscard]] std::shared_ptr<Dict> create_ipa_dict();

/// Create a dictionary from UniDic data
[[nodiscard]] std::shared_ptr<Dict> create_unidic_dict();

/// Load user dictionary from file
[[nodiscard]] std::shared_ptr<UserDict> load_user_dict(const std::string& filepath);

} // namespace factory

// Go gob format decoder
class GobDecoder {
private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
    
public:
    GobDecoder(const uint8_t* data, size_t size) 
        : data_(data), size_(size), pos_(0) {}
    
    // Read variable-length integer (Go's varint encoding)
    bool read_varint(uint64_t& value);
    bool read_varint(int64_t& value);
    
    // Read string with gob encoding
    bool read_string(std::string& str);
    
    // Read slice length
    bool read_slice_length(size_t& length);
    
    // Skip gob type definitions and headers
    bool skip_gob_header();
    
    // Decode specific Go types
    bool decode_string_slice(std::vector<std::string>& strings);
    bool decode_pos_table(POSTable& pos_table);
    bool decode_contents_meta(ankerl::unordered_dense::map<std::string, std::uint32_t>& meta);
    bool decode_dict_info(DictInfo& info);
    bool decode_unk_dict(Dict::UnkDict& unk_dict);
    
private:
    bool has_data(size_t bytes) const { return pos_ + bytes <= size_; }
    uint8_t read_byte() { return has_data(1) ? data_[pos_++] : 0; }
};

} // namespace dict
} // namespace kagome

/// Template implementation for PrefixIndex
template<typename Callback>
void kagome::dict::PrefixIndex::common_prefix_search_callback(
    std::string_view query, Callback&& callback) const {
    
    if (!root_ || query.empty()) {
        return;
    }
    
    const TrieNode* current = root_.get();
    
    for (std::size_t i = 0; i < query.length(); ++i) {
        char ch = query[i];
        auto it = current->children.find(ch);
        
        if (it == current->children.end()) {
            break;
        }
        
        current = it->second.get();
        
        // Call callback for all entries at this node
        for (const auto& [id, length] : current->entries) {
            callback(id, length);
        }
    }
} 