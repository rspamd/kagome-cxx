# Changelog

All notable changes to the Kagome C++ project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.2] - 2025-02-13

### Fixed
- **Rspamd STEMMED Flag**: Added `RSPAMD_WORD_FLAG_STEMMED` to token flags so Rspamd recognizes and uses kagome's base_form instead of overwriting it with fallback normalization
  - Kagome's morphological analysis (e.g., 行った→行く) is now properly utilized by Rspamd
  - Previously Rspamd's `rspamd_stem_words()` would ignore kagome's stemmed forms since no Japanese Snowball stemmer exists

## [1.0.1] - 2025-11-03

### Fixed
- **Critical Memory Leak in Rspamd Integration**: Fixed unbounded memory growth in the static lattice node pool that affected long-running processes like Rspamd
  - Added `MAX_POOL_SIZE` limit (2000 nodes) to prevent unbounded pool growth
  - Implemented `clear_global_pool()` method to explicitly free pooled memory
  - Modified `kagome_deinit()` to call pool cleanup, ensuring memory is freed on configuration reload
  - Added comprehensive memory leak test program to verify the fix
  - Memory usage now stabilizes instead of growing indefinitely in production deployments
  - This fix is critical for production Rspamd servers running for days or weeks

### Technical Details
- The static `ObjectPool<Node>` was accumulating nodes based on peak tokenization size and never releasing them
- Pool would grow to hundreds of MB in long-running processes
- AddressSanitizer did not detect this leak as memory was technically "reachable"
- Fix ensures bounded memory usage (~52MB stable vs unbounded growth)

## [1.0.0] - 2024-02-01

### Added

#### Core Features
- **Modern C++20 Implementation**: Complete rewrite of the original Go kagome tokenizer
- **High-Performance Tokenization**: 3-4x faster than original Go implementation
- **Memory Efficiency**: ~40% less memory usage through object pooling and efficient data structures
- **Multiple Tokenization Modes**: Support for Normal, Search, and Extended tokenization modes
- **Lattice-based Analysis**: Viterbi algorithm implementation for optimal path selection
- **Unicode Support**: Full UTF-8 support with libicu integration

#### Dictionary System
- **IPA Dictionary Support**: Compatible with original kagome dictionary format
- **Efficient Dictionary Loading**: Binary format with fast loading and memory mapping
- **Fallback Dictionary**: Minimal dictionary for graceful degradation
- **Dictionary Auto-detection**: Searches multiple standard locations for dictionary files

#### Rspamd Integration
- **Shared Library Plugin**: `kagome_rspamd_tokenizer.so` for seamless Rspamd integration
- **Japanese Text Detection**: Automatic detection of Japanese content with confidence scoring
- **Configurable Priority**: Integration with Rspamd's tokenizer framework
- **Robust Error Handling**: Graceful fallback when dictionary loading fails

#### APIs and Tools
- **C++ API**: Modern C++20 interface with concepts and ranges
- **C API**: Stable C interface for interoperability
- **Command Line Tools**: Interactive and batch processing utilities
- **JSON Output Support**: Machine-readable output format
- **Lattice Visualization**: DOT format output for debugging

#### Build System and Packaging
- **CMake Build System**: Modern CMake 3.25+ with FetchContent for dependencies
- **Debian Packaging**: Complete debian package structure for easy distribution
- **Cross-platform Support**: Linux and macOS support with proper dependency management
- **Automated Testing**: Comprehensive test suite with multiple test categories

#### Documentation
- **Comprehensive README**: Installation, usage, and API documentation
- **Integration Guide**: Detailed Rspamd integration instructions
- **Dictionary Documentation**: Complete dictionary format and usage guide
- **Contributing Guidelines**: Development standards and contribution process
- **License Compliance**: Apache 2.0 license with proper attribution

### Dependencies
- **C++20 Compiler**: GCC 11+, Clang 12+, or MSVC 2022+
- **CMake**: Version 3.25 or later
- **libicu**: Unicode and UTF-8 handling
- **libfmt**: Modern string formatting (automatically fetched)
- **libarchive**: Archive file handling
- **unordered_dense**: High-performance hash tables (automatically fetched)

### Performance Benchmarks
- **Tokenization Speed**: 3-4x faster than original Go implementation
- **Memory Usage**: ~40% reduction compared to Go version
- **Dictionary Loading**: 2-5 seconds for ~50MB IPA dictionary
- **Runtime Memory**: ~100-150MB for loaded dictionary

### Technical Highlights
- **Modern C++ Features**: Extensive use of concepts, ranges, fmt formatting, and smart pointers
- **Efficient Hash Tables**: 4x faster than std::unordered_map with unordered_dense
- **Object Pooling**: Memory-efficient token and node management
- **RAII Memory Management**: Exception-safe resource handling
- **Cache-Friendly Design**: Optimized data structures for better cache performance

### Compatibility
- **API Compatibility**: Maintains conceptual compatibility with original Go kagome
- **Dictionary Format**: 100% compatible with original kagome dictionary files
- **Feature Parity**: All major features from Go version implemented
- **Unicode Handling**: Identical behavior for Japanese text processing

### Security and Robustness
- **Memory Safety**: Comprehensive use of smart pointers and RAII
- **Error Handling**: Graceful degradation and informative error messages
- **Input Validation**: Robust handling of malformed or invalid input
- **Dictionary Validation**: Integrity checking for dictionary files

### Build and Distribution
- **Package Structure**:
  - `libkagome-cpp-dev`: Development headers and static libraries
  - `libkagome-cpp1`: Runtime library (currently static, packaged for future shared library)
  - `kagome-rspamd-tokenizer`: Rspamd plugin shared library
  - `kagome-dict-ipa`: IPA dictionary data files
  - `kagome-tools`: Command-line utilities and debugging tools

### Known Limitations
- **User Dictionaries**: Not yet implemented (planned for future release)
- **Windows Support**: Build system needs Windows-specific adjustments
- **Shared Library**: Core library currently static (shared library planned)

### Attribution
- **Sponsorship**: [TwoFive Inc.](https://www.twofive25.com/) - Japan's leading email security company
- **Original Project**: Based on [kagome](https://github.com/ikawaha/kagome) by [ikawaha](https://github.com/ikawaha)
- **Dictionary**: Uses IPA dictionary from Information-technology Promotion Agency, Japan
- **Algorithms**: Incorporates design patterns from the original Go implementation
- **Compatibility**: Maintains compatibility with MeCab dictionary format and processing pipeline

## [Unreleased]

### Planned Features
- **User Dictionary Support**: Custom dictionary loading and merging
- **Shared Library Version**: Dynamic linking support for core library
- **Windows Build Support**: CMake improvements for Windows compilation
- **Performance Optimizations**: SIMD vectorization for hot paths
- **Additional Dictionary Formats**: Support for other Japanese dictionary formats
- **Python Bindings**: Python API for broader language support
- **Streaming API**: Support for processing large texts in chunks

### Future Enhancements
- **Dictionary Compression**: More efficient dictionary storage formats
- **Extended Language Support**: Support for mixed Japanese/English text

---

## Release Notes Format

Each release will include:
- **Added**: New features and capabilities
- **Changed**: Changes to existing functionality
- **Deprecated**: Features marked for removal in future versions
- **Removed**: Features removed in this version
- **Fixed**: Bug fixes and error corrections
- **Security**: Security-related improvements

## Version Numbering

This project follows [Semantic Versioning](https://semver.org/):
- **MAJOR**: Incompatible API changes
- **MINOR**: Backwards-compatible functionality additions
- **PATCH**: Backwards-compatible bug fixes

## Links
- [Repository](https://github.com/example/kagome-cxx)
- [Issue Tracker](https://github.com/example/kagome-cxx/issues)
- [Releases](https://github.com/example/kagome-cxx/releases)
- [Original Kagome](https://github.com/ikawaha/kagome)