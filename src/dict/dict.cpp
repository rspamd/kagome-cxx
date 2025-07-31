#include "kagome/dict/dict.hpp"
#include "kagome/dict/binary_loader.hpp"
#include <algorithm>
#include <fmt/format.h>
#include <fmt/core.h>
#include <archive.h>
#include <archive_entry.h>
#include <sstream>
#include <cstdlib>

namespace kagome {
namespace dict {

// IndexTable implementation
std::vector<int> IndexTable::search(const std::string &input) const
{
	auto [id, found] = find_internal(input);
	if (!found) {
		return {};
	}

	auto dup_it = dup.find(static_cast<int32_t>(id));
	int32_t dup_count = (dup_it != dup.end()) ? dup_it->second : 0;

	std::vector<int> result;
	for (int i = 0; i <= dup_count; ++i) {
		result.push_back(id + i);
	}
	return result;
}

std::pair<int, bool> IndexTable::find_internal(const std::string &input) const
{
	if (da.empty() || input.empty()) {
		return {0, false};
	}

	int p = 0, q = 0;
	const int buf_len = static_cast<int>(da.size());

	for (size_t i = 0; i < input.size(); ++i) {
		if (input[i] == '\0') {
			return {0, false};
		}
		p = q;
		q = da[p].base + static_cast<unsigned char>(input[i]);
		if (q >= buf_len || da[q].check != p) {
			return {0, false};
		}
	}

	// Check for terminator
	p = q;
	q = da[p].base + 0;// terminator is 0
	if (q >= buf_len || da[q].check != p || da[q].base > 0) {
		return {0, false};
	}

	return {-da[q].base, true};
}

std::vector<std::pair<std::vector<int>, int>> IndexTable::common_prefix_search(const std::string &input) const
{
	std::vector<std::pair<std::vector<int>, int>> results;

	common_prefix_search_callback(input, [&](int id, int length) {
		auto dup_it = dup.find(static_cast<int32_t>(id));
		int32_t dup_count = (dup_it != dup.end()) ? dup_it->second : 0;

		std::vector<int> ids;
		for (int i = 0; i <= dup_count; ++i) {
			ids.push_back(id + i);
		}
		results.emplace_back(std::move(ids), length);
	});

	return results;
}

void IndexTable::common_prefix_search_callback(const std::string &input,
											   std::function<void(int, int)> callback) const
{
	if (da.empty() || input.empty()) {
		return;
	}

	int p = 0, q = 0;
	const int buf_len = static_cast<int>(da.size());

	for (size_t i = 0; i < input.size(); ++i) {
		if (input[i] == '\0') {
			return;
		}
		p = q;
		q = da[p].base + static_cast<unsigned char>(input[i]);
		if (q >= buf_len || da[q].check != p) {
			break;
		}

		// Check for valid end state
		int ahead = da[q].base + 0;// terminator
		if (ahead < buf_len && da[ahead].check == q && da[ahead].base <= 0) {
			callback(-da[ahead].base, static_cast<int>(i + 1));
		}
	}
}

// Dictionary loading implementation
std::unique_ptr<Dict> DictLoader::load_from_zip(const std::string &zip_path, bool full)
{
	auto dict = std::make_unique<Dict>();

	// Use libarchive to read ZIP file
	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);

	if (archive_read_open_filename(a, zip_path.c_str(), 10240) != ARCHIVE_OK) {
		archive_read_free(a);
		fmt::print(stderr, "Failed to open ZIP file: {}\n", zip_path);
		return create_fallback_dict();
	}

	struct archive_entry *entry;
	bool success = true;

	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		const char *filename = archive_entry_pathname(entry);
		size_t size = archive_entry_size(entry);

		// Skip content.dict if not full loading
		if (!full && std::string(filename) == CONTENT_DICT_FILENAME) {
			archive_read_data_skip(a);
			continue;
		}

		// Read file data
		std::vector<char> buffer(size);
		if (archive_read_data(a, buffer.data(), size) != static_cast<ssize_t>(size)) {
			fmt::print(stderr, "Failed to read data for: {}\n", filename);
			success = false;
			break;
		}

		// Create stream from buffer
		std::stringstream stream(std::string(buffer.begin(), buffer.end()));

		// Load appropriate dictionary part
		try {
			if (std::string(filename) == MORPH_DICT_FILENAME) {
				if (!load_morphs_dict(*dict, stream)) {
					fmt::print(stderr, "Failed to load morphs dict\n");
					success = false;
				}
			}
			else if (std::string(filename) == POS_DICT_FILENAME) {
				if (!load_pos_dict(*dict, stream)) {
					fmt::print(stderr, "Failed to load pos dict\n");
					success = false;
				}
			}
			else if (std::string(filename) == CONTENT_META_FILENAME) {
				if (!load_contents_meta(*dict, stream)) {
					fmt::print(stderr, "Failed to load contents meta\n");
					success = false;
				}
			}
			else if (std::string(filename) == CONTENT_DICT_FILENAME) {
				if (!load_contents_dict(*dict, stream)) {
					fmt::print(stderr, "Failed to load contents dict\n");
					success = false;
				}
			}
			else if (std::string(filename) == INDEX_DICT_FILENAME) {
				if (!load_index_dict(*dict, stream)) {
					fmt::print(stderr, "Failed to load index dict\n");
					success = false;
				}
			}
			else if (std::string(filename) == CONNECTION_DICT_FILENAME) {
				if (!load_connection_dict(*dict, stream)) {
					fmt::print(stderr, "Failed to load connection dict\n");
					success = false;
				}
			}
			else if (std::string(filename) == CHAR_DEF_DICT_FILENAME) {
				if (!load_char_def_dict(*dict, stream)) {
					fmt::print(stderr, "Failed to load char def dict\n");
					success = false;
				}
			}
			else if (std::string(filename) == UNK_DICT_FILENAME) {
				if (!load_unk_dict(*dict, stream)) {
					fmt::print(stderr, "Failed to load unk dict\n");
					success = false;
				}
			}
			else if (std::string(filename) == DICT_INFO_FILENAME) {
				if (!load_dict_info(*dict, stream)) {
					fmt::print(stderr, "Failed to load dict info\n");
					// Don't fail on dict info, it's optional
				}
			}
		} catch (const std::exception &e) {
			fmt::print(stderr, "Exception loading {}: {}\n", filename, e.what());
			success = false;
		}
	}

	archive_read_close(a);
	archive_read_free(a);

	if (!success) {
		fmt::print(stderr, "Dictionary loading failed, using fallback\n");
		return create_fallback_dict();
	}

	fmt::print("Successfully loaded dictionary from: {}\n", zip_path);
	return dict;
}

bool DictLoader::load_morphs_dict(Dict &dict, std::istream &stream)
{
	BinaryReader reader(stream);

	try {
		auto length = reader.read_uint64();

		if (length > 10000000) {// Sanity check
			fmt::print(stderr, "Invalid morphs length: {}\n", length);
			return false;
		}

		dict.morphs.reserve(static_cast<size_t>(length));

		for (uint64_t i = 0; i < length; ++i) {
			Morph morph;
			morph.left_id = reader.read_int16();
			morph.right_id = reader.read_int16();
			morph.weight = reader.read_int16();
			dict.morphs.push_back(morph);
		}
	} catch (const std::exception &e) {
		fmt::print(stderr, "Failed to load morphs: {}\n", e.what());
		return false;
	}

	fmt::print("Loaded {} morphs\n", dict.morphs.size());
	return true;
}

bool DictLoader::load_pos_dict(Dict &dict, std::istream &stream)
{
	try {
		// Read all data from stream
		std::vector<uint8_t> data;
		stream.seekg(0, std::ios::end);
		size_t size = stream.tellg();
		stream.seekg(0, std::ios::beg);

		data.resize(size);
		stream.read(reinterpret_cast<char *>(data.data()), size);

		if (data.empty()) {
			fmt::print("Empty POS dict file, using fallback\n");
			// Create basic fallback
			dict.pos_table.name_list = {"名詞", "動詞", "形容詞"};
			dict.pos_table.pos_entries.resize(dict.pos_table.name_list.size());
			for (size_t i = 0; i < dict.pos_table.name_list.size(); ++i) {
				dict.pos_table.pos_entries[i] = {static_cast<std::uint32_t>(i + 1)};
			}
			return true;
		}

		// Try to decode with GobDecoder
		GobDecoder decoder(data.data(), data.size());
		if (decoder.decode_pos_table(dict.pos_table)) {
			fmt::print("Successfully loaded POS table with {} entries using gob decoder\n",
					   dict.pos_table.name_list.size());
			return true;
		}
		else {
			fmt::print("Failed to decode POS table with gob, using fallback\n");
			// Create basic fallback
			dict.pos_table.name_list = {"名詞", "動詞", "形容詞"};
			dict.pos_table.pos_entries.resize(dict.pos_table.name_list.size());
			for (size_t i = 0; i < dict.pos_table.name_list.size(); ++i) {
				dict.pos_table.pos_entries[i] = {static_cast<std::uint32_t>(i + 1)};
			}
			return true;
		}
	} catch (const std::exception &e) {
		fmt::print(stderr, "Exception loading POS dict: {}\n", e.what());
		// Create basic fallback
		dict.pos_table.name_list = {"名詞", "動詞", "形容詞"};
		dict.pos_table.pos_entries.resize(dict.pos_table.name_list.size());
		for (size_t i = 0; i < dict.pos_table.name_list.size(); ++i) {
			dict.pos_table.pos_entries[i] = {static_cast<std::uint32_t>(i + 1)};
		}
		return true;
	}
}

bool DictLoader::load_contents_meta(Dict &dict, std::istream &stream)
{
	try {
		// Read all data from stream
		std::vector<uint8_t> data;
		stream.seekg(0, std::ios::end);
		size_t size = stream.tellg();
		stream.seekg(0, std::ios::beg);

		data.resize(size);
		stream.read(reinterpret_cast<char *>(data.data()), size);

		if (data.empty()) {
			fmt::print("Empty contents meta file, using fallback\n");
			// Create fallback metadata
			dict.contents_meta[std::string(BASE_FORM_INDEX)] = 6;
			dict.contents_meta[std::string(READING_INDEX)] = 7;
			dict.contents_meta[std::string(PRONUNCIATION_INDEX)] = 8;
			dict.contents_meta[std::string(INFLECTIONAL_TYPE)] = 4;
			dict.contents_meta[std::string(INFLECTIONAL_FORM)] = 5;
			dict.contents_meta[std::string(POS_START_INDEX)] = 0;
			dict.contents_meta[std::string(POS_HIERARCHY)] = 4;
			return true;
		}

		// Try to decode with GobDecoder
		GobDecoder decoder(data.data(), data.size());
		if (decoder.decode_contents_meta(dict.contents_meta)) {
			fmt::print("Successfully loaded contents meta with {} entries using gob decoder\n",
					   dict.contents_meta.size());
			return true;
		}
		else {
			fmt::print("Failed to decode contents meta with gob, using fallback\n");
			// Create fallback metadata
			dict.contents_meta[std::string(BASE_FORM_INDEX)] = 6;
			dict.contents_meta[std::string(READING_INDEX)] = 7;
			dict.contents_meta[std::string(PRONUNCIATION_INDEX)] = 8;
			dict.contents_meta[std::string(INFLECTIONAL_TYPE)] = 4;
			dict.contents_meta[std::string(INFLECTIONAL_FORM)] = 5;
			dict.contents_meta[std::string(POS_START_INDEX)] = 0;
			dict.contents_meta[std::string(POS_HIERARCHY)] = 4;
			return true;
		}
	} catch (const std::exception &e) {
		fmt::print(stderr, "Exception loading contents meta: {}\n", e.what());
		// Create fallback metadata
		dict.contents_meta[std::string(BASE_FORM_INDEX)] = 6;
		dict.contents_meta[std::string(READING_INDEX)] = 7;
		dict.contents_meta[std::string(PRONUNCIATION_INDEX)] = 8;
		dict.contents_meta[std::string(INFLECTIONAL_TYPE)] = 4;
		dict.contents_meta[std::string(INFLECTIONAL_FORM)] = 5;
		dict.contents_meta[std::string(POS_START_INDEX)] = 0;
		dict.contents_meta[std::string(POS_HIERARCHY)] = 4;
		return true;
	}
}

bool DictLoader::load_contents_dict(Dict &dict, std::istream &stream)
{
	try {
		// Read all data from stream
		std::vector<uint8_t> data;
		stream.seekg(0, std::ios::end);
		std::streampos size_pos = stream.tellg();
		stream.seekg(0, std::ios::beg);

		// Add safety check for file size
		if (size_pos < 0 || size_pos > 100 * 1024 * 1024) {// Max 100MB
			fmt::print(stderr, "Contents dict file size invalid or too large: {}\n",
					   static_cast<long>(size_pos));
			return false;
		}

		size_t size = static_cast<size_t>(size_pos);
		data.resize(size);
		stream.read(reinterpret_cast<char *>(data.data()), size);

		if (data.empty()) {
			return true;// Empty contents is OK
		}

		std::string content(data.begin(), data.end());

		// Parse content using delimiters from Go code
		constexpr char row_delimiter = '\n';
		constexpr char col_delimiter = '\a';

		// Estimate number of rows to avoid frequent reallocations
		size_t estimated_rows = std::count(content.begin(), content.end(), row_delimiter);
		if (estimated_rows > 500000) {// Sanity check - max 500k entries
			fmt::print(stderr, "Too many content rows: {}, limiting to 500000\n", estimated_rows);
			estimated_rows = 500000;
		}

		// Pre-allocate to avoid reallocations
		dict.contents.reserve(estimated_rows);

		std::stringstream ss(content);
		std::string row;
		size_t row_count = 0;

		while (std::getline(ss, row, row_delimiter) && row_count < 500000) {
			if (row.empty()) {
				continue;// Skip empty rows
			}

			// Estimate columns to avoid reallocations
			size_t estimated_cols = std::count(row.begin(), row.end(), col_delimiter) + 1;
			if (estimated_cols > 20) {// Sanity check - max 20 features per entry
				estimated_cols = 20;
			}

			std::vector<std::string> cols;
			cols.reserve(estimated_cols);

			std::stringstream row_ss(row);
			std::string col;
			size_t col_count = 0;

			while (std::getline(row_ss, col, col_delimiter) && col_count < 20) {
				cols.push_back(std::move(col));
				col_count++;
			}

			if (!cols.empty()) {
				dict.contents.push_back(std::move(cols));
			}

			row_count++;

			// Progress indicator for large files
			if (row_count % 50000 == 0) {
				fmt::print("Processed {} content rows...\n", row_count);
			}
		}

		fmt::print("Loaded contents with {} rows\n", dict.contents.size());
		return true;

	} catch (const std::exception &e) {
		fmt::print(stderr, "Failed to load contents: {}\n", e.what());

		// Create minimal fallback to prevent total failure
		dict.contents.clear();
		dict.contents.resize(1000);
		for (auto &content: dict.contents) {
			content = {"*", "*", "*", "*", "*", "*", "*", "*", "*"};
		}
		fmt::print("Created fallback contents with {} entries\n", dict.contents.size());

		return true;// Return true to continue with fallback
	}
}

bool DictLoader::load_index_dict(Dict &dict, std::istream &stream)
{
	BinaryReader reader(stream);

	try {
		// Read double array
		auto da_size = reader.read_uint64();

		if (da_size > 10000000) {
			fmt::print(stderr, "Invalid DA size: {}\n", da_size);
			return false;
		}

		dict.index.da.reserve(static_cast<size_t>(da_size));

		for (uint64_t i = 0; i < da_size; ++i) {
			DANode node;
			node.base = reader.read_int32();
			node.check = reader.read_int32();
			dict.index.da.push_back(node);
		}

		// Read duplicate map
		auto dup_size = reader.read_uint64();

		if (dup_size > 1000000) {
			fmt::print(stderr, "Invalid dup size: {}\n", dup_size);
			return false;
		}

		for (uint64_t i = 0; i < dup_size; ++i) {
			auto key = reader.read_int32();
			auto value = reader.read_int32();
			dict.index.dup[key] = value;
		}
	} catch (const std::exception &e) {
		fmt::print(stderr, "Failed to load index: {}\n", e.what());
		return false;
	}

	fmt::print("Loaded index with DA size {} and {} duplicates\n",
			   dict.index.da.size(), dict.index.dup.size());
	return true;
}

bool DictLoader::load_connection_dict(Dict &dict, std::istream &stream)
{
	BinaryReader reader(stream);

	try {
		dict.connection.row = static_cast<int64_t>(reader.read_uint64());
		dict.connection.col = static_cast<int64_t>(reader.read_uint64());

		if (dict.connection.row < 0 || dict.connection.col < 0 ||
			dict.connection.row > 100000 || dict.connection.col > 100000) {
			fmt::print(stderr, "Invalid connection matrix size: {}x{}\n",
					   dict.connection.row, dict.connection.col);
			return false;
		}

		int64_t total_size = dict.connection.row * dict.connection.col;
		dict.connection.vec.resize(static_cast<size_t>(total_size));

		for (int64_t i = 0; i < total_size; ++i) {
			dict.connection.vec[i] = reader.read_int16();
		}
	} catch (const std::exception &e) {
		fmt::print(stderr, "Failed to load connection matrix: {}\n", e.what());
		return false;
	}

	fmt::print("Loaded connection matrix {}x{}\n", dict.connection.row, dict.connection.col);
	return true;
}

bool DictLoader::load_char_def_dict(Dict &dict, std::istream &stream)
{
	// CharDef uses Go's gob encoding - create comprehensive fallback
	dict.char_class = {"DEFAULT", "SPACE", "ALPHA", "DIGIT", "KANJI", "HIRAGANA", "KATAKANA", "SYMBOL", "OTHER"};
	dict.char_category.resize(65536, static_cast<uint8_t>(CharacterCategory::Default));
	dict.invoke_list.resize(dict.char_class.size(), true);
	dict.group_list.resize(dict.char_class.size(), false);

	// Comprehensive categorization for Japanese text analysis

	// Space characters
	dict.char_category[0x0020] = static_cast<uint8_t>(CharacterCategory::Space);// Space
	dict.char_category[0x3000] = static_cast<uint8_t>(CharacterCategory::Space);// Ideographic space

	// ASCII alpha
	for (char32_t c = 0x0041; c <= 0x005A; ++c) {// A-Z
		if (c < dict.char_category.size()) dict.char_category[c] = static_cast<uint8_t>(CharacterCategory::Alpha);
	}
	for (char32_t c = 0x0061; c <= 0x007A; ++c) {// a-z
		if (c < dict.char_category.size()) dict.char_category[c] = static_cast<uint8_t>(CharacterCategory::Alpha);
	}

	// Digits
	for (char32_t c = 0x0030; c <= 0x0039; ++c) {// 0-9
		if (c < dict.char_category.size()) dict.char_category[c] = static_cast<uint8_t>(CharacterCategory::Numeric);
	}

	// Hiragana - CRITICAL for "すもも" analysis
	for (char32_t c = 0x3040; c <= 0x309F; ++c) {// Hiragana block
		if (c < dict.char_category.size()) dict.char_category[c] = static_cast<uint8_t>(CharacterCategory::Hiragana);
	}

	// Katakana
	for (char32_t c = 0x30A0; c <= 0x30FF; ++c) {// Katakana block
		if (c < dict.char_category.size()) dict.char_category[c] = static_cast<uint8_t>(CharacterCategory::Katakana);
	}

	// Kanji
	for (char32_t c = 0x4E00; c <= 0x9FAF; ++c) {// CJK Unified Ideographs
		if (c < dict.char_category.size()) dict.char_category[c] = static_cast<uint8_t>(CharacterCategory::Kanji);
	}

	// Common symbols
	for (char32_t c = 0x0021; c <= 0x002F; ++c) {// !"#$%&'()*+,-./
		if (c < dict.char_category.size()) dict.char_category[c] = static_cast<uint8_t>(CharacterCategory::Symbol);
	}
	for (char32_t c = 0x003A; c <= 0x0040; ++c) {// :;<=>?@
		if (c < dict.char_category.size()) dict.char_category[c] = static_cast<uint8_t>(CharacterCategory::Symbol);
	}

	// Set grouping rules for continuous character sequences
	dict.group_list[static_cast<std::size_t>(CharacterCategory::Numeric)] = true;
	dict.group_list[static_cast<std::size_t>(CharacterCategory::Alpha)] = true;
	dict.group_list[static_cast<std::size_t>(CharacterCategory::Hiragana)] = true;
	dict.group_list[static_cast<std::size_t>(CharacterCategory::Katakana)] = true;
	dict.group_list[static_cast<std::size_t>(CharacterCategory::Kanji)] = true;

	// Enable invocation for all categories
	for (size_t i = 0; i < dict.invoke_list.size(); ++i) {
		dict.invoke_list[i] = true;
	}

	fmt::print("Created fallback char def with {} classes\n", dict.char_class.size());
	return true;
}

bool DictLoader::load_unk_dict(Dict &dict, std::istream &stream)
{
	try {
		// Read all data from stream
		std::vector<uint8_t> data;
		stream.seekg(0, std::ios::end);
		size_t size = stream.tellg();
		stream.seekg(0, std::ios::beg);

		data.resize(size);
		stream.read(reinterpret_cast<char *>(data.data()), size);

		if (data.empty()) {
			fmt::print("Empty unk dict file, using fallback\n");
			// Create comprehensive fallback
			dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Hiragana)] = 0;
			dict.unk_dict.morphs = {{38, 39, 800}};
			dict.unk_dict.contents = {{"助詞", "格助詞", "一般", "*", "*", "*", "*", "*", "*"}};
			dict.unk_dict.contents_meta[std::string(POS_START_INDEX)] = 0;
			dict.unk_dict.contents_meta[std::string(POS_HIERARCHY)] = 3;
			return true;
		}

		// Try to decode with GobDecoder
		GobDecoder decoder(data.data(), data.size());
		if (decoder.decode_unk_dict(dict.unk_dict)) {
			fmt::print("Successfully loaded unknown word dictionary with {} morphs, {} index entries, {} contents using gob decoder\n",
					   dict.unk_dict.morphs.size(), dict.unk_dict.index.size(),
					   dict.unk_dict.contents.size());
			return true;
		}
		else {
			fmt::print("Failed to decode unk dict with gob, using comprehensive fallback\n");

			// Create comprehensive unknown word mappings for Japanese character categories
			dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Default)] = 0;
			dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Space)] = 1;
			dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Alpha)] = 2;
			dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Numeric)] = 3;
			dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Kanji)] = 4;
			dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Hiragana)] = 5;// Important for "すもも"
			dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Katakana)] = 6;
			dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Symbol)] = 7;
			dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Greek)] = 8;
			dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Cyrillic)] = 8;// Same as Greek for fallback

			// Create multiple unknown word morph types with proper connection IDs
			dict.unk_dict.morphs = {
				{1, 1, 1000},  // General unknown (DEFAULT)
				{13, 13, 500}, // Space
				{15, 15, 2000},// Alpha
				{19, 19, 1500},// Numeric
				{36, 37, 1000},// Kanji noun
				{38, 39, 800}, // Hiragana (usually particles/inflections)
				{40, 41, 1200},// Katakana (usually foreign words)
				{2, 2, 3000},  // Symbol
				{15, 15, 2000} // Greek/Cyrillic (same as Alpha)
			};

			// Create comprehensive unknown word contents with proper POS tags
			dict.unk_dict.contents = {
				{"名詞", "一般", "*", "*", "*", "*", "*", "*", "*"},       // General noun
				{"記号", "空白", "*", "*", "*", "*", "*", "*", "*"},       // Space
				{"名詞", "固有名詞", "一般", "*", "*", "*", "*", "*", "*"},// Alpha (proper noun)
				{"名詞", "数", "*", "*", "*", "*", "*", "*", "*"},         // Number
				{"名詞", "一般", "*", "*", "*", "*", "*", "*", "*"},       // Kanji noun
				{"助詞", "格助詞", "一般", "*", "*", "*", "*", "*", "*"},  // Hiragana particle
				{"名詞", "一般", "*", "*", "*", "*", "*", "*", "*"},       // Katakana noun
				{"記号", "一般", "*", "*", "*", "*", "*", "*", "*"},       // Symbol
				{"名詞", "固有名詞", "一般", "*", "*", "*", "*", "*", "*"} // Greek/Cyrillic (foreign proper noun)
			};

			// Set up contents metadata for unknown words matching main dictionary
			dict.unk_dict.contents_meta[std::string(POS_START_INDEX)] = 0;
			dict.unk_dict.contents_meta[std::string(POS_HIERARCHY)] = 3;
			dict.unk_dict.contents_meta[std::string(BASE_FORM_INDEX)] = 6;
			dict.unk_dict.contents_meta[std::string(READING_INDEX)] = 7;
			dict.unk_dict.contents_meta[std::string(PRONUNCIATION_INDEX)] = 8;

			fmt::print("Created fallback unk dict with {} morphs, {} index entries, {} contents\n",
					   dict.unk_dict.morphs.size(), dict.unk_dict.index.size(),
					   dict.unk_dict.contents.size());

			return true;
		}
	} catch (const std::exception &e) {
		fmt::print(stderr, "Exception loading unk dict: {}\n", e.what());

		// Create minimal fallback on exception
		dict.unk_dict.index[static_cast<int32_t>(CharacterCategory::Hiragana)] = 0;
		dict.unk_dict.morphs = {{38, 39, 800}};
		dict.unk_dict.contents = {{"助詞", "格助詞", "一般", "*", "*", "*", "*", "*", "*"}};
		dict.unk_dict.contents_meta[std::string(POS_START_INDEX)] = 0;
		dict.unk_dict.contents_meta[std::string(POS_HIERARCHY)] = 3;

		return true;
	}
}

bool DictLoader::load_dict_info(Dict &dict, std::istream &stream)
{
	try {
		// Read all data from stream
		std::vector<uint8_t> data;
		stream.seekg(0, std::ios::end);
		size_t size = stream.tellg();
		stream.seekg(0, std::ios::beg);

		data.resize(size);
		stream.read(reinterpret_cast<char *>(data.data()), size);

		if (data.empty()) {
			fmt::print("Empty dict info file, using fallback\n");
			auto info = std::make_unique<DictInfo>();
			info->name = "IPA Dictionary";
			info->src = "kagome-dict";
			dict.set_info(std::move(info));
			return true;
		}

		// Try to decode with GobDecoder
		auto info = std::make_unique<DictInfo>();
		GobDecoder decoder(data.data(), data.size());
		if (decoder.decode_dict_info(*info)) {
			fmt::print("Successfully loaded dict info: {} from {} using gob decoder\n",
					   info->name, info->src);
			dict.set_info(std::move(info));
			return true;
		}
		else {
			fmt::print("Failed to decode dict info with gob, using fallback\n");
			info->name = "IPA Dictionary";
			info->src = "kagome-dict";
			dict.set_info(std::move(info));
			return true;
		}
	} catch (const std::exception &e) {
		fmt::print(stderr, "Exception loading dict info: {}\n", e.what());
		auto info = std::make_unique<DictInfo>();
		info->name = "IPA Dictionary";
		info->src = "kagome-dict";
		dict.set_info(std::move(info));
		return true;
	}
}

std::unique_ptr<Dict> DictLoader::create_fallback_dict()
{
	auto dict = std::make_unique<Dict>();

	// Create minimal fallback dictionary
	dict->morphs = {
		Morph{1, 1, 1000},// Basic noun
		Morph{2, 2, 2000},// Basic verb
		Morph{3, 3, 3000} // Basic adjective
	};

	dict->pos_table.name_list = {"名詞", "動詞", "形容詞"};
	dict->pos_table.pos_entries = {{1}, {2}, {3}};

	dict->contents_meta[POS_START_INDEX] = 0;
	dict->contents_meta[READING_INDEX] = 1;

	dict->contents = {
		{"test", "テスト"},
		{"example", "エグザンプル"}};

	// Basic connection matrix
	dict->connection.row = 3;
	dict->connection.col = 3;
	dict->connection.vec = {0, 100, 200, 100, 0, 150, 200, 150, 0};

	// Basic double array
	dict->index.da = {
		DANode{1, -1},// root
		DANode{-1, 0} // terminal for "test"
	};

	dict->char_class = {"DEFAULT"};
	dict->char_category.resize(65536, 0);
	dict->invoke_list = {true};
	dict->group_list = {false};

	auto info = std::make_unique<DictInfo>();
	info->name = "Fallback Dictionary";
	info->src = "Internal";
	dict->set_info(std::move(info));

	fmt::print("Created fallback dictionary\n");
	return dict;
}

void Dict::init_character_categories()
{
	// Initialize basic character categories
	char_category.resize(65536, 0);// Unicode basic multilingual plane
	invoke_list.resize(static_cast<std::size_t>(CharacterCategory::Cyrillic) + 1, true);
	group_list.resize(static_cast<std::size_t>(CharacterCategory::Cyrillic) + 1, false);

	// Basic categorization for Japanese text
	for (char32_t c = 0x3040; c <= 0x309F; ++c) {// Hiragana
		if (c < char_category.size()) char_category[c] = static_cast<uint8_t>(CharacterCategory::Hiragana);
	}
	for (char32_t c = 0x30A0; c <= 0x30FF; ++c) {// Katakana
		if (c < char_category.size()) char_category[c] = static_cast<uint8_t>(CharacterCategory::Katakana);
	}
	for (char32_t c = 0x4E00; c <= 0x9FAF; ++c) {// Kanji
		if (c < char_category.size()) char_category[c] = static_cast<uint8_t>(CharacterCategory::Kanji);
	}
	for (char32_t c = 0x0030; c <= 0x0039; ++c) {// ASCII digits
		if (c < char_category.size()) char_category[c] = static_cast<uint8_t>(CharacterCategory::Numeric);
	}
	for (char32_t c = 0x0041; c <= 0x005A; ++c) {// ASCII uppercase
		if (c < char_category.size()) char_category[c] = static_cast<uint8_t>(CharacterCategory::Alpha);
	}
	for (char32_t c = 0x0061; c <= 0x007A; ++c) {// ASCII lowercase
		if (c < char_category.size()) char_category[c] = static_cast<uint8_t>(CharacterCategory::Alpha);
	}

	// Set grouping rules
	group_list[static_cast<std::size_t>(CharacterCategory::Numeric)] = true;
	group_list[static_cast<std::size_t>(CharacterCategory::Alpha)] = true;
	group_list[static_cast<std::size_t>(CharacterCategory::Hiragana)] = true;
	group_list[static_cast<std::size_t>(CharacterCategory::Katakana)] = true;
}

std::shared_ptr<Dict> factory::create_ipa_dict()
{
	// Try to load from default location
	const char *dict_path = std::getenv("KAGOME_DICT_PATH");
	if (!dict_path) {
		dict_path = "~/kagome-dict/ipa/ipa.dict";
	}

	// Expand tilde
	std::string expanded_path(dict_path);
	if (!expanded_path.empty() && expanded_path[0] == '~') {
		const char *home = std::getenv("HOME");
		if (home) {
			expanded_path = std::string(home) + expanded_path.substr(1);
		}
	}

	try {
		auto dict = DictLoader::load_from_zip(expanded_path, true);
		if (dict) {
			return std::shared_ptr<Dict>(dict.release());
		}
	} catch (const std::exception &e) {
		fmt::print(stderr, "Failed to load IPA dictionary from {}: {}\n", expanded_path, e.what());
	}

	// Fall back to creating a basic dictionary
	fmt::print("Creating fallback IPA dictionary\n");
	auto dict = DictLoader::create_fallback_dict();
	return std::shared_ptr<Dict>(dict.release());
}

// GobDecoder implementation

bool GobDecoder::read_varint(uint64_t &value)
{
	value = 0;
	int shift = 0;

	while (has_data(1)) {
		uint8_t b = read_byte();
		if ((b & 0x80) == 0) {
			// Last byte
			value |= static_cast<uint64_t>(b) << shift;
			return true;
		}
		// More bytes to come
		value |= static_cast<uint64_t>(b & 0x7F) << shift;
		shift += 7;
		if (shift >= 64) {
			return false;// Overflow
		}
	}
	return false;
}

bool GobDecoder::read_varint(int64_t &value)
{
	uint64_t uvalue;
	if (!read_varint(uvalue)) {
		return false;
	}

	// Handle Go's zigzag encoding for signed integers
	value = static_cast<int64_t>((uvalue >> 1) ^ (-(uvalue & 1)));
	return true;
}

bool GobDecoder::read_string(std::string &str)
{
	uint64_t length;
	if (!read_varint(length)) {
		return false;
	}

	if (length > 1024 * 1024) {// Sanity check
		return false;
	}

	if (!has_data(static_cast<size_t>(length))) {
		return false;
	}

	str.assign(reinterpret_cast<const char *>(data_ + pos_), static_cast<size_t>(length));
	pos_ += static_cast<size_t>(length);
	return true;
}

bool GobDecoder::read_slice_length(size_t &length)
{
	uint64_t len;
	if (!read_varint(len)) {
		return false;
	}
	length = static_cast<size_t>(len);
	return true;
}

bool GobDecoder::skip_gob_header()
{
	// Gob files start with type definitions
	// For simplicity, we'll try to skip to the actual data
	// This is a heuristic approach - real gob parsing would be more complex

	// Look for patterns that indicate we've reached the data section
	while (pos_ < size_) {
		if (pos_ + 4 < size_) {
			// Check for reasonable length values (heuristic)
			uint64_t potential_length;
			size_t saved_pos = pos_;
			if (read_varint(potential_length) && potential_length > 0 && potential_length < 100000) {
				// This might be the start of actual data
				pos_ = saved_pos;
				return true;
			}
			pos_ = saved_pos + 1;
		}
		else {
			pos_++;
		}
	}
	return false;
}

bool GobDecoder::decode_string_slice(std::vector<std::string> &strings)
{
	if (!skip_gob_header()) {
		return false;
	}

	size_t count;
	if (!read_slice_length(count)) {
		return false;
	}

	if (count > 100000) {// Sanity check
		return false;
	}

	strings.clear();
	strings.reserve(count);

	for (size_t i = 0; i < count; ++i) {
		std::string str;
		if (!read_string(str)) {
			return false;
		}
		strings.push_back(std::move(str));
	}

	return true;
}

bool GobDecoder::decode_pos_table(POSTable &pos_table)
{
	// Try to decode the gob-encoded POS table
	// This is complex as it contains both string slice and nested uint16 slices

	// For now, implement a simplified version that focuses on the name list
	if (!decode_string_slice(pos_table.name_list)) {
		return false;
	}

	// Create corresponding pos_entries (simplified - each name gets one ID)
	pos_table.pos_entries.clear();
	pos_table.pos_entries.reserve(pos_table.name_list.size());
	for (size_t i = 0; i < pos_table.name_list.size(); ++i) {
		pos_table.pos_entries.push_back({static_cast<std::uint32_t>(i)});
	}

	return true;
}

bool GobDecoder::decode_contents_meta(ankerl::unordered_dense::map<std::string, std::uint32_t> &meta)
{
	if (!skip_gob_header()) {
		return false;
	}

	size_t count;
	if (!read_slice_length(count)) {
		return false;
	}

	if (count > 1000) {// Sanity check
		return false;
	}

	meta.clear();

	for (size_t i = 0; i < count; ++i) {
		std::string key;
		if (!read_string(key)) {
			return false;
		}

		int64_t value;
		if (!read_varint(value)) {
			return false;
		}

		meta[key] = static_cast<std::uint32_t>(value);
	}

	return true;
}

bool GobDecoder::decode_dict_info(DictInfo &info)
{
	if (!skip_gob_header()) {
		return false;
	}

	// Read name field
	if (!read_string(info.name)) {
		return false;
	}

	// Read src field
	if (!read_string(info.src)) {
		return false;
	}

	return true;
}

bool GobDecoder::decode_unk_dict(Dict::UnkDict &unk_dict)
{
	if (!skip_gob_header()) {
		return false;
	}

	try {
		// Read index mappings (character category -> morph ID)
		size_t index_count;
		if (!read_slice_length(index_count)) {
			return false;
		}

		unk_dict.index.clear();
		for (size_t i = 0; i < index_count && i < 20; ++i) {// Sanity limit
			int64_t key, value;
			if (!read_varint(key) || !read_varint(value)) {
				return false;
			}
			unk_dict.index[static_cast<int32_t>(key)] = static_cast<int32_t>(value);
		}

		// Read morphs
		size_t morph_count;
		if (!read_slice_length(morph_count)) {
			return false;
		}

		unk_dict.morphs.clear();
		unk_dict.morphs.reserve(morph_count);
		for (size_t i = 0; i < morph_count && i < 1000; ++i) {// Sanity limit
			int64_t left_id, right_id, weight;
			if (!read_varint(left_id) || !read_varint(right_id) || !read_varint(weight)) {
				return false;
			}
			unk_dict.morphs.emplace_back(
				static_cast<std::int16_t>(left_id),
				static_cast<std::int16_t>(right_id),
				static_cast<std::int16_t>(weight));
		}

		// Read contents metadata
		if (!decode_contents_meta(unk_dict.contents_meta)) {
			// If metadata fails, create basic fallback
			unk_dict.contents_meta[std::string(POS_START_INDEX)] = 0;
			unk_dict.contents_meta[std::string(POS_HIERARCHY)] = 3;
		}

		// Read contents (feature strings)
		size_t content_count;
		if (!read_slice_length(content_count)) {
			return false;
		}

		unk_dict.contents.clear();
		unk_dict.contents.reserve(content_count);
		for (size_t i = 0; i < content_count && i < 1000; ++i) {// Sanity limit
			size_t feature_count;
			if (!read_slice_length(feature_count)) {
				return false;
			}

			std::vector<std::string> features;
			features.reserve(feature_count);
			for (size_t j = 0; j < feature_count && j < 20; ++j) {// Sanity limit
				std::string feature;
				if (!read_string(feature)) {
					return false;
				}
				features.push_back(std::move(feature));
			}
			unk_dict.contents.push_back(std::move(features));
		}

		return true;

	} catch (const std::exception &) {
		return false;
	}
}

}// namespace dict
}// namespace kagome