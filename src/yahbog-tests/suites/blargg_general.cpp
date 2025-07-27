#include <yahbog-tests.h>

namespace {

	struct test_info {
		std::string_view name;

		std::string_view rom_src;
		std::string_view rom_path;
	};

	#define BASE_DIR TEST_DATA_DIR "/blargg/general"



}

bool run_blargg_general() {
	TestSuite::test_suite_runner suite("Blargg General Tests");
	suite.start();

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

	suite.print_info("🔍 Found " + std::to_string(roms.size()) + " general tests");
	suite.print_info("📊 Running tests with serial output verification...");
	std::cout << "\n";

	for(const auto& rom : roms) {
		auto result = TestSuite::run_rom_with_serial_check(rom);
		auto real_time_ms = test_output::cycles_to_real_time_ms(result.cycles_executed);
		
		// Print individual test result with timing info
		suite.print_test_line(
			rom.filename().string(), 
			result.passed, 
			result.execution_time,
			"(real: " + test_output::format_real_time(real_time_ms) + ", " + std::to_string(result.cycles_executed) + " cycles)"
		);
		
		// Show failure details if test failed
		if (!result.passed) {
			std::cout << termcolor::red << "   💬 " << result.failure_reason << termcolor::reset << "\n";
		}
		
		suite.add_result(rom.filename().string(), result.passed, result.execution_time);
	}

	suite.finish();
	return suite.passed();
}