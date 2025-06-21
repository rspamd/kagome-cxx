# Kagome Rspamd Tokenizer Integration

This directory contains a C++ implementation of a Japanese morphological analyzer (kagome) that can be used as a custom tokenizer plugin for Rspamd.

## Overview

The integration provides:
- A C API wrapper around the kagome C++ library
- An Rspamd-compatible tokenizer plugin with proper language detection
- Japanese text tokenization using the IPA dictionary
- Proper handling of word flags and normalization for Rspamd

## Building

### Prerequisites

- CMake 3.25+
- C++23 compatible compiler (GCC 11+, Clang 14+)
- ICU library (for Unicode handling)
- libarchive (for dictionary loading)
- IPA dictionary data

### Build Steps

```bash
# Create build directory
mkdir build && cd build

# Configure the project
cmake ..

# Build the project
make -j$(nproc)
```

This will create:
- `kagome_rspamd_tokenizer.so` - The shared library for Rspamd
- `kagome_test_plugin` - Test program to verify functionality
- `kagome_main` - Original kagome demo program

## Testing

You can test the tokenizer functionality before integrating with Rspamd:

```bash
./kagome_test_plugin
```

This will test Japanese language detection and tokenization with various text samples.

## Rspamd Integration

### 1. Install the Plugin

Copy the shared library to your Rspamd plugins directory:

```bash
# Example for system installation
sudo cp kagome_rspamd_tokenizer.so /usr/local/lib/rspamd/plugins/

# Or for local installation
cp kagome_rspamd_tokenizer.so /path/to/rspamd/plugins/
```

### 2. Configure Rspamd

Add the tokenizer configuration to your Rspamd configuration:

```lua
-- In your rspamd configuration (e.g., local.d/options.inc)
tokenizers {
  japanese_kagome = {
    enabled = true;
    path = "/usr/local/lib/rspamd/plugins/kagome_rspamd_tokenizer.so";
    priority = 1.0;  -- Higher priority for better Japanese detection
    min_confidence = 0.3;
  };
}
```

### 3. Restart Rspamd

```bash
sudo systemctl restart rspamd
# or
sudo service rspamd restart
```

## API Implementation

The plugin implements the Rspamd custom tokenizer API with these functions:

- `init()` - Loads the IPA dictionary and initializes the tokenizer
- `deinit()` - Cleans up resources
- `detect_language()` - Detects Japanese text based on script analysis
- `tokenize()` - Performs morphological analysis and returns tokens
- `cleanup_result()` - Frees allocated token data
- `get_language_hint()` - Returns "ja" for Japanese
- `get_min_confidence()` - Returns minimum confidence threshold (0.3)

## Features

### Language Detection

The tokenizer uses ICU's script detection to identify Japanese text by looking for:
- Hiragana script (ひらがな)
- Katakana script (カタカナ)  
- Han/Kanji script (漢字)

Confidence is calculated based on the ratio of Japanese characters to total characters.

### Tokenization

The tokenizer provides:
- Surface form extraction
- Base form normalization
- Part-of-speech based stop word detection
- Proper UTF-8/UTF-32 handling
- Memory-safe string allocation and cleanup

### Word Flags

The following Rspamd word flags are set appropriately:
- `RSPAMD_WORD_FLAG_TEXT` - For text tokens
- `RSPAMD_WORD_FLAG_UTF` - For UTF-8 content
- `RSPAMD_WORD_FLAG_NORMALISED` - For normalized forms
- `RSPAMD_WORD_FLAG_STOP_WORD` - For particles and auxiliary verbs

## Dictionary

The tokenizer uses the IPA (IPADIC) dictionary format. The dictionary is loaded automatically during initialization from the following search paths:

1. `data/ipa/ipa.dict` (relative to executable)
2. `../data/ipa/ipa.dict`
3. `../../data/ipa/ipa.dict`
4. User-specific paths (configurable)

## Performance

The tokenizer is designed for email processing workloads:
- Fast initialization with dictionary caching
- Efficient memory management with proper cleanup
- Minimal overhead for non-Japanese text (quick rejection)
- Optimized for typical email text lengths

## Troubleshooting

### Plugin Not Loading

1. Check that the shared library path is correct
2. Verify that all dependencies (ICU, libarchive) are available
3. Check Rspamd logs for loading errors
4. Ensure the dictionary file is accessible

### Poor Detection

1. Verify that the text contains actual Japanese characters
2. Check that ICU is properly installed and working
3. Adjust `min_confidence` if needed (lower values = more sensitive)

### Memory Issues

1. Ensure `cleanup_result()` is being called properly
2. Check for dictionary loading issues during initialization
3. Monitor memory usage during processing

## Example Usage

The tokenizer successfully handles Japanese text:

```bash
# Test the plugin directly
./kagome_test_plugin

# Output for simple Japanese words:
Testing text: "これ"
  Language detection confidence: 0.95  
  Tokenization successful! Found 1 tokens:
    Token 1: Original: "これ", Normalized: "これ", Flags: TEXT|UTF|NORMALISED

Testing text: "日本"
  Language detection confidence: 0.95
  Tokenization successful! Found 1 tokens:
    Token 1: Original: "日本", Normalized: "日本", Flags: TEXT|UTF|NORMALISED
```

This enables proper Japanese text analysis for Rspamd's statistical and rule-based processing.

## Current Limitations

**Note**: The current implementation has a limitation with longer Japanese text sequences. While individual Japanese words and short phrases are tokenized correctly, longer sentences may not be processed due to lattice path-finding issues in the core kagome implementation. This is a known issue that can be addressed in future versions.

The tokenizer works well for:
- ✅ Individual Japanese words (これ, 日本, カタカナ)
- ✅ Short phrases  
- ✅ Language detection for all Japanese text
- ✅ Mixed language detection

Areas for improvement:
- 🔄 Longer Japanese sentences (lattice processing needs enhancement)
- 🔄 Complex morphological analysis for extended text 