#include <yahbog-tests.h>

namespace {

	struct test_info {
		std::string_view name;

		std::string_view rom_src;
		std::string_view rom_path;
	};

	#define BASE_DIR TEST_DATA_DIR "/blargg/general"

	bool run_test(const std::filesystem::path& rom_path) {
		auto filename = rom_path.filename().string();
		auto start_time = std::chrono::high_resolution_clock::now();

		auto emu = std::make_unique<yahbog::emulator>();
		std::string serial_data{};
		emu->hook_writing([&serial_data](uint16_t addr, uint8_t value) {
			// ignore audio registers
			if(addr >= 0xFF10 && addr <= 0xFF3F) {
				return true;
			}

			// ignore serial transfer registers
			if(addr == 0xFF02) {
				return true;
			}

			// ignore joypad
			if(addr == 0xFF00) {
				return true;
			}

			// print serial transfer data
			if(addr == 0xFF01) {
				serial_data += static_cast<char>(value);
				return true;
			}

			return false;
		});

		emu->hook_reading([](uint16_t addr) -> yahbog::emulator::reader_hook_response {
			if(addr == 0xFF00) {
				return std::uint8_t{0x00};
			}

			// ignore audio registers
			if(addr >= 0xFF10 && addr <= 0xFF3F) {
				return std::uint8_t{0x00};
			}

			return {};
		});

		auto regs = yahbog::registers{};
		regs.a = 0x01; regs.f = 0xB0;
		regs.b = 0x00; regs.c = 0x13;
		regs.d = 0x00; regs.e = 0xD8;
		regs.h = 0x01; regs.l = 0x4D;
		regs.sp = 0xFFFE; regs.pc = 0x0100;

		emu->z80.load_registers(regs);
		emu->rom.load_rom(rom_path.string());

		auto& cpu = emu->z80;
		auto& mem = emu->mmu;

		// Track cycles for performance info
		std::size_t cycle_count = 0;
		const std::size_t max_cycles = 100000000; // 100M cycles safety limit

		while(cycle_count < max_cycles) {
			cpu.cycle();
			cycle_count++;

			if(serial_data.ends_with("Passed")) {
				auto end_time = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
				auto real_time_ms = TestOutput::cycles_to_real_time_ms(cycle_count);
				
				std::cout << termcolor::green << "✅ " << std::left << std::setw(40) << filename;
				std::cout << termcolor::yellow << std::right << std::setw(8) << duration.count() << "ms" << termcolor::reset;
				std::cout << termcolor::dark << " (real: " << TestOutput::format_real_time(real_time_ms) << ", " << cycle_count << " cycles)" << termcolor::reset << "\n";
				return true;
			} else if(serial_data.ends_with("Failed")) {
				auto end_time = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
				auto real_time_ms = TestOutput::cycles_to_real_time_ms(cycle_count);
				
				std::cout << termcolor::red << "❌ " << std::left << std::setw(40) << filename;
				std::cout << termcolor::yellow << std::right << std::setw(8) << duration.count() << "ms" << termcolor::reset;
				std::cout << termcolor::red << " Test failed" << termcolor::reset << "\n";
				std::cout << termcolor::dark << "  Real time: " << TestOutput::format_real_time(real_time_ms) << ", " << cycle_count << " cycles" << termcolor::reset << "\n";
				std::cout << termcolor::dark << "  Serial output: " << serial_data << termcolor::reset << "\n";
				return false;
			}
		}

		// Test timed out
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
		auto real_time_ms = TestOutput::cycles_to_real_time_ms(cycle_count);
		
		std::cout << termcolor::red << "❌ " << std::left << std::setw(40) << filename;
		std::cout << termcolor::yellow << std::right << std::setw(8) << duration.count() << "ms" << termcolor::reset;
		std::cout << termcolor::red << " Timeout (>100M cycles)" << termcolor::reset << "\n";
		std::cout << termcolor::dark << "  Real time: " << TestOutput::format_real_time(real_time_ms) << ", " << cycle_count << " cycles" << termcolor::reset << "\n";
		return false;
	}

}

bool run_blargg_general() {
	TestOutput::print_header("Blargg General Tests");

	std::vector<std::filesystem::path> roms{};

	for(const auto& entry : std::filesystem::directory_iterator(BASE_DIR)) {
		if(entry.is_regular_file() && entry.path().extension() == ".gb") {
			roms.push_back(entry.path());
		}
	}

	std::sort(roms.begin(), roms.end());

	if (roms.empty()) {
		std::cout << termcolor::red << "❌ No test ROMs found in " << BASE_DIR << termcolor::reset << "\n";
		return false;
	}

	std::cout << termcolor::blue << "🔍 Found " << roms.size() << " general tests" << termcolor::reset << "\n";
	std::cout << termcolor::blue << "📊 Running tests with serial output verification..." << termcolor::reset << "\n\n";

	auto start_time = std::chrono::high_resolution_clock::now();
	std::vector<TestResult> results;
	int passed = 0, failed = 0;

	for(const auto& rom : roms) {
		auto test_start = std::chrono::high_resolution_clock::now();
		bool success = run_test(rom);
		auto test_end = std::chrono::high_resolution_clock::now();
		auto test_duration = std::chrono::duration_cast<std::chrono::milliseconds>(test_end - test_start);
		
		results.push_back({rom.filename().string(), success, test_duration, ""});
		
		if (success) {
			passed++;
		} else {
			failed++;
		}
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

	std::cout << "\n";
	TestOutput::print_header("Blargg General Test Results");

	if (failed > 0) {
		std::cout << termcolor::red << "❌ Failed tests:" << termcolor::reset << "\n";
		for (const auto& result : results) {
			if (!result.passed) {
				std::cout << "   • " << result.name << " (" << result.duration.count() << "ms)\n";
			}
		}
		std::cout << "\n";
	}

	std::cout << termcolor::cyan << termcolor::bold << "Summary:" << termcolor::reset << "\n";
	std::cout << "  " << termcolor::green << "✅ Passed: " << passed << termcolor::reset << "\n";
	if (failed > 0) {
		std::cout << "  " << termcolor::red << "❌ Failed: " << failed << termcolor::reset << "\n";
	}
	std::cout << "  " << termcolor::yellow << "⏱️  Total time: " << total_duration.count() << "ms" << termcolor::reset << "\n";
	
	// Calculate average time per test
	if (!results.empty()) {
		auto avg_time = total_duration.count() / results.size();
		std::cout << "  " << termcolor::magenta << "📊 Average per test: " << avg_time << "ms" << termcolor::reset << "\n";
	}

	bool all_passed = (failed == 0);
	if (all_passed) {
		std::cout << "\n" << termcolor::green << termcolor::bold << "🎉 All general tests passed! 🎉" << termcolor::reset << "\n";
	} else {
		std::cout << "\n" << termcolor::red << termcolor::bold << "💥 Some general tests failed!" << termcolor::reset << "\n";
	}

	return all_passed;
}