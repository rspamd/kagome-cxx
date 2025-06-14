#include <iostream>

// Declare the test function
void run_all_tests();

int main() {
    try {
        run_all_tests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Tests failed: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Tests failed with unknown exception\n";
        return 1;
    }
} 