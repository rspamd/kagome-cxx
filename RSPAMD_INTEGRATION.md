# Kagome Japanese Tokenizer Integration with Rspamd

This document describes how to integrate the Kagome Japanese tokenizer with Rspamd for improved Japanese text analysis.

## Quick Setup

### 1. Build the Kagome Tokenizer Plugin

```bash
cd kagome-cxx
mkdir -p build && cd build
cmake ..
make -j4
```

This will create `kagome_rspamd_tokenizer.dylib` (macOS) or `kagome_rspamd_tokenizer.so` (Linux).

### 2. Install the Dictionary

The tokenizer requires a Japanese dictionary file. You have several options:

#### Option A: Place next to the library (Recommended)
```bash
# Copy the dictionary to the same directory as the plugin
cp ../data/ipa/ipa.dict ./ipa.dict
```

#### Option B: Install to system location
```bash
# Create system directory
sudo mkdir -p /usr/local/share/kagome/

# Copy dictionary
sudo cp ../data/ipa/ipa.dict /usr/local/share/kagome/ipa.dict
```

#### Option C: Alternative locations
The tokenizer will search these locations automatically:
- `./ipa.dict` (next to the library) - **Recommended**
- `./data/ipa/ipa.dict`
- `../data/ipa/ipa.dict` 
- `../../data/ipa/ipa.dict`
- `/usr/local/share/kagome/ipa.dict`
- `/usr/share/kagome/ipa.dict`
- `/opt/kagome/ipa.dict`

### 3. Configure Rspamd

Add the following to your rspamd configuration (e.g., `/etc/rspamd/local.d/options.inc`):

```ucl
# Enable custom tokenizers
custom_tokenizers {
    # Japanese tokenizer using Kagome
    kagome {
        enabled = true;
        path = "/path/to/kagome_rspamd_tokenizer.so";  # Update this path
        priority = 60.0;  # Higher priority than default tokenizer
        
        # Optional: tokenizer-specific config
        config {
            # Currently, config is ignored by kagome tokenizer
            # Dictionary path is auto-detected
        }
    }
}
```

### 4. Restart Rspamd

```bash
sudo systemctl restart rspamd
# or
sudo service rspamd restart
```

## Configuration Options

### Library Path
Update the `path` to point to your compiled library:
- **Linux**: `/path/to/kagome_rspamd_tokenizer.so`
- **macOS**: `/path/to/kagome_rspamd_tokenizer.dylib`

### Priority
The `priority` determines the order of tokenizer detection:
- Higher values = checked first
- Default: 50.0
- Recommended for Japanese: 60.0+

### Enable/Disable
```ucl
kagome {
    enabled = false;  # Disable the tokenizer
}
```

## How It Works

1. **Language Detection**: When processing text, rspamd asks each tokenizer to detect the language
2. **Confidence Scoring**: Kagome returns confidence 0.3-0.95 for Japanese text, -1.0 for non-Japanese
3. **Tokenizer Selection**: The tokenizer with highest confidence above its threshold is used
4. **Fallback**: If no custom tokenizer matches, rspamd falls back to ICU word breaking

### Japanese Detection
Kagome detects Japanese text by looking for:
- Hiragana characters (ひらがな)
- Katakana characters (カタカナ) 
- Kanji/Han characters (漢字)

The confidence score is based on the ratio of Japanese characters to total characters.

## Troubleshooting

### Dictionary Loading Issues

**Problem**: Tokenizer fails to initialize with dictionary errors

**Solutions**:
1. **Check dictionary location**: Ensure `ipa.dict` is accessible to the tokenizer
   ```bash
   # Check if file exists next to library
   ls -la /path/to/kagome_rspamd_tokenizer.so
   ls -la /path/to/ipa.dict
   ```

2. **Check file permissions**: Ensure rspamd can read the dictionary
   ```bash
   sudo chown rspamd:rspamd /path/to/ipa.dict
   sudo chmod 644 /path/to/ipa.dict
   ```

3. **Use fallback dictionary**: If the main dictionary is corrupted, the tokenizer will use a minimal fallback

**Error Messages**:
- `"Dictionary file not found"` → Copy `ipa.dict` to the correct location
- `"Dictionary file size invalid"` → Dictionary file may be corrupted, re-copy from source
- `"Warning: Using fallback dictionary"` → Main dictionary not found, basic functionality available

### Library Loading Issues

**Problem**: `"cannot load tokenizer ... from ..."`

**Solutions**:
1. **Check library path**: Verify the path in rspamd config matches the actual file location
2. **Check file permissions**: Ensure rspamd can read the library
   ```bash
   sudo chmod 755 /path/to/kagome_rspamd_tokenizer.so
   ```
3. **Check dependencies**: Ensure all required libraries are installed (ICU, fmt, etc.)

### Runtime Issues

**Problem**: Japanese text not being tokenized correctly

**Solutions**:
1. **Check detection**: Enable debug logging to see if Japanese text is being detected
2. **Verify priority**: Ensure kagome has higher priority than other tokenizers
3. **Test confidence**: Very short Japanese text may have low confidence scores

### Memory Issues (AddressSanitizer)

**Problem**: Crashes with AddressSanitizer when loading dictionary

**Solutions**:
1. **Use fallback dictionary**: The tokenizer will automatically fall back if main dictionary loading fails
2. **Check dictionary integrity**: Re-download or rebuild the dictionary file
3. **Update library**: Ensure you're using the latest version with improved error handling

## Testing

### Verify Installation
```bash
# Check if library loads
ldd /path/to/kagome_rspamd_tokenizer.so  # Linux
otool -L /path/to/kagome_rspamd_tokenizer.dylib  # macOS

# Test with rspamd
sudo rspamd -t  # Test configuration
```

### Debug Logging
Enable debug logging in rspamd config:
```ucl
logging {
    level = "info";
    # This will show tokenizer loading and detection messages
}
```

Look for messages like:
- `"starting to load custom tokenizer 'kagome'"`
- `"successfully loaded custom tokenizer 'kagome'"`
- `"using tokenizer 'kagome' for language hint 'ja'"`

### Test Japanese Text
Send an email with Japanese text through rspamd and check the logs for tokenizer activity.

## Performance Notes

- **Dictionary Loading**: Initial loading may take a few seconds
- **Memory Usage**: Expect ~50-100MB memory usage for the dictionary
- **Tokenization Speed**: Very fast once loaded (microseconds per text)
- **Fallback Mode**: If using fallback dictionary, functionality is limited but won't crash

## Advanced Configuration

### Custom Dictionary Paths
Currently, dictionary paths are auto-detected. In future versions, you may be able to specify custom paths in the config section.

### Multiple Languages
You can configure multiple tokenizers for different languages:
```ucl
custom_tokenizers {
    kagome {
        enabled = true;
        path = "/path/to/kagome_rspamd_tokenizer.so";
        priority = 60.0;
    }
    
    # Add other language tokenizers here
    chinese_tokenizer {
        enabled = true; 
        path = "/path/to/chinese_tokenizer.so";
        priority = 55.0;
    }
}
```

## Support

For issues and questions:
1. Check rspamd logs for error messages
2. Verify all installation steps were completed
3. Test with the fallback dictionary if main dictionary fails
4. Report specific error messages for debugging

The integration is designed to be robust - if the kagome tokenizer fails to load or crashes, rspamd will continue working with the default ICU tokenizer. 