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
#include <atomic>

#define TEST_DATA_DIR "testdata"

// Constants for consistent formatting
constexpr int TITLE_BOX_WIDTH = 40;

// Test result structure for better reporting
struct test_result {
	std::string name;
	bool passed;
	std::chrono::milliseconds duration;
	std::string details;
};

// Game Boy CPU frequency: 1.048576 MHz
constexpr double GB_CPU_FREQUENCY_HZ = 1048576.0;

// Utility functions for enhanced output
namespace test_output {
	// Function declarations (implementations in yahbog-tests.cpp)
	std::string repeat_unicode(const std::string& str, size_t count);
	double cycles_to_real_time_ms(std::size_t cycles);
	std::string format_real_time(double real_time_ms);
	void print_header(const std::string& title);
	void print_test_result(const test_result& result);
	void print_progress(int current, int total, const std::string& current_test = "");
	void print_suite_summary(const std::string& suite_name, int passed, int failed, 
						   std::chrono::milliseconds total_time, const std::string& extra_stats = "");
	void print_summary(const std::vector<test_result>& results);
}

// Common test suite functionality
namespace test_suite {
	// Shared emulator execution result
	struct emulator_result {
		bool passed;
		std::string failure_reason;
		std::size_t cycles_executed;
		std::chrono::milliseconds execution_time;
	};

	// Shared emulator execution functions
	std::unique_ptr<yahbog::emulator> create_emulator(yahbog::hardware_mode mode);
	emulator_result run_rom_with_serial_check(const std::filesystem::path& rom_path, yahbog::hardware_mode mode);
	
	void write_framebuffer_to_console(const yahbog::emulator& emulator);
	void write_framebuffer_to_file(const yahbog::emulator& emulator, const std::filesystem::path& path);

	// Helper class for managing test suite execution and reporting
	class test_suite_runner {
	private:
		std::string suite_name;
		std::chrono::high_resolution_clock::time_point start_time;
		std::vector<test_result> results;
		std::string custom_extra_stats;
		bool all_passed = true;

	public:
		test_suite_runner(const std::string& name);
		void start();
		void finish();
		void add_result(const std::string& name, bool passed, 
					   std::chrono::milliseconds duration, const std::string& details = "");
		void add_extra_stats(const std::string& stats);
		bool passed() const;
		void print_info(const std::string& message);
		void print_test_line(const std::string& name, bool passed, 
						   std::chrono::milliseconds duration, const std::string& details = "");
	};

	// Helper class for progress tracking with threading
	class progress_tracker {
	private:
		int total_count;
		std::atomic<int> current_count;
		std::atomic<bool> done;
		std::thread progress_thread;

	public:
		progress_tracker(int total);
		void start(const std::string& initial_message = "Running...");
		void update(int count);
		void finish();
	};
}

bool run_single_step_tests();
bool run_blargg_cpu_instrs();
bool run_blargg_general();
bool run_mooneye();
bool run_gbmicrotest();