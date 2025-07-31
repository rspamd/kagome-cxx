# Build Guide - Kagome C++

This document provides detailed instructions for building Kagome C++ from source on different platforms.

## Prerequisites

### Compiler Requirements
- **C++23 compatible compiler**:
  - GCC 12.0 or later
  - Clang 15.0 or later  
  - MSVC 2022 or later (Visual Studio 17.0+)

### Build Tools
- **CMake 3.25+**
- **pkg-config**
- **Make** (or Ninja)

### System Dependencies

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config
sudo apt install libicu-dev libfmt-dev libarchive-dev
```

#### RHEL/CentOS/Fedora
```bash
# RHEL/CentOS 8+
sudo dnf install gcc-c++ cmake pkg-config
sudo dnf install libicu-devel fmt-devel libarchive-devel

# Older versions
sudo yum install gcc-c++ cmake3 pkg-config
sudo yum install libicu-devel libarchive-devel
# Note: fmt may need to be built from source on older systems
```

#### macOS
```bash
# Using Homebrew
brew install cmake fmt icu4c libarchive pkg-config

# Using MacPorts
sudo port install cmake fmt icu libarchive pkgconfig
```

#### Arch Linux
```bash
sudo pacman -S gcc cmake pkg-config
sudo pacman -S icu fmt libarchive
```

## Building from Source

### Basic Build

```bash
# Clone the repository
git clone https://github.com/example/kagome-cxx.git
cd kagome-cxx

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
make -j$(nproc)

# Run tests
ctest
```

### Build Options

#### Build Types
```bash
# Debug build (default)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Release with debug info
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

# Minimum size release
cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..
```



#### Custom Installation Prefix
```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make install
```

#### Ninja Generator
```bash
# Use Ninja instead of Make (faster builds)
cmake -G Ninja ..
ninja
```

### Advanced Build Options

#### Sanitizers (Debug Builds)
```bash
# Address Sanitizer
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" ..

# Thread Sanitizer
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread" ..

# Undefined Behavior Sanitizer
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=undefined" ..
```

#### Static Analysis
```bash
# Enable clang-tidy
cmake -DCMAKE_CXX_CLANG_TIDY="clang-tidy" ..

# Enable cppcheck
cmake -DCMAKE_CXX_CPPCHECK="cppcheck" ..
```

#### Custom Compiler
```bash
# Use specific compiler
cmake -DCMAKE_CXX_COMPILER=g++-12 ..
cmake -DCMAKE_CXX_COMPILER=clang++-15 ..
```

## Building Specific Components

### Core Library Only
```bash
make kagome_cpp
```

### C API Library
```bash
make kagome_c_api
```

### Rspamd Plugin
```bash
make kagome_rspamd_tokenizer
```

### Command Line Tools
```bash
make kagome_main
make kagome_tests
```

### All Executables
```bash
# All production executables
make kagome_main kagome_tests
```

## Cross-Compilation

### For ARM64 (AArch64)
```bash
# Install cross-compilation toolchain
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Configure for cross-compilation
cmake -DCMAKE_SYSTEM_NAME=Linux \
      -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
      -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
      -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
      -DCMAKE_FIND_ROOT_PATH=/usr/aarch64-linux-gnu \
      -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
      -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
      -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
      ..
```

## Package Building

### Debian Packages
```bash
# Install packaging tools
sudo apt install devscripts debhelper

# Build packages
dpkg-buildpackage -us -uc

# Install packages
sudo dpkg -i ../kagome-*.deb
```

### RPM Packages (Future)
```bash
# Create RPM spec file (to be implemented)
rpmbuild -bb kagome.spec
```

## Platform-Specific Notes

### macOS

#### Apple Silicon (M1/M2)
```bash
# Ensure ARM64 homebrew paths
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"
cmake ..
```

#### Intel Macs
```bash
# Use Intel homebrew paths
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
cmake ..
```

#### Universal Binaries
```bash
cmake -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" ..
```

### Linux

#### Static Linking
```bash
# Link libraries statically (larger binary, no runtime dependencies)
cmake -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" ..
```

#### Custom ICU Installation
```bash
# If ICU is installed in non-standard location
cmake -DICU_ROOT=/path/to/icu ..
```

### Container Builds

#### Docker Build
```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential cmake pkg-config \
    libicu-dev libfmt-dev libarchive-dev

COPY . /src
WORKDIR /src
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)
```

#### Podman Build
```bash
podman build -t kagome-cpp .
```

## Troubleshooting

### Common Issues

#### Missing Dependencies
```bash
# Error: Could not find libfmt
sudo apt install libfmt-dev  # Ubuntu/Debian
sudo dnf install fmt-devel   # RHEL/Fedora

# Error: Could not find ICU
sudo apt install libicu-dev
```

#### CMake Version Too Old
```bash
# Ubuntu 20.04 and older
sudo snap install cmake --classic

# Or build from source
wget https://github.com/Kitware/CMake/releases/download/v3.25.0/cmake-3.25.0.tar.gz
tar xzf cmake-3.25.0.tar.gz && cd cmake-3.25.0
./bootstrap && make -j$(nproc) && sudo make install
```

#### Compiler Version Issues
```bash
# Install newer GCC
sudo apt install gcc-12 g++-12
cmake -DCMAKE_CXX_COMPILER=g++-12 ..

# Install newer Clang
sudo apt install clang-15
cmake -DCMAKE_CXX_COMPILER=clang++-15 ..
```

#### pkg-config Issues
```bash
# macOS: ICU not found
export PKG_CONFIG_PATH="/opt/homebrew/opt/icu4c/lib/pkgconfig:$PKG_CONFIG_PATH"

# Linux: Custom installation paths
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
```

### Build Performance

#### Parallel Builds
```bash
# Use all available cores
make -j$(nproc)

# Limit parallel jobs (for memory-constrained systems)
make -j4
```

#### Ccache (Faster Rebuilds)
```bash
# Install ccache
sudo apt install ccache

# Enable in CMake
cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache ..
```

#### Ninja (Faster Than Make)
```bash
sudo apt install ninja-build
cmake -G Ninja ..
ninja
```

## Testing

### Running Tests
```bash
# CTest
ctest

# Direct execution
./kagome_tests

# Verbose output
ctest --verbose
./kagome_tests --verbose
```

### Memory Testing
```bash
# Valgrind
valgrind --tool=memcheck ./kagome_tests

# AddressSanitizer (compile-time option)
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..
make
./kagome_tests
```

### Performance Testing
```bash
# Build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Profile with perf (Linux)
perf record ./kagome_main "large_text.txt"
perf report
```

## Installation

### System Installation
```bash
sudo make install

# Components installed:
# /usr/local/include/kagome/          - Headers
# /usr/local/lib/libkagome_cpp.a      - Static library
# /usr/local/lib/libkagome_c_api.a    - C API library
# /usr/local/bin/kagome_main          - CLI tool
# /usr/local/share/kagome/ipa.dict    - Dictionary
```

### Custom Installation
```bash
# Install to specific directory
cmake -DCMAKE_INSTALL_PREFIX=/opt/kagome ..
make install
```

### Uninstallation
```bash
# Remove installed files
sudo make uninstall  # If supported by CMake

# Manual removal
sudo rm -rf /usr/local/include/kagome
sudo rm -f /usr/local/lib/libkagome_*.a
sudo rm -f /usr/local/bin/kagome_*
sudo rm -rf /usr/local/share/kagome
```

## Development Build

### IDE Setup

#### Visual Studio Code
```json
{
    "cmake.configureArgs": [
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    ],
    "cmake.buildDirectory": "${workspaceFolder}/build"
}
```

#### CLion
- Open project directory
- CLion will automatically detect CMakeLists.txt
- Configure toolchain in Settings > Build, Execution, Deployment > Toolchains

### Code Formatting
```bash
# Install clang-format
sudo apt install clang-format

# Format all files
find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check formatting
find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run --Werror
```

### Static Analysis
```bash
# Run clang-tidy
find src -name "*.cpp" | xargs clang-tidy -p build/

# Run cppcheck
cppcheck --enable=all --project=build/compile_commands.json
```

## See Also

- [README.md](README.md) - Project overview and basic usage
- [CONTRIBUTING.md](CONTRIBUTING.md) - Development guidelines
- [RSPAMD_INTEGRATION.md](RSPAMD_INTEGRATION.md) - Rspamd plugin setup
- [DICTIONARY.md](DICTIONARY.md) - Dictionary documentation