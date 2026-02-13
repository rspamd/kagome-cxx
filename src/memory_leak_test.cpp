/**
 * Memory Leak Test for Kagome-cxx
 * 
 * This program simulates Rspamd's usage pattern to demonstrate
 * the memory leak in the static node pool and verify the fix.
 * 
 * Build:
 *   g++ -std=c++20 -I../include memory_leak_test.cpp -o memory_leak_test \
 *       -L../build -lkagome_c_api -lkagome_cpp
 * 
 * Run:
 *   ./memory_leak_test
 * 
 * Expected output:
 *   - Before fix: Memory usage grows continuously
 *   - After fix: Memory usage stabilizes after initial allocation
 */

#include "kagome/c_api/kagome_c_api.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/resource.h>
#ifdef __APPLE__
#include <mach/mach.h>
#include <malloc/malloc.h>
#endif

// Get current memory usage in KB
long get_memory_usage() {
#ifdef __APPLE__
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    kern_return_t kr = task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                                 reinterpret_cast<task_info_t>(&info), &count);

    if (kr == KERN_SUCCESS) {
        return static_cast<long>(info.resident_size / 1024);
    }

    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return static_cast<long>(usage.ru_maxrss / 1024);
    }

    return 0;
#else
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
#endif
}

#ifdef __APPLE__
struct MallocZoneUsage {
    std::string name;
    size_t size_in_use;
};

std::vector<MallocZoneUsage> get_malloc_zone_usages() {
    std::vector<MallocZoneUsage> usages;

    vm_address_t *zones = nullptr;
    unsigned int zone_count = 0;
    if (malloc_get_all_zones(mach_task_self(), nullptr, &zones, &zone_count) == KERN_SUCCESS && zones != nullptr) {
        usages.reserve(zone_count);
        for (unsigned int i = 0; i < zone_count; ++i) {
            auto *zone = reinterpret_cast<malloc_zone_t *>(zones[i]);
            if (!zone) {
                continue;
            }

            malloc_statistics_t stats = {0};
            malloc_zone_statistics(zone, &stats);

            const char *zone_name = zone->zone_name;
            usages.push_back({zone_name ? zone_name : "(unnamed)", stats.size_in_use});
        }
    }

    if (usages.empty()) {
        if (malloc_zone_t *default_zone = malloc_default_zone()) {
            malloc_statistics_t stats = {0};
            malloc_zone_statistics(default_zone, &stats);
            usages.push_back({"default", stats.size_in_use});
        }
    }

    std::sort(usages.begin(), usages.end(), [](const auto &lhs, const auto &rhs) {
        return lhs.size_in_use > rhs.size_in_use;
    });

    return usages;
}

size_t total_malloc_in_use(const std::vector<MallocZoneUsage> &usages) {
    return std::accumulate(usages.begin(), usages.end(), size_t{0}, [](size_t acc, const auto &entry) {
        return acc + entry.size_in_use;
    });
}

size_t get_malloc_in_use() {
    return total_malloc_in_use(get_malloc_zone_usages());
}

void print_malloc_zone_breakdown(const std::vector<MallocZoneUsage> &usages, size_t indent_spaces = 11, size_t max_entries = 5) {
    if (usages.empty()) {
        return;
    }

    std::string indent(indent_spaces, ' ');
    std::cout << indent << "zone breakdown:";
    size_t limit = std::min(max_entries, usages.size());
    for (size_t idx = 0; idx < limit; ++idx) {
        const auto &zone = usages[idx];
        std::cout << " [" << zone.name << ": " << (zone.size_in_use / 1024) << " KB]";
    }
    if (usages.size() > max_entries) {
        std::cout << " ...";
    }
    std::cout << std::endl;
}

void dump_malloc_zone_details(const char *indent_prefix, const char *target_zone) {
    vm_address_t *zones = nullptr;
    unsigned int zone_count = 0;
    if (malloc_get_all_zones(mach_task_self(), nullptr, &zones, &zone_count) != KERN_SUCCESS || zones == nullptr) {
        std::cout << indent_prefix << "malloc_get_all_zones failed" << std::endl;
        return;
    }

    bool matched = false;
    for (unsigned int i = 0; i < zone_count; ++i) {
        auto *zone = reinterpret_cast<malloc_zone_t *>(zones[i]);
        if (!zone) {
            continue;
        }

        const char *zone_name = zone->zone_name ? zone->zone_name : "(unnamed)";
        if (target_zone && std::strlen(target_zone) > 0) {
            if (std::strcmp(zone_name, target_zone) != 0) {
                continue;
            }
        }

        if (!matched) {
            std::cout << indent_prefix << "Detailed malloc_zone_print for zone(s):" << std::endl;
            matched = true;
        }

        std::cout << indent_prefix << "- " << zone_name << std::endl;
        malloc_zone_print(zone, true);
    }

    if (!matched) {
        std::cout << indent_prefix << "No malloc zone named '" << (target_zone ? target_zone : "") << "' found" << std::endl;
    }
}

void maybe_dump_zone_details(const char *indent_prefix) {
    const char *env = std::getenv("KAGOME_DUMP_MALLOC_ZONE");
    if (!env) {
        return;
    }

    const char *target_zone = (std::strlen(env) > 0) ? env : "MallocHelperZone";
    dump_malloc_zone_details(indent_prefix, target_zone);
}
#endif

// Test text samples of various sizes
std::vector<std::string> test_texts = {
    // Small text (~10 tokens)
    "これは日本語のテストです。",

    // Medium text (~50 tokens)
    "私は昨日、友達と一緒に東京の新しいレストランに行きました。"
    "そこでとても美味しい料理を食べました。特に寿司が最高でした。"
    "また行きたいと思います。",

    // Large text (~200 tokens)
    "日本の四季は非常に美しいです。春には桜が咲き、夏には海で泳ぎ、"
    "秋には紅葉を楽しみ、冬には雪景色を見ることができます。"
    "これらの季節の変化は日本の文化と深く結びついています。"
    "例えば、春の花見は日本人にとって重要な行事です。"
    "家族や友人と一緒に公園に行き、桜の下でお弁当を食べます。"
    "夏には花火大会があり、浴衣を着て夜空に咲く花火を楽しみます。"
    "秋には収穫祭があり、新米や果物を味わいます。"
    "冬にはお正月があり、神社に初詣に行きます。"
    "このように、日本の四季は人々の生活に彩りを添えています。",

    // Very large text (~500 tokens)
    "日本語は世界で最も複雑な言語の一つと言われています。"
    "それは、ひらがな、カタカナ、そして漢字という三つの異なる文字体系を使用しているためです。"
    "ひらがなは日本固有の文字で、主に助詞や動詞の活用語尾などに使われます。"
    "カタカナは外来語や擬音語、強調したい言葉などに使われます。"
    "漢字は中国から伝わった文字で、名詞や動詞の語幹などに使われます。"
    "これらの文字を適切に使い分けることが、日本語を正しく書く上で重要です。"
    "また、日本語には敬語という独特の表現方法があります。"
    "敬語には尊敬語、謙譲語、丁寧語の三種類があり、相手や状況に応じて使い分けます。"
    "これは日本の社会における上下関係や礼儀を反映したものです。"
    "さらに、日本語には多くの方言があります。"
    "北は北海道から南は沖縄まで、地域によって言葉の使い方や発音が大きく異なります。"
    "標準語は主に東京の言葉をベースにしていますが、"
    "大阪弁や博多弁など、独特の魅力を持つ方言も多く存在します。"
    "日本語の文法も独特です。主語、目的語、動詞の順序は英語とは逆で、"
    "「私は本を読む」という文は英語では「I read a book」となりますが、"
    "語順が「主語-目的語-動詞」となっています。"
    "このような特徴を持つ日本語は、学習者にとって難しい言語の一つですが、"
    "その美しさと深さは多くの人々を魅了し続けています。"};

void run_tokenization_test() {
    char error_buf[1024];
    
    std::cout << "=== Kagome Memory Leak Test ===" << std::endl;
    std::cout << std::endl;
    
    // Initialize tokenizer
    std::cout << "Initializing tokenizer..." << std::endl;
    if (kagome_init(nullptr, error_buf, sizeof(error_buf)) != 0) {
        std::cerr << "Failed to initialize: " << error_buf << std::endl;
        return;
    }
    
    long initial_memory = get_memory_usage();
    std::cout << "Initial memory: " << initial_memory << " KB" << std::endl;
    std::cout << std::endl;
    
    // Run tokenization cycles
    const int NUM_CYCLES = 10;
    const int ITERATIONS_PER_CYCLE = 100;
    
    std::cout << "Running " << NUM_CYCLES << " cycles of " 
              << ITERATIONS_PER_CYCLE << " tokenizations each..." << std::endl;
    std::cout << std::endl;
    
    for (int cycle = 0; cycle < NUM_CYCLES; ++cycle) {
        // Tokenize many times to simulate Rspamd processing emails
        for (int i = 0; i < ITERATIONS_PER_CYCLE; ++i) {
            // Use different text sizes to simulate varied email content
            const std::string &text = test_texts[i % test_texts.size()];
            
            rspamd_words_t result;
            memset(&result, 0, sizeof(result));
            
            int ret = kagome_tokenize(text.c_str(), text.length(), &result);
            if (ret == 0) {
                kagome_cleanup_result(&result);
            }
        }
        
        long current_memory = get_memory_usage();
        long memory_growth = current_memory - initial_memory;
        
        std::cout << "Cycle " << (cycle + 1) << ": Memory = " 
                  << current_memory << " KB (+" << memory_growth << " KB)" 
                  << std::endl;
#ifdef __APPLE__
        auto zones = get_malloc_zone_usages();
        size_t in_use = total_malloc_in_use(zones);
        if (!zones.empty()) {
            std::cout << "           malloc in-use: " << (in_use / 1024) << " KB" << std::endl;
            print_malloc_zone_breakdown(zones);
            maybe_dump_zone_details("           ");
        }
#endif
        
        // Small delay to allow memory statistics to update
        usleep(100000);  // 100ms
    }
    
    std::cout << std::endl;
    std::cout << "Deinitializing tokenizer (this should free pool memory)..." << std::endl;
    kagome_deinit();
    
    // Give system time to report memory changes
    usleep(500000);  // 500ms
    
    long final_memory = get_memory_usage();
    long total_growth = final_memory - initial_memory;
    
    std::cout << "Final memory: " << final_memory << " KB" << std::endl;
#ifdef __APPLE__
    auto final_zones = get_malloc_zone_usages();
    size_t final_in_use = total_malloc_in_use(final_zones);
    if (!final_zones.empty()) {
        std::cout << "Malloc in-use after deinit: " << (final_in_use / 1024) << " KB" << std::endl;
        print_malloc_zone_breakdown(final_zones);
        maybe_dump_zone_details("           ");
    }
#endif
    std::cout << "Total growth: " << total_growth << " KB" << std::endl;
    std::cout << std::endl;
    
    // Analysis
    if (total_growth > 5000) {  // More than 5MB growth
        std::cout << "❌ MEMORY LEAK DETECTED!" << std::endl;
        std::cout << "   Memory grew by more than 5MB during the test." << std::endl;
        std::cout << "   This indicates the node pool is not being freed properly." << std::endl;
    } else if (total_growth > 2000) {  // 2-5MB growth
        std::cout << "⚠️  WARNING: Possible memory leak" << std::endl;
        std::cout << "   Memory grew by " << total_growth << " KB." << std::endl;
        std::cout << "   This might be normal startup allocation, but monitor closely." << std::endl;
    } else {
        std::cout << "✅ PASS: Memory usage is stable" << std::endl;
        std::cout << "   Memory growth is within acceptable limits." << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "=== Test Complete ===" << std::endl;
}

void run_reinit_test() {
    std::cout << std::endl;
    std::cout << "=== Re-initialization Test ===" << std::endl;
    std::cout << "Testing multiple init/deinit cycles..." << std::endl;
    std::cout << std::endl;
    
    long start_memory = get_memory_usage();
    
    for (int i = 0; i < 5; ++i) {
        char error_buf[1024];
        
        // Initialize
        if (kagome_init(nullptr, error_buf, sizeof(error_buf)) != 0) {
            std::cerr << "Failed to initialize: " << error_buf << std::endl;
            return;
        }
        
        // Tokenize some text
        for (int j = 0; j < 50; ++j) {
            const std::string &text = test_texts[j % test_texts.size()];
            rspamd_words_t result;
            memset(&result, 0, sizeof(result));
            
            int ret = kagome_tokenize(text.c_str(), text.length(), &result);
            if (ret == 0) {
                kagome_cleanup_result(&result);
            }
        }
        
        // Deinitialize
        kagome_deinit();
        
        long current_memory = get_memory_usage();
        std::cout << "Cycle " << (i + 1) << ": Memory = " << current_memory 
                  << " KB (+" << (current_memory - start_memory) << " KB)" 
                  << std::endl;
#ifdef __APPLE__
        auto zones = get_malloc_zone_usages();
        size_t in_use = total_malloc_in_use(zones);
        if (!zones.empty()) {
            std::cout << "           malloc in-use: " << (in_use / 1024) << " KB" << std::endl;
            print_malloc_zone_breakdown(zones);
            maybe_dump_zone_details("           ");
        }
#endif
        
        usleep(100000);
    }
    
    long end_memory = get_memory_usage();
    long growth = end_memory - start_memory;
    
    std::cout << std::endl;
    std::cout << "Total growth after 5 init/deinit cycles: " << growth << " KB" << std::endl;
#ifdef __APPLE__
    auto end_zones = get_malloc_zone_usages();
    size_t end_in_use = total_malloc_in_use(end_zones);
    if (!end_zones.empty()) {
        std::cout << "Malloc in-use after cycles: " << (end_in_use / 1024) << " KB" << std::endl;
        print_malloc_zone_breakdown(end_zones);
        maybe_dump_zone_details("           ");
    }
#endif
    
    if (growth > 1000) {
        std::cout << "❌ LEAK: Memory is not being freed between cycles!" << std::endl;
    } else {
        std::cout << "✅ PASS: Memory is properly freed between cycles" << std::endl;
    }
    
    std::cout << std::endl;
}

int main()
{
	// Run the main tokenization test
	run_tokenization_test();

	// Run the re-initialization test
	run_reinit_test();

	return 0;
}
