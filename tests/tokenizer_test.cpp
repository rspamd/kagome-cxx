#include <iostream>
#include <cassert>
#include <string>
#include <vector>

#include "kagome/tokenizer/tokenizer.hpp"
#include "kagome/dict/dict.hpp"

void test_basic_tokenization() {
    std::cout << "Testing basic tokenization...\n";
    
    // Create dictionary
    auto dict = kagome::dict::factory::create_ipa_dict();
    assert(dict != nullptr);
    
    // Create tokenizer
    kagome::tokenizer::TokenizerConfig config;
    config.omit_bos_eos = true;
    
    kagome::tokenizer::Tokenizer tokenizer(dict, config);
    
    // Test tokenization
    std::string test_text = "すもも";
    auto tokens = tokenizer.tokenize(test_text);
    
    assert(!tokens.empty());
    std::cout << "✓ Basic tokenization test passed\n";
    
    // Print results for verification
    for (const auto& token : tokens) {
        auto features = token.features();
        std::string features_str;
        if (!features.empty()) {
            features_str = features[0];
            for (std::size_t i = 1; i < features.size(); ++i) {
                features_str += "," + features[i];
            }
        }
        std::cout << "  " << token.surface() << "\t" << features_str << "\n";
    }
}

void test_wakati_mode() {
    std::cout << "Testing wakati mode...\n";
    
    auto dict = kagome::dict::factory::create_ipa_dict();
    kagome::tokenizer::TokenizerConfig config;
    config.omit_bos_eos = true;
    kagome::tokenizer::Tokenizer tokenizer(dict, config);
    
    std::string test_text = "すもも";
    auto tokens = tokenizer.wakati(test_text);
    
    assert(!tokens.empty());
    std::cout << "✓ Wakati mode test passed\n";
}

void test_different_modes() {
    std::cout << "Testing different tokenization modes...\n";
    
    auto dict = kagome::dict::factory::create_ipa_dict();
    kagome::tokenizer::TokenizerConfig config;
    config.omit_bos_eos = true;
    kagome::tokenizer::Tokenizer tokenizer(dict, config);
    
    std::string test_text = "すもも";
    
    // Test Normal mode
    auto normal_tokens = tokenizer.analyze(test_text, kagome::tokenizer::TokenizeMode::Normal);
    assert(!normal_tokens.empty());
    
    // Test Search mode
    auto search_tokens = tokenizer.analyze(test_text, kagome::tokenizer::TokenizeMode::Search);
    assert(!search_tokens.empty());
    
    // Test Extended mode
    auto extended_tokens = tokenizer.analyze(test_text, kagome::tokenizer::TokenizeMode::Extended);
    assert(!extended_tokens.empty());
    
    std::cout << "✓ Different modes test passed\n";
}

void test_token_features() {
    std::cout << "Testing token features...\n";
    
    auto dict = kagome::dict::factory::create_ipa_dict();
    kagome::tokenizer::TokenizerConfig config;
    config.omit_bos_eos = true;
    kagome::tokenizer::Tokenizer tokenizer(dict, config);
    
    std::string test_text = "すもも";
    auto tokens = tokenizer.tokenize(test_text);
    
    for (const auto& token : tokens) {
        if (token.token_class() == kagome::tokenizer::TokenClass::Dummy) {
            continue;
        }
        
        // Test various feature methods
        auto features = token.features();
        auto pos = token.pos();
        auto base_form = token.base_form();
        auto reading = token.reading();
        auto pronunciation = token.pronunciation();
        
        // These should not crash
        assert(!features.empty() || token.token_class() == kagome::tokenizer::TokenClass::Unknown);
        
        // Test TokenData conversion
        auto data = token.to_token_data();
        assert(data.surface == token.surface());
        assert(data.start == token.start());
        assert(data.end == token.end());
    }
    
    std::cout << "✓ Token features test passed\n";
}

void run_all_tests() {
    std::cout << "Running kagome C++ tokenizer tests...\n\n";
    
    try {
        test_basic_tokenization();
        test_wakati_mode();
        test_different_modes();
        test_token_features();
        
        std::cout << "\n✓ All tests passed!\n";
    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << "\n";
        throw;
    }
} 