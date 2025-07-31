# Kagome C++ - Japanese Morphological Analyzer

[![CI](https://github.com/rspamd/kagome-cxx/workflows/CI/badge.svg)](https://github.com/rspamd/kagome-cxx/actions/workflows/ci.yml)
[![CodeQL](https://github.com/rspamd/kagome-cxx/workflows/CodeQL/badge.svg)](https://github.com/rspamd/kagome-cxx/actions/workflows/codeql.yml)
[![Release](https://github.com/rspamd/kagome-cxx/workflows/Release/badge.svg)](https://github.com/rspamd/kagome-cxx/actions/workflows/release.yml)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

---

**Sponsored by [TwoFive Inc.](https://www.twofive25.com/)** - Japan's leading email security company, specializing in messaging systems and security solutions.

---

A modern C++ implementation of the Japanese morphological analyzer [kagome](https://github.com/ikawaha/kagome), originally written in Go by [ikawaha](https://github.com/ikawaha). This implementation uses C++23 features and modern libraries for high-performance Japanese text tokenization and provides seamless integration with [Rspamd](https://rspamd.com/) mail processing system.

## Features

- **Modern C++23**: Uses the latest C++ standard features including concepts, ranges, and format library
- **High Performance**: Efficient hash tables from [unordered_dense](https://github.com/martinus/unordered_dense)
- **Unicode Support**: Proper UTF-8/Unicode handling with libicu
- **Multiple Tokenization Modes**: Normal, Search, and Extended modes
- **Lattice-based Analysis**: Uses Viterbi algorithm for optimal path selection
- **Dictionary Support**: System and user dictionary support
- **Memory Efficient**: Object pooling for optimal memory usage

## Dependencies

- **C++23 compatible compiler** (GCC 12+, Clang 15+, or MSVC 2022+)
- **CMake 3.25+**
- **libfmt**: For advanced string formatting
- **libicu**: For Unicode and UTF-8 handling
- **unordered_dense**: Modern hash table implementation (automatically fetched)

### Ubuntu/Debian Installation

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config
sudo apt install libfmt-dev libicu-dev
```

### macOS Installation

```bash
brew install cmake fmt icu4c pkg-config
```

## Building

```bash
# Clone the repository
git clone <repository-url>
cd kagome-cxx

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)

# Run tests
ctest

# Or run tests directly
./kagome_tests


```

## Usage

### Basic Tokenization

```cpp
#include "kagome/tokenizer/tokenizer.hpp"
#include "kagome/dict/dict.hpp"

int main() {
    // Create dictionary
    auto dict = kagome::dict::factory::create_ipa_dict();
    
    // Create tokenizer
    kagome::tokenizer::TokenizerConfig config;
    config.omit_bos_eos = true;
    kagome::tokenizer::Tokenizer tokenizer(dict, config);
    
    // Tokenize text
    auto tokens = tokenizer.tokenize("すもももももももものうち");
    
    for (const auto& token : tokens) {
        auto features = token.features();
        std::string features_str = features.empty() ? "" : 
            std::accumulate(features.begin() + 1, features.end(), features[0],
                           [](const std::string& a, const std::string& b) {
                               return a + "," + b;
                           });
        
        std::cout << kagome::format("{}\t{}\n", token.surface(), features_str);
    }
    
    return 0;
}
```

### Command Line Interface

```bash
# Interactive mode
./kagome_main

# Tokenize specific text
./kagome_main "すもももももももものうち"

# Different modes
./kagome_main -m search "関西国際空港"
./kagome_main -m extended "デジカメを買った"

# Wakati mode (surface forms only)
./kagome_main -w "すもももももももものうち"

# JSON output
./kagome_main -j "猫"
```

### API Examples

#### Different Tokenization Modes

```cpp
// Normal mode - regular segmentation
auto tokens = tokenizer.analyze(text, kagome::tokenizer::TokenizeMode::Normal);

// Search mode - additional segmentation for search
auto tokens = tokenizer.analyze(text, kagome::tokenizer::TokenizeMode::Search);

// Extended mode - unigram unknown words
auto tokens = tokenizer.analyze(text, kagome::tokenizer::TokenizeMode::Extended);
```

#### Wakati Tokenization

```cpp
// Get only surface strings
auto wakati_tokens = tokenizer.wakati("すもももももももものうち");
for (const auto& token : wakati_tokens) {
    std::cout << token.surface() << " ";
}
```

#### Token Information

```cpp
for (const auto& token : tokens) {
    std::cout << "Surface: " << token.surface() << "\n";
    std::cout << "POS: ";
    for (const auto& pos : token.pos()) {
        std::cout << pos << " ";
    }
    std::cout << "\n";
    
    if (auto reading = token.reading()) {
        std::cout << "Reading: " << *reading << "\n";
    }
    
    if (auto pronunciation = token.pronunciation()) {
        std::cout << "Pronunciation: " << *pronunciation << "\n";
    }
}
```

#### Lattice Visualization

```cpp
std::ofstream dot_file("lattice.dot");
auto tokens = tokenizer.analyze_graph(dot_file, "私は猫", 
                                     kagome::tokenizer::TokenizeMode::Normal);

// Convert to PNG using Graphviz
// dot -Tpng lattice.dot -o lattice.png
```

## Architecture

### Core Components

1. **Tokenizer**: Main interface for morphological analysis
2. **Lattice**: Viterbi algorithm implementation for optimal path finding
3. **Dictionary**: System and user dictionary management
4. **Token**: Morphological unit with features and metadata

### Modern C++ Features Used

- **Concepts**: Type constraints for template parameters
- **Ranges**: STL ranges for efficient iteration
- **fmt formatting**: Type-safe string formatting via libfmt
- **std::optional**: Null-safe optional values
- **Smart Pointers**: RAII memory management
- **Move Semantics**: Efficient resource management

### Hash Tables

Uses [unordered_dense](https://github.com/martinus/unordered_dense) for:
- 4x faster than std::unordered_map
- Lower memory usage
- Better cache performance
- Same API as standard containers

### Unicode Support

Leverages libicu for:
- Proper UTF-8 handling
- Character classification (Hiragana, Katakana, Kanji, etc.)
- Script detection
- Unicode normalization

## Rspamd Integration

This library provides a shared library plugin for [Rspamd](https://rspamd.com/) mail processing system, enabling Japanese text tokenization for spam detection and email classification.

### Quick Setup
```bash
# Build the plugin
mkdir build && cd build
cmake ..
make -j4

# Install dictionary (place next to the library)
cp ../data/ipa/ipa.dict ./ipa.dict

# Configure Rspamd
echo 'custom_tokenizers {
    kagome {
        enabled = true;
        path = "/path/to/kagome_rspamd_tokenizer.so";
        priority = 60.0;
    }
}' >> /etc/rspamd/local.d/options.inc

# Restart Rspamd
sudo systemctl restart rspamd
```

For detailed integration instructions, see [RSPAMD_INTEGRATION.md](RSPAMD_INTEGRATION.md).


## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines.

**Project Sponsors**: See [SPONSORS.md](SPONSORS.md) for our amazing sponsors who make this project possible.

**Quick Start for Contributors:**
1. Follow modern C++ best practices (C++23 features encouraged)
2. Maintain API compatibility with the original Go version
3. Add comprehensive tests for new features  
4. Document public interfaces with examples
5. Ensure CI passes (see [GITHUB_ACTIONS.md](GITHUB_ACTIONS.md) for CI details)

**Development Workflow:**
```bash
# Fork and clone the repository
git clone https://github.com/YOUR_USERNAME/kagome-cxx.git
cd kagome-cxx

# Create feature branch
git checkout -b feature/your-feature-name

# Build and test
mkdir build && cd build
cmake ..
make -j$(nproc)
ctest

# Submit pull request
```

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) file for details.

This project is based on the original [kagome](https://github.com/ikawaha/kagome) project by ikawaha.

## Sponsors

This project is proudly sponsored by:

### [TwoFive Inc.](https://www.twofive25.com/)
**Japan's leading email security company** - TwoFive specializes in messaging systems, email security solutions, and infrastructure consulting. As experts in large-scale email systems and Japanese text processing, they recognize the importance of advanced Japanese tokenization for email security and spam detection.

## Acknowledgments

- **Sponsorship**: [TwoFive Inc.](https://www.twofive25.com/) for supporting Japanese email security innovation
- **Original Implementation**: [kagome](https://github.com/ikawaha/kagome) project by ikawaha
- **Performance Libraries**: [unordered_dense](https://github.com/martinus/unordered_dense) by martinus
- **Formatting**: [fmtlib](https://github.com/fmtlib/fmt) formatting library
- **Unicode Support**: [ICU](https://icu.unicode.org/) Unicode library 