#include <yahbog-tests.h>

int main(int argc, char** argv) {

	std::filesystem::remove_all(std::filesystem::path(TEST_DATA_DIR) / "results");

	test_output::print_header("Yahbog Test Suite");
	std::cout << termcolor::blue << "🎮 Game Boy emulator test suite" << termcolor::reset << "\n";

	auto overall_start = std::chrono::high_resolution_clock::now();
	
	auto all_passed = true;
	int total_test_suites = 1;
	int passed_suites = 0;

	std::cout << termcolor::cyan << "   Running " << total_test_suites << " test suites..." << termcolor::reset << "\n\n";

	// Single step tests
	if (run_single_step_tests()) {
		passed_suites++;
	} else {
		all_passed = false;
	}
	std::cout << "\n";

	// Blargg CPU instruction tests  
	if (run_blargg_cpu_instrs()) {
		passed_suites++;
	} else {
		all_passed = false;
	}
	std::cout << "\n";

	// Blargg general tests
	if (run_blargg_general()) {
		passed_suites++;
	} else {
		all_passed = false;
	}
	std::cout << "\n";

	// Mooneye tests
	if (run_mooneye()) {
		passed_suites++;
	} else {
		all_passed = false;
	}
	std::cout << "\n";

	// GB Microtest tests
	if (run_gbmicrotest()) {
		passed_suites++;
	} else {
		all_passed = false;
	}
	std::cout << "\n";

	auto overall_end = std::chrono::high_resolution_clock::now();
	auto overall_duration = std::chrono::duration_cast<std::chrono::milliseconds>(overall_end - overall_start);

	// Final summary
	std::cout << "\n";
	test_output::print_header("Final Results");

	std::cout << termcolor::cyan << termcolor::bold << "Overall Summary:" << termcolor::reset << "\n";
	std::cout << "  " << termcolor::green << "✅ Test suites passed: " << passed_suites << "/" << total_test_suites << termcolor::reset << "\n";
	if (passed_suites != total_test_suites) {
		std::cout << "  " << termcolor::red << "❌ Test suites failed: " << (total_test_suites - passed_suites) << termcolor::reset << "\n";
	}
	std::cout << "  " << termcolor::yellow << "⏱️  Total execution time: " << overall_duration.count() << "ms" << termcolor::reset << "\n";

	if (all_passed) {
		std::cout << "\n" << termcolor::green << termcolor::bold << "🎉🎉🎉 ALL TESTS PASSED! 🎉🎉🎉" << termcolor::reset << "\n";
	} else {
		std::cout << "\n" << termcolor::red << termcolor::bold << "💥 SOME TESTS FAILED!" << termcolor::reset << "\n";
		std::cout << termcolor::red << "Please check the failed tests above for details. 🔍" << termcolor::reset << "\n\n";
	}

	return all_passed ? 0 : 1;
}