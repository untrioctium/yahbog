#include <yahbog-tests.h>

#include <termcolor/termcolor.hpp>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <zip.h>

// Function to check if a test can run on original Game Boy models (group G)
// Based on Mooneye test suite naming scheme:
// - G = dmg+mgb (Game Boy + Game Boy Pocket)
// - dmg = Game Boy only
// - mgb = Game Boy Pocket only  
// - Tests without specific suffixes run on all platforms
bool can_run_on_original_gameboy(std::string_view filename) {

	// filter madness, manual-only, and utils
	if(filename.contains("madness") || filename.contains("manual-only") || filename.contains("utils")) {
		return false;
	}

	// Trim up to the last /
	auto last_slash = filename.find_last_of('/');
	if (last_slash == std::string::npos) {
		return true;
	}

	std::string_view name = filename.substr(last_slash + 1);

	// Remove the .gb extension for analysis
	if (name.ends_with(".gb")) {
		name = name.substr(0, name.length() - 3);
	}
	
	// Tests without specific model suffixes should run on all platforms
	if (name.find('-') == std::string::npos) {
		return true;
	}
	
	// Find the last part after the final dash (this contains the model/group info)
	size_t last_dash = name.find_last_of('-');
	if (last_dash == std::string::npos) {
		return true; // No model suffix, should run on all platforms
	}
	
	std::string_view suffix = name.substr(last_dash + 1);
	
	// Check for original Game Boy specific tests
	if (suffix.contains("dmg")) {
		if(suffix.contains("dmg0")) return false;
		return true;
	}
	
	// Check for group G (dmg+mgb) and combinations containing G
	if (suffix.find('G') != std::string::npos) {
		return true;
	}
	
	// Tests that don't match original Game Boy models
	return false;
}

struct mooneye_test {
	std::string name;
	std::vector<std::uint8_t> rom;
};

test_suite::emulator_result run_mooneye_test(mooneye_test& test) {
	auto start_time = std::chrono::high_resolution_clock::now();
	auto emu = test_suite::create_emulator(yahbog::hardware_mode::dmg);

	auto rom = yahbog::rom_t::load_rom(std::move(test.rom));
	if(!rom) {
		return {false, "Failed to load ROM (bad MBC?)", 0, std::chrono::milliseconds{0}};
	}
	emu->set_rom(std::move(rom));

    emu->z80.reset(&emu->mem_fns);
    emu->io.reset();
    emu->ppu.reset();

	const std::size_t max_cycles = static_cast<std::size_t>(10 * GB_CPU_FREQUENCY_HZ); // 180 real seconds

	std::size_t cycles = 0;

	while(cycles < max_cycles) {
		emu->tick();
		cycles++;

		if(emu->z80.r().ir == 0x40) [[unlikely]] {
			auto end_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
			
			auto r = emu->z80.r();
			bool passed = false;
			std::string details = "";

			if(r.b == 3 && r.c == 5 && r.d == 8 && r.e == 13 && r.h == 21 && r.l == 34) {
				passed = true;
			} else if(r.b == 0x42 && r.c == 0x42 && r.d == 0x42 && r.e == 0x42 && r.h == 0x42 && r.l == 0x42) {
				details = "Registers reported failure";
			} else {
				details = "Registers do not match any expected values";
			}

			if(!passed) {
				// wait until the next vblank
				while(!emu->ppu.framebuffer_ready()) {
					emu->tick();
				}

				auto path = std::filesystem::path("mooneye") / (std::string(test.name) + ".png");
				test_suite::write_framebuffer_to_file(*emu, path);
			}
			return {passed, details, cycles, duration};
		}
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

	// dump the last drawn framebuffer, might not have anything useful to show though
	auto path = std::filesystem::path("mooneye") / (std::string(test.name) + ".png");
	test_suite::write_framebuffer_to_file(*emu, path);

	return {false, "Timeout (>10 real seconds)", cycles, duration};
	
}

bool run_mooneye() {

	test_suite::test_suite_runner suite("Mooneye Tests");
	suite.start();

	if(!std::filesystem::exists(TEST_DATA_DIR "/mooneye.zip")) {
		std::cout << termcolor::red << "❌ mooneye.zip not found, please run scripts/get_testing_deps.py" << termcolor::reset << "\n";
		return false;
	}

	suite.print_info("🔍 Opening archive: " + std::string(TEST_DATA_DIR) + "/mooneye.zip");
	std::unique_ptr<zip_t, decltype(&zip_close)> archive_ptr(zip_open(TEST_DATA_DIR "/mooneye.zip", 0, 'r'), &zip_close);
	auto archive = archive_ptr.get();

	if (!archive_ptr) {
		std::cout << termcolor::red << "❌ Failed to open archive" << termcolor::reset << "\n";
		return false;
	}

	std::vector<mooneye_test> test_files;

	auto file_count = zip_entries_total(archive);

	for(auto i = 0; i < file_count; i++) {

		zip_entry_openbyindex(archive, i);
		std::string_view name = zip_entry_name(archive);
		if(name.ends_with(".gb") && can_run_on_original_gameboy(name)) {
			// trim the first directory
			auto first_slash = name.find_first_of('/');
			auto trimmed_name = name.substr(first_slash + 1);
			test_files.push_back(mooneye_test{std::string(trimmed_name), std::vector<std::uint8_t>(zip_entry_size(archive))});
			zip_entry_noallocread(archive, test_files.back().rom.data(), test_files.back().rom.size());
		}
		zip_entry_close(archive);
	}

    constexpr static auto split = [](std::string_view str, char delimiter) {
        std::vector<std::string_view> components;
        auto start = 0;
        auto end = str.find(delimiter);
        while(end != std::string_view::npos) {
            components.push_back(str.substr(start, end - start));
            start = end + 1;
            end = str.find(delimiter, start);
        }
        components.push_back(str.substr(start));
        return components;
    };

	std::sort(test_files.begin(), test_files.end(), [](const mooneye_test& a, const mooneye_test& b) {
        auto a_components = split(a.name, '/');
        auto b_components = split(b.name, '/');
        
        // Compare directory components one by one
        size_t min_dirs = std::min(a_components.size() - 1, b_components.size() - 1);
        
        for (size_t i = 0; i < min_dirs; ++i) {
            if (a_components[i] != b_components[i]) {
                return a_components[i] < b_components[i];
            }
        }
        
        // If all common directory components are the same,
        // files in parent directories come after files in subdirectories
        if (a_components.size() != b_components.size()) {
            return a_components.size() > b_components.size();
        }
        
        // Same directory level, sort alphabetically by filename
        return a_components.back() < b_components.back();
    });

	suite.print_info("🔍 Found " + std::to_string(test_files.size()) + " tests compatible with original Game Boy models");

	for(auto& test_file : test_files) {
		auto result = run_mooneye_test(test_file);
		auto real_time_ms = test_output::cycles_to_real_time_ms(result.cycles_executed);

		suite.print_test_line(
			test_file.name,
			result.passed,
			result.execution_time,
			"(real: " + test_output::format_real_time(real_time_ms) + ", " + std::to_string(result.cycles_executed) + " cycles)"
		);

		if(!result.passed) {
			std::cout << termcolor::red << "   💬 " << result.failure_reason << termcolor::reset << "\n";
		}

		suite.add_result(test_file.name, result.passed, result.execution_time);
	}

	suite.finish();
	return suite.passed();

}