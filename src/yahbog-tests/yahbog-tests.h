#pragma once

#include <yahbog.h>

#include <nlohmann/json.hpp>
#include <termcolor/termcolor.hpp>

#include <print>
#include <iostream>
#include <zip.h>
#include <filesystem>

#include <memory>
#include <thread>
#include <span>
#include <unordered_map>
#include <set>
#include <chrono>
#include <iomanip>
#include <sstream>

#define TEST_DATA_DIR "testdata"

// Constants for consistent formatting
constexpr int TITLE_BOX_WIDTH = 40;

// Test result structure for better reporting
struct TestResult {
    std::string name;
    bool passed;
    std::chrono::milliseconds duration;
    std::string details;
};

// Game Boy CPU frequency: 1.048576 MHz
constexpr double GB_CPU_FREQUENCY_HZ = 1048576.0;

// Utility functions for enhanced output
namespace TestOutput {
    // Utility function to repeat Unicode strings
    inline std::string repeat_unicode(const std::string& str, size_t count) {
        std::string result;
        result.reserve(str.length() * count);
        for (size_t i = 0; i < count; ++i) {
            result += str;
        }
        return result;
    }

    inline double cycles_to_real_time_ms(std::size_t cycles) {
        return (static_cast<double>(cycles) / GB_CPU_FREQUENCY_HZ) * 1000.0;
    }

    inline std::string format_real_time(double real_time_ms) {
        if (real_time_ms < 1.0) {
            return std::format("{:.2f}μs", real_time_ms * 1000.0);
        } else if (real_time_ms < 1000.0) {
            return std::format("{:.2f}ms", real_time_ms);
        } else {
            return std::format("{:.2f}s", real_time_ms / 1000.0);
        }
    }

    inline void print_header(const std::string& title) {
        std::cout << termcolor::cyan << termcolor::bold;
        
        // Top border
        std::cout << "\n┌" << repeat_unicode("─", TITLE_BOX_WIDTH) << "┐\n";
        
        // Middle with centered text
        int title_len = static_cast<int>(title.length());
        int padding_total = TITLE_BOX_WIDTH - title_len;
        int left_padding = padding_total / 2;
        int right_padding = padding_total - left_padding;
        
        std::cout << "│" << std::string(left_padding, ' ') << title << std::string(right_padding, ' ') << "│\n";
        
        // Bottom border
        std::cout << "└" << repeat_unicode("─", TITLE_BOX_WIDTH) << "┘" << termcolor::reset << "\n";
    }

    inline void print_test_result(const TestResult& result) {
        if (result.passed) {
            std::cout << termcolor::green << "✅ " << termcolor::reset;
        } else {
            std::cout << termcolor::red << "❌ " << termcolor::reset;
        }
        std::cout << std::left << std::setw(40) << result.name;
        std::cout << termcolor::yellow << std::right << std::setw(8) << result.duration.count() << "ms" << termcolor::reset;
        if (!result.details.empty()) {
            std::cout << " " << termcolor::dark << result.details << termcolor::reset;
        }
        std::cout << "\n";
    }

    inline void print_progress(int current, int total, const std::string& current_test = "") {
        float percentage = (float)current / total * 100;
        int bar_width = 40;
        int filled = (int)(percentage / 100 * bar_width);
        
        std::cout << "\r" << termcolor::cyan << "   Progress: [";
        for (int i = 0; i < bar_width; i++) {
            if (i < filled) std::cout << "█";
            else std::cout << "░";
        }
        std::cout << "] " << std::fixed << std::setprecision(1) << percentage << "% ";
        std::cout << "(" << current << "/" << total << ")" << termcolor::reset;
        if (!current_test.empty()) {
            std::cout << " " << termcolor::dark << current_test << termcolor::reset;
        }
        std::cout.flush();
    }

    inline void print_summary(const std::vector<TestResult>& results) {
        int passed = 0, failed = 0;
        std::chrono::milliseconds total_time{0};
        
        for (const auto& result : results) {
            if (result.passed) passed++;
            else failed++;
            total_time += result.duration;
        }

        std::cout << "\n" << termcolor::cyan << termcolor::bold << "Summary:" << termcolor::reset << "\n";
        std::cout << "  " << termcolor::green << "✅ Passed: " << passed << termcolor::reset << "\n";
        if (failed > 0) {
            std::cout << "  " << termcolor::red << "❌ Failed: " << failed << termcolor::reset << "\n";
        }
        std::cout << "  " << termcolor::yellow << "⏱️  Total time: " << total_time.count() << "ms" << termcolor::reset << "\n";
        
        if (failed == 0) {
            std::cout << "\n" << termcolor::green << termcolor::bold << "🎉 All tests passed! 🎉" << termcolor::reset << "\n";
        } else {
            std::cout << "\n" << termcolor::red << termcolor::bold << "💥 Some tests failed!" << termcolor::reset << "\n";
        }
    }
}

bool run_single_step_tests();
bool run_blargg_cpu_instrs();
bool run_blargg_general();