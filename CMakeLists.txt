cmake_minimum_required(VERSION 3.25)
project(kagome_cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(PkgConfig REQUIRED)

# Set PKG_CONFIG_PATH for Homebrew on macOS
if(APPLE)
    set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/opt/icu4c/lib/pkgconfig:/opt/homebrew/opt/libarchive/lib/pkgconfig:/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    # Also add library directory for linking
    link_directories(/opt/homebrew/opt/icu4c/lib)
endif()

# Find ICU
pkg_check_modules(ICU REQUIRED icu-uc icu-io)

# Find libarchive for ZIP file handling
pkg_check_modules(LIBARCHIVE REQUIRED libarchive)

# Add dependencies via FetchContent
include(FetchContent)

# Add fmt library
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

# Add unordered_dense
FetchContent_Declare(
    unordered_dense
    GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
    GIT_TAG v4.5.0
)
FetchContent_MakeAvailable(unordered_dense)

# Create the main library
add_library(kagome_cpp STATIC
    src/tokenizer/token.cpp
    src/tokenizer/tokenizer.cpp
    src/tokenizer/lattice/lattice.cpp
    src/tokenizer/lattice/node.cpp
    src/dict/dict.cpp
    src/dict/binary_loader.cpp
)

target_include_directories(kagome_cpp PUBLIC
    include
    ${ICU_INCLUDE_DIRS}
    ${LIBARCHIVE_INCLUDE_DIRS}
)

target_link_libraries(kagome_cpp PUBLIC
    fmt::fmt
    unordered_dense::unordered_dense
    ${ICU_LIBRARIES}
    ${LIBARCHIVE_LIBRARIES}
)

target_compile_options(kagome_cpp PRIVATE
    ${ICU_CFLAGS_OTHER}
    -Wall -Wextra -Wpedantic
)

# Create the C API library
add_library(kagome_c_api STATIC
    src/c_api/kagome_c_api.cpp
)

target_include_directories(kagome_c_api PUBLIC
    include
    ${ICU_INCLUDE_DIRS}
)

target_link_libraries(kagome_c_api PUBLIC
    kagome_cpp
    ${ICU_LIBRARIES}
)

target_compile_options(kagome_c_api PRIVATE
    ${ICU_CFLAGS_OTHER}
    -Wall -Wextra -Wpedantic
)

# Create the shared library for Rspamd plugin
add_library(kagome_rspamd_tokenizer SHARED
    src/rspamd_plugin/kagome_tokenizer.cpp
)

target_include_directories(kagome_rspamd_tokenizer PRIVATE
    include
)

target_link_libraries(kagome_rspamd_tokenizer PRIVATE
    kagome_c_api
    kagome_cpp
)

# Set proper SONAME and version for the shared library
set_target_properties(kagome_rspamd_tokenizer PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    OUTPUT_NAME "kagome_rspamd_tokenizer"
    PREFIX ""
)

# Ensure symbols are exported
target_compile_definitions(kagome_rspamd_tokenizer PRIVATE 
    -DBUILDING_SHARED_LIBRARY
)

# Copy dictionary data to build directory
file(COPY ${CMAKE_SOURCE_DIR}/data/ipa/ipa.dict 
     DESTINATION ${CMAKE_BINARY_DIR}/data/ipa/)

# Create the main executable
add_executable(kagome_main
    src/main.cpp
)

target_link_libraries(kagome_main PRIVATE
    kagome_cpp
)

# Create the test plugin executable
add_executable(kagome_test_plugin
    src/test_plugin.cpp
)

target_link_libraries(kagome_test_plugin PRIVATE
    kagome_c_api
    kagome_cpp
)

# Create the debug tokenizer executable
add_executable(kagome_debug_tokenizer
    src/debug_tokenizer.cpp
)

target_link_libraries(kagome_debug_tokenizer PRIVATE
    kagome_cpp
)

# Create the simple test executable
add_executable(kagome_simple_test
    src/simple_test.cpp
)

target_link_libraries(kagome_simple_test PRIVATE
    kagome_cpp
)

# Create the deep debug executable
add_executable(kagome_deep_debug
    src/deep_debug.cpp
)

target_link_libraries(kagome_deep_debug PRIVATE
    kagome_cpp
)

# Create the lattice fix test executable
add_executable(kagome_lattice_fix_test
    src/lattice_fix_test.cpp
)

target_link_libraries(kagome_lattice_fix_test PRIVATE
    kagome_cpp
)

# Create the debug dict sizes executable
add_executable(kagome_debug_dict_sizes
    src/debug_dict_sizes.cpp
)

target_link_libraries(kagome_debug_dict_sizes PRIVATE
    kagome_cpp
)

# Create the debug features executable
add_executable(kagome_debug_features
    src/debug_features.cpp
)

target_link_libraries(kagome_debug_features PRIVATE
    kagome_cpp
)

# Create the debug metadata executable
add_executable(kagome_debug_metadata
    src/debug_metadata.cpp
)

target_link_libraries(kagome_debug_metadata PRIVATE
    kagome_cpp
)

# Create the debug user sentence executable
add_executable(kagome_debug_user_sentence
    debug_user_sentence.cpp
)

target_link_libraries(kagome_debug_user_sentence PRIVATE
    kagome_cpp
)

# Tests
enable_testing()
add_executable(kagome_tests
    tests/tokenizer_test.cpp
    tests/test_main.cpp
)

target_link_libraries(kagome_tests PRIVATE
    kagome_cpp
)

add_test(NAME kagome_tests COMMAND kagome_tests) 