#include "yahbog-tests.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


// Utility function implementations
namespace test_output {
	std::string repeat_unicode(const std::string& str, size_t count) {
		std::string result;
		result.reserve(str.length() * count);
		for (size_t i = 0; i < count; ++i) {
			result += str;
		}
		return result;
	}

	double cycles_to_real_time_ms(std::size_t cycles) {
		return (static_cast<double>(cycles) / GB_CPU_FREQUENCY_HZ) * 1000.0;
	}

	std::string format_real_time(double real_time_ms) {
		if (real_time_ms < 1.0) {
			return std::format("{:.2f}μs", real_time_ms * 1000.0);
		} else if (real_time_ms < 1000.0) {
			return std::format("{:.2f}ms", real_time_ms);
		} else {
			return std::format("{:.2f}s", real_time_ms / 1000.0);
		}
	}

	void print_header(const std::string& title) {
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

	void print_test_result(const test_result& result) {
		if (result.passed) {
			std::cout << termcolor::green << "✅ " << termcolor::reset;
		} else {
			std::cout << termcolor::red << "❌ " << termcolor::reset;
		}
		std::cout << std::left << std::setw(25) << result.name;
		std::cout << termcolor::yellow << std::right << std::setw(8) << result.duration.count() << "ms" << termcolor::reset;
		if (!result.details.empty()) {
			std::cout << " " << termcolor::dark << result.details << termcolor::reset;
		}
		std::cout << "\n";
	}

	void print_progress(int current, int total, const std::string& current_test) {
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

	void print_suite_summary(const std::string& suite_name, int passed, int failed, 
						   std::chrono::milliseconds total_time, const std::string& extra_stats) {
		std::cout << "\n";
		print_header(suite_name + " Results");

		std::cout << termcolor::cyan << termcolor::bold << "Summary:" << termcolor::reset << "\n";
		std::cout << "  " << termcolor::green << "✅ Passed: " << passed << termcolor::reset << "\n";
		if (failed > 0) {
			std::cout << "  " << termcolor::red << "❌ Failed: " << failed << termcolor::reset << "\n";
		}
		std::cout << "  " << termcolor::yellow << "⏱️  Total time: " << total_time.count() << "ms" << termcolor::reset << "\n";
		
		if (!extra_stats.empty()) {
			std::cout << extra_stats;
		}

		if (failed == 0) {
			std::cout << "\n" << termcolor::green << termcolor::bold << "🎉 All " << suite_name.substr(0, suite_name.find(" Results")) << " passed! 🎉" << termcolor::reset << "\n";
		} else {
			std::cout << "\n" << termcolor::red << termcolor::bold << "💥 Some " << suite_name.substr(0, suite_name.find(" Results")) << " failed!" << termcolor::reset << "\n";
		}
	}

	void print_summary(const std::vector<test_result>& results) {
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

// Common test suite functionality
namespace test_suite {
	// Shared emulator setup with standard Game Boy initial state
	std::unique_ptr<yahbog::emulator> create_emulator(yahbog::hardware_mode mode) {
		auto emu = std::make_unique<yahbog::emulator>(mode);
		
		// Standard Game Boy initial register state
		auto regs = yahbog::registers{};
		regs.a = 0x01; regs.f = 0xB0;
		regs.b = 0x00; regs.c = 0x13;
		regs.d = 0x00; regs.e = 0xD8;
		regs.h = 0x01; regs.l = 0x4D;
		regs.sp = 0xFFFE; regs.pc = 0x0100;
		
		emu->z80.load_registers(regs);
		return emu;
	}

	emulator_result run_rom_with_serial_check(const std::filesystem::path& rom_path, yahbog::hardware_mode mode) {
		auto start_time = std::chrono::high_resolution_clock::now();
		auto emu = create_emulator(mode);
		
		std::string serial_data{};
		bool serial_written = false;
		std::size_t cycle_count = 0;
		const std::size_t max_cycles = 100000000; // 100M cycles safety limit

		// Hook for serial output detection
		emu->hook_writing([&serial_data, &serial_written](uint16_t addr, uint8_t value) {
			// capture serial transfer data
			if(addr == 0xFF01) [[unlikely]] {
				serial_data += static_cast<char>(value);
				serial_written = true;
				return true;
			}

			return false;
		});

		/*emu->hook_reading([](uint16_t addr) -> yahbog::emulator::reader_hook_response {
			// ignore audio registers
			if(addr >= 0xFF10 && addr <= 0xFF3F) {
				return std::uint8_t{0x00};
			}

			return {};
		});*/

		auto rom = yahbog::rom_t::load_rom(rom_path.string());
		if(!rom) {
			return {false, "Failed to load ROM", 0, std::chrono::milliseconds{0}};
		}
		emu->set_rom(std::move(rom));

		// Execute until pass/fail or timeout
		while(cycle_count < max_cycles) {
			serial_written = false;
			emu->tick();
			cycle_count++;

			if(serial_written) [[unlikely]] {
				if(serial_data.ends_with("Passed")) {
					auto end_time = std::chrono::high_resolution_clock::now();
					auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

					return {true, "", cycle_count, duration};
				} else if(serial_data.ends_with("Failed")) {
					auto end_time = std::chrono::high_resolution_clock::now();
					auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

					return {false, "Test reported failure via serial output", cycle_count, duration};
				}
			}
		}

		// Timeout
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
		return {false, "Timeout (>100M cycles)", cycle_count, duration};
	}

	void write_framebuffer_to_console(const yahbog::emulator& emulator) {
		const auto& framebuffer = emulator.ppu.framebuffer();
		for(std::size_t y = 0; y < yahbog::gpu::screen_height; y++) {
			for(std::size_t x = 0; x < yahbog::gpu::screen_width; x++) {
				auto color = emulator.ppu.get_pixel(x, y);

				constexpr static std::string_view char_map[4] = {" ", "░", "▓", "█"};

				std::cout << char_map[color];
			}
			std::cout << "\n";
		}
	}

	void write_framebuffer_to_file(const yahbog::emulator& emulator, const std::filesystem::path& path) {
		const auto& framebuffer = emulator.ppu.framebuffer();

		const auto real_path = std::filesystem::path(TEST_DATA_DIR) / "results" / path;

		std::filesystem::remove_all(std::filesystem::path(TEST_DATA_DIR) / "results");

		const auto size_multiplier = 8;

		std::array<std::uint8_t, yahbog::gpu::screen_width * yahbog::gpu::screen_height * 3 * size_multiplier * size_multiplier> rgb_framebuffer;

		struct rgb_color {
			std::uint8_t r, g, b;
		};

		constexpr static rgb_color color_map[4] = {
			{0, 0, 0}, // 0
			{64, 64, 64}, // 1
			{128, 128, 128}, // 2
			{255, 255, 255}, // 3
		};

		for(std::size_t y = 0; y < yahbog::gpu::screen_height; y++) {
			for(std::size_t x = 0; x < yahbog::gpu::screen_width; x++) {
				const auto color = emulator.ppu.get_pixel(x, y);
				const auto& rgb = color_map[color];
				for(std::size_t i = 0; i < size_multiplier; i++) {
					for(std::size_t j = 0; j < size_multiplier; j++) {
						rgb_framebuffer[(y * size_multiplier + i) * yahbog::gpu::screen_width * 3 * size_multiplier + (x * size_multiplier + j) * 3 + 0] = rgb.r;
						rgb_framebuffer[(y * size_multiplier + i) * yahbog::gpu::screen_width * 3 * size_multiplier + (x * size_multiplier + j) * 3 + 1] = rgb.g;
						rgb_framebuffer[(y * size_multiplier + i) * yahbog::gpu::screen_width * 3 * size_multiplier + (x * size_multiplier + j) * 3 + 2] = rgb.b;
					}
				}
			}
		}

		std::filesystem::create_directories(path.parent_path());

		stbi_write_png(path.string().c_str(), yahbog::gpu::screen_width * size_multiplier, yahbog::gpu::screen_height * size_multiplier, 3, rgb_framebuffer.data(), yahbog::gpu::screen_width * 3 * size_multiplier);
	}

	test_suite_runner::test_suite_runner(const std::string& name) 
		: suite_name(name), start_time(std::chrono::high_resolution_clock::now()) {}

	void test_suite_runner::start() {
		test_output::print_header(suite_name);
	}

	void test_suite_runner::finish() {
		auto end_time = std::chrono::high_resolution_clock::now();
		auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
		
		// Count results
		int passed = 0, failed = 0;
		std::vector<test_result> failed_tests;
		for (const auto& result : results) {
			if (result.passed) {
				passed++;
			} else {
				failed++;
				failed_tests.push_back(result);
			}
		}

		// Show failed test details before summary
		/*if (!failed_tests.empty()) {
			std::cout << "\n" << termcolor::red << termcolor::bold << "🚨 Failed Test Details:" << termcolor::reset << "\n";
			for (const auto& test : failed_tests) {
				std::cout << termcolor::red << "   ❌ " << termcolor::reset << termcolor::bold << test.name << termcolor::reset;
				if (test.duration.count() > 0) {
					std::cout << termcolor::yellow << " (" << test.duration.count() << "ms)" << termcolor::reset;
				}
				if (!test.details.empty()) {
					std::cout << "\n	 " << termcolor::dark << test.details << termcolor::reset;
				}
				std::cout << "\n";
			}
		}*/

		// Calculate additional stats
		std::stringstream extra_stats;
		if (!results.empty()) {
			auto avg_time = total_duration.count() / results.size();
			extra_stats << "  " << termcolor::magenta << "📊 Average per test: " << avg_time << "ms" << termcolor::reset << "\n";
		}

		// Add custom extra stats if any were set
		if (!custom_extra_stats.empty()) {
			extra_stats << custom_extra_stats;
		}

		test_output::print_suite_summary(suite_name, passed, failed, total_duration, extra_stats.str());
		
		all_passed = (failed == 0);
	}

	void test_suite_runner::add_extra_stats(const std::string& stats) {
		custom_extra_stats += stats;
	}

	void test_suite_runner::add_result(const std::string& name, bool passed, 
								   std::chrono::milliseconds duration, const std::string& details) {
		results.push_back({name, passed, duration, details});
	}

	bool test_suite_runner::passed() const {
		return all_passed;
	}

	void test_suite_runner::print_info(const std::string& message) {
		std::cout << termcolor::blue << message << termcolor::reset << "\n";
	}

	void test_suite_runner::print_test_line(const std::string& name, bool passed, 
										std::chrono::milliseconds duration, const std::string& details) {
		if (passed) {
			std::cout << termcolor::green << "✅ " << termcolor::reset;
		} else {
			std::cout << termcolor::red << "❌ " << termcolor::reset;
		}
		std::cout << std::left << std::setw(25) << name;
		std::cout << termcolor::yellow << std::right << std::setw(8) << duration.count() << "ms" << termcolor::reset;
		if (!details.empty()) {
			std::cout << termcolor::dark << " " << details << termcolor::reset;
		}
		std::cout << "\n";
	}

	progress_tracker::progress_tracker(int total) : total_count(total), current_count(0), done(false) {}

	void progress_tracker::start(const std::string& initial_message) {
		constexpr static std::string_view base_complete_message = "Complete!";
		progress_thread = std::thread([this, initial_message]() {

			std::string complete_message = std::string(base_complete_message);
			if(initial_message.length() > complete_message.length()) {
				complete_message = std::string(base_complete_message) + std::string(initial_message.length() - base_complete_message.length(), ' ');
			}

			while (!done.load()) {
				test_output::print_progress(current_count.load(), total_count, 
					current_count.load() == total_count ? complete_message : initial_message);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		});
	}

	void progress_tracker::update(int count) {
		current_count.store(count);
	}

	void progress_tracker::finish() {
		done = true;
		if (progress_thread.joinable()) {
			progress_thread.join();
		}
		
		// Ensure we show 100% completion
		test_output::print_progress(total_count, total_count, "Complete!	   ");
		std::cout << "\n";
	}
}
