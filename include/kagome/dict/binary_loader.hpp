#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <fstream>
#include <cstdint>

#include "kagome/dict/dict.hpp"

namespace kagome::dict {

/// Binary format reader for kagome dictionary files
class BinaryReader {
public:
	explicit BinaryReader(std::istream &stream)
		: stream_(stream)
	{
	}

	/// Read a uint64 value (little endian)
	[[nodiscard]] std::uint64_t read_uint64();

	/// Read a uint32 value (little endian)
	[[nodiscard]] std::uint32_t read_uint32();

	/// Read a int32 value (little endian)
	[[nodiscard]] std::int32_t read_int32();

	/// Read a uint16 value (little endian)
	[[nodiscard]] std::uint16_t read_uint16();

	/// Read a int16 value (little endian)
	[[nodiscard]] std::int16_t read_int16();

	/// Read a string with length prefix
	[[nodiscard]] std::string read_string();

	/// Read raw bytes
	[[nodiscard]] std::vector<std::uint8_t> read_bytes(std::size_t count);

	/// Read all remaining data
	[[nodiscard]] std::vector<std::uint8_t> read_all();

	/// Check if we're at end of stream
	[[nodiscard]] bool eof() const
	{
		return stream_.eof();
	}

private:
	std::istream &stream_;
};

/// Kagome binary dictionary loader
class BinaryDictLoader {
public:
	/// Load dictionary from ZIP file path
	[[nodiscard]] static std::shared_ptr<Dict> load_from_zip(const std::string &zip_path);

	/// Load dictionary from extracted directory
	[[nodiscard]] static std::shared_ptr<Dict> load_from_directory(const std::string &dir_path);

private:
	/// Load individual dictionary components
	static void load_morph_dict(Dict &dict, std::istream &stream);
	static void load_pos_dict(Dict &dict, std::istream &stream);
	static void load_content_meta(Dict &dict, std::istream &stream);
	static void load_content_dict(Dict &dict, std::istream &stream);
	static void load_index_dict(Dict &dict, std::istream &stream);
	static void load_connection_dict(Dict &dict, std::istream &stream);
	static void load_chardef_dict(Dict &dict, std::istream &stream);
	static void load_unk_dict(Dict &dict, std::istream &stream);
	static void load_dict_info(Dict &dict, std::istream &stream);
};

}// namespace kagome::dict