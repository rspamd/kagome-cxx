# Contributing to Kagome C++

Thank you for your interest in contributing to Kagome C++! This document provides guidelines for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Environment](#development-environment)
- [Coding Standards](#coding-standards)
- [Testing](#testing)
- [Pull Request Process](#pull-request-process)
- [Issue Reporting](#issue-reporting)
- [Documentation](#documentation)

## Code of Conduct

We are committed to providing a welcoming and inclusive environment for all contributors. Please be respectful and constructive in all interactions.

## Getting Started

### Prerequisites

- **C++23 compatible compiler**: GCC 12+, Clang 15+, or MSVC 2022+
- **CMake 3.25+**
- **libicu-dev**: Unicode support
- **libfmt-dev**: String formatting
- **libarchive-dev**: Archive handling
- **pkg-config**: Build configuration

### Fork and Clone

```bash
# Fork the repository on GitHub, then clone your fork
git clone https://github.com/YOUR_USERNAME/kagome-cxx.git
cd kagome-cxx

# Add upstream remote
git remote add upstream https://github.com/rspamd/kagome-cxx.git
```

### Build the Project

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run tests
ctest
./kagome_tests
```

## Development Environment

### Recommended Tools

- **IDE**: Visual Studio Code with C++ extensions, CLion, or similar
- **Debugger**: GDB, LLDB, or IDE integrated debugger
- **Static Analysis**: clang-tidy, cppcheck
- **Memory Checking**: AddressSanitizer, Valgrind
- **Formatting**: clang-format (config included)

### Build Types

```bash
# Debug build (default)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug with sanitizers
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZER=ON ..
```

### Development Dependencies

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake pkg-config
sudo apt install libfmt-dev libicu-dev libarchive-dev
sudo apt install clang-tidy clang-format

# macOS
brew install cmake fmt icu4c libarchive pkg-config
brew install llvm  # for clang-tidy and clang-format
```

## Coding Standards

### C++ Version and Features

- **C++23 Standard**: Use modern C++23 features where appropriate
- **Concepts**: Use concepts for template constraints
- **Ranges**: Prefer ranges over traditional iterator-based loops
- **fmt formatting**: Use kagome::format (fmt::format) for string formatting instead of printf/iostream
- **std::optional**: Use for nullable values
- **Smart Pointers**: Use RAII and smart pointers for memory management

### Code Style

We use `clang-format` for code formatting. The configuration is in `.clang-format`.

```bash
# Format all source files
find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check formatting
find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run --Werror
```

#### Naming Conventions

- **Classes/Structs**: `PascalCase` (e.g., `TokenizerConfig`)
- **Functions/Methods**: `snake_case` (e.g., `tokenize_text()`)
- **Variables**: `snake_case` (e.g., `token_count`)
- **Constants**: `UPPER_CASE` (e.g., `MAX_TOKEN_LENGTH`)
- **Namespaces**: `snake_case` (e.g., `kagome::tokenizer`)
- **Files**: `snake_case` (e.g., `tokenizer.hpp`, `lattice.cpp`)

#### Code Organization

```cpp
// File: include/kagome/tokenizer/tokenizer.hpp
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

// Third-party includes
#include <fmt/format.h>

// Project includes
#include "kagome/dict/dict.hpp"
#include "kagome/tokenizer/token.hpp"

namespace kagome::tokenizer {

class Tokenizer {
public:
    // Public interface first
    explicit Tokenizer(std::shared_ptr<dict::Dict> dictionary);
    
    auto tokenize(const std::string& text) -> std::vector<Token>;
    
private:
    // Private members last
    std::shared_ptr<dict::Dict> dict_;
};

} // namespace kagome::tokenizer
```

### Modern C++ Guidelines

#### Use of fmt formatting

```cpp
// Good: Use kagome::format (which is fmt::format)
auto message = kagome::format("Processing {} tokens", token_count);

// Avoid: printf-style formatting
char buffer[256];
sprintf(buffer, "Processing %d tokens", token_count);
```

#### Use of Concepts

```cpp
// Define concepts for template constraints
template<typename T>
concept StringLike = requires(T t) {
    { t.c_str() } -> std::convertible_to<const char*>;
    { t.size() } -> std::convertible_to<size_t>;
};

template<StringLike S>
auto process_text(const S& text) -> std::vector<Token> {
    // Implementation
}
```

#### Error Handling

```cpp
// Use std::optional for nullable returns
auto find_token(const std::string& surface) -> std::optional<Token>;

// Use exceptions for exceptional cases
class TokenizerError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Use expected<T, E> when available (C++23)
auto parse_features(const std::string& data) -> std::expected<Features, ParseError>;
```

## Testing

### Test Framework

We use a simple custom test framework. Tests are located in the `tests/` directory.

### Writing Tests

```cpp
// tests/new_feature_test.cpp
#include "test_framework.hpp"
#include "kagome/tokenizer/tokenizer.hpp"

TEST_CASE("Tokenizer handles empty input") {
    auto dict = create_test_dictionary();
    kagome::tokenizer::Tokenizer tokenizer(dict);
    
    auto tokens = tokenizer.tokenize("");
    REQUIRE(tokens.empty());
}

TEST_CASE("Tokenizer processes Japanese text") {
    auto dict = create_test_dictionary();
    kagome::tokenizer::Tokenizer tokenizer(dict);
    
    auto tokens = tokenizer.tokenize("こんにちは");
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].surface() == "こんにちは");
}
```

### Running Tests

```bash
# Build and run all tests
make kagome_tests
./kagome_tests

# Run specific test
./kagome_tests --filter "Tokenizer"

# Run with verbose output
./kagome_tests --verbose

# Memory checking
valgrind --tool=memcheck ./kagome_tests
```

### Test Categories

1. **Unit Tests**: Test individual components in isolation
2. **Integration Tests**: Test component interactions
3. **Regression Tests**: Ensure compatibility with original kagome
4. **Performance Tests**: Benchmark critical paths
5. **Rspamd Integration Tests**: Test plugin functionality

## Pull Request Process

### Before Submitting

1. **Rebase on latest main**: `git rebase upstream/main`
2. **Run tests**: Ensure all tests pass
3. **Check formatting**: Run clang-format
4. **Static analysis**: Run clang-tidy
5. **Update documentation**: Update relevant docs

### PR Guidelines

1. **Clear title**: Describe what the PR does
2. **Detailed description**: Explain the problem and solution
3. **Link issues**: Reference related issues with `#issue_number`
4. **Small focused changes**: Keep PRs focused on a single concern
5. **Tests included**: Add tests for new functionality
6. **Documentation updated**: Update docs for user-facing changes

### Commit Message Format

```
type(scope): brief description

Detailed explanation of the changes, including:
- What was changed and why
- Any breaking changes
- References to issues

Closes #123
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

Examples:
```
feat(tokenizer): add support for custom user dictionaries

- Implement UserDict class for loading custom dictionaries
- Add priority-based dictionary merging
- Include comprehensive tests for user dictionary functionality

Closes #45

fix(rspamd): handle dictionary loading failures gracefully

Previously, dictionary loading failures would crash the plugin.
Now we fall back to a minimal dictionary and log warnings.

Fixes #67
```

## Issue Reporting

### Bug Reports

Please include:

1. **Environment**: OS, compiler version, dependencies
2. **Steps to reproduce**: Minimal example demonstrating the issue
3. **Expected behavior**: What should happen
4. **Actual behavior**: What actually happens
5. **Additional context**: Logs, stack traces, etc.

### Feature Requests

Please include:

1. **Use case**: Why is this feature needed?
2. **Proposed solution**: How should it work?
3. **Alternatives considered**: Other approaches you've considered
4. **Additional context**: Examples, references, etc.

## Documentation

### API Documentation

- Use Doxygen-style comments for public APIs
- Include usage examples
- Document parameters, return values, and exceptions

```cpp
/**
 * @brief Tokenizes Japanese text into morphological units
 * 
 * @param text The input text to tokenize (UTF-8 encoded)
 * @param mode The tokenization mode (Normal, Search, Extended)
 * @return Vector of tokens with morphological information
 * @throws TokenizerError if dictionary is not loaded
 * 
 * @example
 * ```cpp
 * auto tokens = tokenizer.tokenize("こんにちは世界");
 * for (const auto& token : tokens) {
 *     std::cout << token.surface() << "\n";
 * }
 * ```
 */
auto tokenize(const std::string& text, 
              TokenizeMode mode = TokenizeMode::Normal) -> std::vector<Token>;
```

### User Documentation

- Keep README.md up to date
- Update RSPAMD_INTEGRATION.md for integration changes
- Update DICTIONARY.md for dictionary-related changes
- Include examples and common use cases

## Performance Considerations

### Optimization Guidelines

1. **Profile first**: Measure before optimizing
2. **Algorithmic improvements**: Focus on O(n) improvements
3. **Memory efficiency**: Use object pooling, avoid unnecessary allocations
4. **Cache-friendly code**: Consider data locality
5. **SIMD when applicable**: Use vectorization for hot loops

### Benchmarking

```bash
# Run performance tests
./kagome_performance_tests

# Profile with perf (Linux)
perf record ./kagome_main "large_input_text.txt"
perf report

# Memory profiling
valgrind --tool=massif ./kagome_main "input.txt"
```

## Compatibility

### API Compatibility

- Maintain source compatibility within major versions
- Follow semantic versioning (SemVer)
- Document breaking changes clearly
- Provide migration guides for major version updates

### Platform Support

- **Primary**: Linux (Ubuntu/Debian, RHEL/CentOS)
- **Secondary**: macOS (Homebrew environment)
- **Future**: Windows (with appropriate build system updates)

### Dependency Management

- Keep dependencies minimal and well-justified
- Prefer header-only libraries when possible
- Use FetchContent for C++ dependencies
- Document system package requirements

## Release Process

### Version Tagging

1. Update version in `CMakeLists.txt`
2. Update `CHANGELOG.md`
3. Create git tag: `git tag -a v1.0.0 -m "Release v1.0.0"`
4. Push tag: `git push upstream v1.0.0`

### Package Building

```bash
# Build Debian packages
dpkg-buildpackage -us -uc

# Test installation
sudo dpkg -i ../kagome-*.deb

# Test Rspamd integration
sudo systemctl restart rspamd
```

## Getting Help

- **Issues**: Use GitHub issues for bugs and feature requests
- **Discussions**: Use GitHub discussions for questions
- **Documentation**: Check existing docs first
- **Code review**: Ask for review on complex changes

## License

By contributing to Kagome C++, you agree that your contributions will be licensed under the Apache License, Version 2.0.

## Acknowledgments

- **Sponsor**: [TwoFive Inc.](https://www.twofive25.com/) for supporting this project
- **Original Project**: [kagome](https://github.com/ikawaha/kagome) by ikawaha
- **Contributors**: All contributors to the project
- **Dependencies**: Open source libraries and tools used in development

## Sponsorship

This project is proudly sponsored by [TwoFive Inc.](https://www.twofive25.com/), Japan's leading email security company. Their expertise in email systems and Japanese text processing makes them the perfect sponsor for advancing Japanese tokenization in email security applications.

For sponsorship opportunities, see [SPONSORS.md](SPONSORS.md).