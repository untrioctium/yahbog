#include <yahbog-tests.h>

#include <termcolor/termcolor.hpp>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <zip.h>

struct gbmicrotest_test {
	std::string name;
	std::vector<std::uint8_t> rom;
};

test_suite::emulator_result run_gbmicrotest_test(gbmicrotest_test& test) {
	auto start_time = std::chrono::high_resolution_clock::now();
	auto emu = test_suite::create_emulator(yahbog::hardware_mode::dmg);

	auto rom = yahbog::rom_t::load_rom(std::move(test.rom));
	if(!rom) {
		return {false, "Failed to load ROM", 0, std::chrono::milliseconds{0}};
	}
	emu->set_rom(std::move(rom));

    emu->z80.reset(&emu->mem_fns);
    emu->io.reset();
    emu->ppu.reset();

    bool done = false;
    bool passed = false;

    emu->hook_writing([&done, &passed](std::uint16_t addr, std::uint8_t value) {
        if(addr == 0xFF82) [[unlikely]] {
            if(value == 0x01) {
                done = true;
                passed = true;
                return true;
            } else if(value == 0xFF) {
                done = true;
                passed = false;
                return true;
            }
        }
        return false;
    });

	const std::size_t max_cycles = static_cast<std::size_t>(1 * GB_CPU_FREQUENCY_HZ); // 1 real second

	std::size_t cycles = 0;

	while(cycles < max_cycles) {
		emu->tick();
		cycles++;

		if(done) [[unlikely]] {
			auto end_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
			
			if(!passed) {
                return {false, "Test reported failure via write to 0xFF82", cycles, duration};
            }

			return {true, "Test passed", cycles, duration};
		}
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
	return {false, "Timeout (>1 real second)", cycles, duration};
	
}

bool run_gbmicrotest() {

	test_suite::test_suite_runner suite("GB Microtest");
	suite.start();

	if(!std::filesystem::exists(TEST_DATA_DIR "/gbmicrotest.zip")) {
		std::cout << termcolor::red << "❌ gbmicrotest.zip not found, please run scripts/get_testing_deps.py" << termcolor::reset << "\n";
		return false;
	}

	suite.print_info("🔍 Opening archive: " + std::string(TEST_DATA_DIR) + "/gbmicrotest.zip");
	std::unique_ptr<zip_t, decltype(&zip_close)> archive_ptr(zip_open(TEST_DATA_DIR "/gbmicrotest.zip", 0, 'r'), &zip_close);
	auto archive = archive_ptr.get();

	if (!archive_ptr) {
		std::cout << termcolor::red << "❌ Failed to open archive" << termcolor::reset << "\n";
		return false;
	}

	std::vector<gbmicrotest_test> test_files;

	auto file_count = zip_entries_total(archive);

	for(auto i = 0; i < file_count; i++) {

		zip_entry_openbyindex(archive, i);
		std::string_view name = zip_entry_name(archive);
		if(name.ends_with(".gb")) {
			// trim the first directory
			auto first_slash = name.find_first_of('/');
			auto trimmed_name = name.substr(first_slash + 1);
			test_files.push_back(gbmicrotest_test{std::string(trimmed_name), std::vector<std::uint8_t>(zip_entry_size(archive))});
			zip_entry_noallocread(archive, test_files.back().rom.data(), test_files.back().rom.size());
		}
		zip_entry_close(archive);
	}

	std::sort(test_files.begin(), test_files.end(), [](const gbmicrotest_test& a, const gbmicrotest_test& b) {
		return a.name < b.name;
	});

	suite.print_info("🔍 Found " + std::to_string(test_files.size()) + " tests compatible with original Game Boy models");

	for(auto& test_file : test_files) {
		auto result = run_gbmicrotest_test(test_file);
		auto real_time_ms = test_output::cycles_to_real_time_ms(result.cycles_executed);

		suite.print_test_line(
			test_file.name,
			result.passed,
			result.execution_time,
			"(real: " + test_output::format_real_time(real_time_ms) + ", " + std::to_string(result.cycles_executed) + " cycles)"
		);

		//if(!result.passed) {
		//	std::cout << termcolor::red << "   💬 " << result.failure_reason << termcolor::reset << "\n";
		//}

		suite.add_result(test_file.name, result.passed, result.execution_time);
	}

	suite.finish();
	return suite.passed();

}