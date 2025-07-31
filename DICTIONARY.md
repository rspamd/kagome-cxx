# Dictionary Guide - Kagome C++

This document explains the dictionary system used by Kagome C++ for Japanese morphological analysis.

## Overview

Kagome C++ uses morphological dictionaries to analyze Japanese text. The dictionary contains information about:

- **Word entries**: Known Japanese words and their variations
- **Part-of-speech (POS) tags**: Grammatical classifications
- **Readings**: Pronunciation information (hiragana/katakana)
- **Features**: Additional morphological attributes
- **Connection costs**: Weights for optimal path selection in the lattice

## Dictionary Format

### IPA Dictionary

The default dictionary is based on the **IPA (Information-technology Promotion Agency) dictionary**, which is derived from the IPAdic project. This is the same dictionary format used by MeCab and the original Go kagome project.

#### Dictionary File Structure

The dictionary is stored as a binary file (`ipa.dict`) containing:

1. **Header**: Metadata about dictionary version and structure
2. **Word entries**: Compressed morphological data
3. **Connection matrix**: Costs for connecting different POS types
4. **Feature definitions**: POS tag definitions and feature sets

#### Dictionary Location

The Kagome C++ tokenizer searches for the dictionary in the following locations (in order):

1. `./ipa.dict` (current working directory) - **Recommended for Rspamd**
2. `./data/ipa/ipa.dict` (relative to executable)
3. `../data/ipa/ipa.dict` (relative to executable)
4. `../../data/ipa/ipa.dict` (relative to executable)
5. `/usr/local/share/kagome/ipa.dict` (system-wide installation)
6. `/usr/share/kagome/ipa.dict` (distribution package)
7. `/opt/kagome/ipa.dict` (optional installation)

## Dictionary Features

### Part-of-Speech (POS) Tags

The IPA dictionary uses a hierarchical POS tagging system with the following major categories:

#### Main Categories
- **名詞** (Noun): General nouns, proper nouns, pronouns
- **動詞** (Verb): Action words, copulas
- **形容詞** (Adjective): I-adjectives, na-adjectives
- **副詞** (Adverb): Manner, degree, time adverbs
- **助詞** (Particle): Case markers, topic markers
- **助動詞** (Auxiliary verb): Auxiliary and helping verbs
- **連体詞** (Adnominal): Pre-noun modifiers
- **接続詞** (Conjunction): Connecting words
- **感動詞** (Interjection): Exclamations
- **記号** (Symbol): Punctuation, symbols
- **フィラー** (Filler): Hesitation sounds
- **その他** (Other): Miscellaneous, unknown words

#### Detailed Sub-categories

Each main category has multiple sub-categories providing fine-grained classification:

```
名詞,一般,*,*,*,*,猫,ネコ,ネコ
名詞,代名詞,一般,*,*,*,私,ワタシ,ワタシ
名詞,固有名詞,人名,名,*,*,太郎,タロウ,タロー
動詞,自立,*,*,五段・ラ行,基本形,歩く,アルク,アルク
形容詞,自立,*,*,形容詞・イ段,基本形,美しい,ウツクシイ,ウツクシー
```

### Reading Information

The dictionary provides two types of reading information:

1. **Reading (読み)**: Standard pronunciation in katakana
2. **Pronunciation (発音)**: Actual pronunciation accounting for sound changes

### Morphological Features

Each word entry includes comprehensive morphological features:

- **Inflection type**: How the word changes form
- **Inflection form**: Current form of the word
- **Base form**: Dictionary form of the word
- **Reading**: Pronunciation information
- **Semantic information**: Additional meaning-related data

## Dictionary Installation

### For Development

```bash
# Clone the repository with dictionary
git clone <repository-url>
cd kagome-cxx

# Dictionary is included in data/ipa/ipa.dict
ls -la data/ipa/ipa.dict
```

### For Rspamd Integration

```bash
# Copy to the same directory as the plugin (recommended)
cp data/ipa/ipa.dict /path/to/kagome_rspamd_tokenizer.so.dir/ipa.dict

# Or install system-wide
sudo mkdir -p /usr/share/kagome
sudo cp data/ipa/ipa.dict /usr/share/kagome/
```

### For System Installation (Debian/Ubuntu)

```bash
# Install via package manager (when available)
sudo apt install kagome-dict-ipa

# Manual installation
sudo mkdir -p /usr/share/kagome
sudo cp data/ipa/ipa.dict /usr/share/kagome/
sudo chmod 644 /usr/share/kagome/ipa.dict
```

## Dictionary Licensing

### IPA Dictionary License

The IPA dictionary is licensed under a BSD-style license:

- **Copyright**: 2000-2007 Nara Institute of Science and Technology (NAIST)
- **Copyright**: 2008-2023 Information-technology Promotion Agency, Japan (IPA)
- **License**: BSD 3-Clause License (see `debian/copyright` for full text)

### Compatibility

The IPA dictionary license is compatible with:
- ✅ Apache 2.0 (this project's license)
- ✅ MIT License
- ✅ BSD Licenses
- ✅ Commercial use
- ✅ Distribution and modification

## Advanced Dictionary Usage

### Fallback Dictionary

If the main IPA dictionary cannot be loaded, Kagome C++ uses a minimal fallback dictionary containing:

- Basic hiragana/katakana entries
- Common punctuation
- Essential POS tags for basic tokenization

**Note**: Fallback mode provides limited functionality and should only be used for debugging.

### Custom Dictionaries

**Future Enhancement**: Support for user dictionaries is planned for future versions.

Planned features:
- User-defined word entries
- Custom POS tags
- Domain-specific vocabularies
- Dictionary priority levels

### Memory Usage

The IPA dictionary requires:
- **Disk space**: ~50MB (compressed binary format)
- **Memory usage**: ~100-150MB when loaded
- **Loading time**: 2-5 seconds on modern systems

### Performance Considerations

- **Dictionary caching**: Loaded once per process and reused
- **Memory mapping**: Dictionary uses efficient memory mapping when possible
- **Compression**: Binary format reduces memory footprint compared to text format

## Troubleshooting

### Dictionary Not Found

**Error**: "Dictionary file not found"

**Solutions**:
1. Verify dictionary file exists: `ls -la ipa.dict`
2. Check file permissions: `ls -la ipa.dict` (should be readable)
3. Copy to expected location: `cp data/ipa/ipa.dict ./`

### Dictionary Corruption

**Error**: "Dictionary file size invalid" or loading crashes

**Solutions**:
1. Re-download dictionary: `git checkout data/ipa/ipa.dict`
2. Verify file integrity: `file ipa.dict` (should show "data")
3. Check available disk space
4. Try system-wide installation location

### Memory Issues

**Error**: AddressSanitizer errors or segmentation faults during loading

**Solutions**:
1. Fallback dictionary will be used automatically
2. Increase available memory
3. Check for conflicting libraries
4. Report issue with specific error details

### Permission Issues

**Error**: Cannot read dictionary file

**Solutions**:
```bash
# Fix file permissions
chmod 644 ipa.dict

# For system installation
sudo chown root:root /usr/share/kagome/ipa.dict
sudo chmod 644 /usr/share/kagome/ipa.dict

# For Rspamd integration
sudo chown rspamd:rspamd /path/to/ipa.dict
chmod 644 /path/to/ipa.dict
```

## API Usage

### Dictionary Information

```cpp
#include "kagome/dict/dict.hpp"

// Create dictionary
auto dict = kagome::dict::factory::create_ipa_dict();

// Check if dictionary is loaded
if (dict) {
    std::cout << "Dictionary loaded successfully\n";
    
    // Get dictionary statistics (if available)
    // auto stats = dict->get_stats();
} else {
    std::cout << "Dictionary loading failed\n";
}
```

### Dictionary Path Configuration

```cpp
#include "kagome/dict/dict.hpp"

// Use specific dictionary path (planned feature)
// auto dict = kagome::dict::factory::create_ipa_dict("/custom/path/ipa.dict");
```

## Dictionary Sources and Credits

### Original Sources

- **IPAdic**: Original morphological dictionary project
- **MeCab**: Reference implementation and format specification
- **Kagome (Go)**: Dictionary format and processing algorithms

### Acknowledgments

- Nara Institute of Science and Technology (NAIST) for IPAdic
- Information-technology Promotion Agency (IPA) for ongoing maintenance
- [ikawaha](https://github.com/ikawaha) for the original kagome Go implementation
- MeCab project for dictionary format specification

## See Also

- [README.md](README.md) - Main project documentation
- [RSPAMD_INTEGRATION.md](RSPAMD_INTEGRATION.md) - Rspamd-specific setup
- [CONTRIBUTING.md](CONTRIBUTING.md) - Development guidelines
- [Original kagome project](https://github.com/ikawaha/kagome) - Go implementation