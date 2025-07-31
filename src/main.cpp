#include <iostream>
#include <string>
#include <vector>
#include "kagome/common/format.hpp"

#include "kagome/tokenizer/tokenizer.hpp"
#include "kagome/dict/dict.hpp"

void print_usage() {
    std::cout << "Japanese Morphological Analyzer -- C++ Implementation\n";
    std::cout << "Usage: kagome_main [options] [text]\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help     Show this help message\n";
    std::cout << "  -m, --mode     Tokenization mode (normal|search|extended)\n";
    std::cout << "  -w, --wakati   Wakati mode (surface forms only)\n";
    std::cout << "  -j, --json     Output in JSON format\n";
    std::cout << "  --omit-bos-eos Omit BOS/EOS tokens\n";
    std::cout << "\nIf no text is provided, interactive mode is started.\n";
}

void print_tokens_table(const std::vector<kagome::tokenizer::Token>& tokens) {
    for (const auto& token : tokens) {
        // Skip tokens with empty surface, but accept Dummy tokens with valid text
        if (token.surface().empty()) {
            continue;
        }
        
        auto features = token.features();
        std::string features_str;
        if (!features.empty()) {
            features_str = features[0];
            for (std::size_t i = 1; i < features.size(); ++i) {
                features_str += "," + features[i];
            }
        }
        
        std::cout << kagome::format("{}\t{}\n", token.surface(), features_str);
    }
    std::cout << "EOS\n";
}

void print_tokens_json(const std::vector<kagome::tokenizer::Token>& tokens) {
    std::cout << "[\n";
    bool first = true;
    
    for (const auto& token : tokens) {
        // Skip tokens with empty surface, but accept Dummy tokens with valid text
        if (token.surface().empty()) {
            continue;
        }
        
        if (!first) {
            std::cout << ",\n";
        }
        first = false;
        
        auto data = token.to_token_data();
        std::cout << "  {\n";
        std::cout << kagome::format("    \"id\": {},\n", data.id);
        std::cout << kagome::format("    \"start\": {},\n", data.start);
        std::cout << kagome::format("    \"end\": {},\n", data.end);
        std::cout << kagome::format("    \"surface\": \"{}\",\n", data.surface);
        std::cout << kagome::format("    \"class\": \"{}\",\n", data.token_class);
        
        std::cout << "    \"pos\": [";
        for (std::size_t i = 0; i < data.pos.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << kagome::format("\"{}\"", data.pos[i]);
        }
        std::cout << "],\n";
        
        std::cout << kagome::format("    \"base_form\": \"{}\",\n", data.base_form);
        std::cout << kagome::format("    \"reading\": \"{}\",\n", data.reading);
        std::cout << kagome::format("    \"pronunciation\": \"{}\",\n", data.pronunciation);
        
        std::cout << "    \"features\": [";
        for (std::size_t i = 0; i < data.features.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << kagome::format("\"{}\"", data.features[i]);
        }
        std::cout << "]\n";
        
        std::cout << "  }";
    }
    
    std::cout << "\n]\n";
}

void print_wakati(const std::vector<kagome::tokenizer::Token>& tokens) {
    std::cout << "[";
    bool first = true;
    
    for (const auto& token : tokens) {
        if (token.token_class() == kagome::tokenizer::TokenClass::Dummy || 
            token.surface().empty()) {
            continue;
        }
        
        if (!first) {
            std::cout << " ";
        }
        first = false;
        std::cout << token.surface();
    }
    
    std::cout << "]\n";
}

void interactive_mode(kagome::tokenizer::Tokenizer& tokenizer, 
                     kagome::tokenizer::TokenizeMode mode, 
                     bool wakati_mode, bool json_mode) {
    std::string line;
    std::cout << "Enter Japanese text (Ctrl+C to exit):\n";
    
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }
        
        if (wakati_mode) {
            auto wakati_results = tokenizer.wakati(line);
            std::cout << "[";
            bool first = true;
            for (const auto& surface : wakati_results) {
                if (!first) std::cout << " ";
                first = false;
                std::cout << surface;
            }
            std::cout << "]\n";
        } else if (json_mode) {
            auto tokens = tokenizer.analyze(line, mode);
            print_tokens_json(tokens);
        } else {
            auto tokens = tokenizer.analyze(line, mode);
            print_tokens_table(tokens);
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        kagome::tokenizer::TokenizeMode mode = kagome::tokenizer::TokenizeMode::Normal;
        bool wakati_mode = false;
        bool json_mode = false;
        bool omit_bos_eos = false;
        std::string input_text;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "-h" || arg == "--help") {
                print_usage();
                return 0;
            } else if (arg == "-m" || arg == "--mode") {
                if (i + 1 < argc) {
                    std::string mode_str = argv[++i];
                    if (mode_str == "normal") {
                        mode = kagome::tokenizer::TokenizeMode::Normal;
                    } else if (mode_str == "search") {
                        mode = kagome::tokenizer::TokenizeMode::Search;
                    } else if (mode_str == "extended") {
                        mode = kagome::tokenizer::TokenizeMode::Extended;
                    } else {
                        std::cerr << "Invalid mode: " << mode_str << "\n";
                        return 1;
                    }
                } else {
                    std::cerr << "Missing mode argument\n";
                    return 1;
                }
            } else if (arg == "-w" || arg == "--wakati") {
                wakati_mode = true;
            } else if (arg == "-j" || arg == "--json") {
                json_mode = true;
            } else if (arg == "--omit-bos-eos") {
                omit_bos_eos = true;
            } else if (arg.substr(0, 1) != "-") {
                // Assume this is input text
                input_text = arg;
            } else {
                std::cerr << "Unknown option: " << arg << "\n";
                return 1;
            }
        }
        
        // Create dictionary
        auto dict = kagome::dict::factory::create_ipa_dict();
        if (!dict) {
            std::cerr << "Failed to create dictionary\n";
            return 1;
        }
        
        // Create tokenizer
        kagome::tokenizer::TokenizerConfig config;
        config.omit_bos_eos = omit_bos_eos;
        config.default_mode = mode;
        
        kagome::tokenizer::Tokenizer tokenizer(dict, config);
        
        if (input_text.empty()) {
            // Interactive mode
            interactive_mode(tokenizer, mode, wakati_mode, json_mode);
        } else {
            // Process single input
            if (wakati_mode) {
                auto wakati_results = tokenizer.wakati(input_text);
                std::cout << "[";
                bool first = true;
                for (const auto& surface : wakati_results) {
                    if (!first) std::cout << " ";
                    first = false;
                    std::cout << surface;
                }
                std::cout << "]\n";
            } else if (json_mode) {
                auto tokens = tokenizer.analyze(input_text, mode);
                print_tokens_json(tokens);
            } else {
                auto tokens = tokenizer.analyze(input_text, mode);
                print_tokens_table(tokens);
            }
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
} 