#include <yahbog-tests.h>

#include <readerwritercircularbuffer.h>
#include <termcolor/termcolor.hpp>

#include <array>
#include <functional>
#include <fstream>
#include <iostream>
#include <vector>
#include <format>
#include <sstream> // Required for std::ostringstream
#include <iomanip> // Required for std::setw

// outputs in the format:
// A:00 F:11 B:22 C:33 D:44 E:55 H:66 L:77 SP:8888 PC:9999 PCMEM:AA,BB,CC,DD
std::string gameboy_doctor_log(const yahbog::registers& reg, const auto& mem_read_fn) {
	return std::format("A:{:02X} F:{:02X} B:{:02X} C:{:02X} D:{:02X} E:{:02X} H:{:02X} L:{:02X} SP:{:04X} PC:{:04X} PCMEM:{:02X},{:02X},{:02X},{:02X}",
		reg.a, reg.f, reg.b, reg.c, reg.d, reg.e, reg.h, reg.l, reg.sp, reg.pc - 1,
		mem_read_fn(reg.pc - 1), mem_read_fn(reg.pc), mem_read_fn(reg.pc + 1), mem_read_fn(reg.pc + 2));
}

struct test_info {
	std::string_view name;

	std::string_view rom_src;
	std::string_view rom_path;

	std::string_view log_src;
	std::string_view log_path;
};

#define BASE_DIR TEST_DATA_DIR "/blargg/cpu_instrs"

struct decompress_thread_state {
	std::string_view log_path;
	moodycamel::BlockingReaderWriterCircularBuffer<std::string>* log_queue;
	std::stop_source logs_done;
	std::stop_token  execution_done;
	std::string error_message;
};

// Global data for callback communication
struct callback_context {
	decompress_thread_state* state;
	std::string* line_buffer;
};

// Static callback function for zip extraction
static size_t extract_callback(void* arg, uint64_t offset, const void* data, size_t size) {
	(void)offset;  // Unused
	
	callback_context* ctx = static_cast<callback_context*>(arg);
	if (!ctx || !ctx->state || !ctx->line_buffer) {
		return 0;  // Invalid context
	}
	
	// Check if execution should stop
	if (ctx->state->execution_done.stop_requested()) {
		return 0;  // Signal to stop extraction
	}
	
	const char* char_data = static_cast<const char*>(data);
	
	// Process each character in the data
	for (size_t i = 0; i < size; ++i) {
		char c = char_data[i];
		
		if (c == '\n') {
			// Complete line found - try to push to queue
			if (!ctx->line_buffer->empty()) {
				try {
					// Use simple enqueue - the queue will handle blocking internally
					while(!ctx->state->execution_done.stop_requested()) {
						if (ctx->state->log_queue->try_enqueue(std::move(*ctx->line_buffer))) {
							break;
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(0));
					}
				} catch (const std::exception& e) {
					ctx->state->error_message = "Failed to enqueue line: " + std::string(e.what());
					return 0;  // Signal to stop extraction
				}
			}
			ctx->line_buffer->clear();
		} else if (c != '\r') {  // Skip carriage returns
			*ctx->line_buffer += c;
		}
		
		// Periodically check if we should stop
		if (i % 1024 == 0 && ctx->state->execution_done.stop_requested()) {
			return 0;  // Signal to stop extraction
		}
	}
	
	return size;  // Return number of bytes processed
}

void print_colored_diff(std::string_view expected, std::string_view actual) {
	// Split both strings into sections (space-separated)
	auto split_string = [](std::string_view str) {
		std::vector<std::string> sections;
		size_t start = 0;
		size_t pos = 0;
		
		while ((pos = str.find(' ', start)) != std::string_view::npos) {
			if (pos > start) {
				sections.emplace_back(str.substr(start, pos - start));
			}
			start = pos + 1;
		}
		if (start < str.length()) {
			sections.emplace_back(str.substr(start));
		}
		return sections;
	};
	
	auto expected_sections = split_string(expected);
	auto actual_sections = split_string(actual);
	
	// Print expected line
	size_t max_sections = (std::max)(expected_sections.size(), actual_sections.size());
	
	std::cout << "	  ";
	for (size_t i = 0; i < max_sections; ++i) {
		if (i > 0) std::cout << " ";
		
		std::string exp_section = (i < expected_sections.size()) ? expected_sections[i] : "";
		std::string act_section = (i < actual_sections.size()) ? actual_sections[i] : "";
		
		if (exp_section != act_section) {
			std::cout << termcolor::green << exp_section << termcolor::reset;
		} else {
			std::cout << termcolor::dark << exp_section << termcolor::reset;
		}
	}
	std::cout << "\n	  ";
	
	// Print actual line
	for (size_t i = 0; i < max_sections; ++i) {
		if (i > 0) std::cout << " ";
		
		std::string exp_section = (i < expected_sections.size()) ? expected_sections[i] : "";
		std::string act_section = (i < actual_sections.size()) ? actual_sections[i] : "";
		
		if (exp_section != act_section) {
			std::cout << termcolor::red << act_section << termcolor::reset;
		} else {
			std::cout << termcolor::dark << act_section << termcolor::reset;
		}
	}
	std::cout << "\n";
}

template<typename F>
struct defer_t {
	F func;

	defer_t(F&& func) : func(std::forward<F>(func)) {}

	~defer_t() {
		func();
	}
};

#define TOKEN_PASTE(x, y) x ## y
#define CAT(x, y) TOKEN_PASTE(x, y)
#define DEFER(expr) auto CAT(deferred_, __LINE__) = defer_t{[&] { expr; }}

void decompress_thread(decompress_thread_state&& state) {
	
	DEFER(state.logs_done.request_stop());

	// Buffer to accumulate partial lines
	std::string line_buffer;
	
	// Context for the callback
	callback_context ctx = {&state, &line_buffer};
	
	try {
		zip_t* zip = nullptr;
		DEFER(zip_close(zip));

		// Open the zip file
		int error_code = 0;
		zip = zip_open(state.log_path.data(), 0, 'r');
		if (!zip) {
			state.error_message = "Failed to open zip file: " + std::string(state.log_path);
			return;
		}
		
		// Get total number of entries
		int64_t entries = zip_entries_total(zip);
		if (entries <= 0) {
			state.error_message = "Zip file is empty or invalid";
			return;
		}
		
		// Open the first entry
		if (zip_entry_openbyindex(zip, 0) < 0) {
			state.error_message = "Failed to open first entry in zip file";
			return;
		}

		DEFER(zip_entry_close(zip));
		
		// Extract the entry using the callback
		if (zip_entry_extract(zip, extract_callback, &ctx) < 0) {
			// Check if this was due to execution_done being triggered
			if (!state.execution_done.stop_requested()) {
				state.error_message = "Failed to extract entry from zip file";
			}
			return;
		}
		
		// Handle any remaining data in line_buffer (file might not end with newline)
		if (!line_buffer.empty() && !state.execution_done.stop_requested()) {
			try {
				while(!state.execution_done.stop_requested()) {
					if (state.log_queue->try_enqueue(std::move(line_buffer))) {
						break;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(0));
				}
			} catch (const std::exception& e) {
				state.error_message = "Failed to enqueue final line: " + std::string(e.what());
			}
		}
		
	} catch (const std::exception& e) {
		state.error_message = "Exception during decompression: " + std::string(e.what());
	}
}



static bool run_test(const std::filesystem::path& rom_path, yahbog::hardware_mode mode) {
	auto filename = rom_path.filename().string();
	auto start_time = std::chrono::high_resolution_clock::now();

	auto number_start = filename.find_first_not_of('0');
	auto number_end = filename.find_first_of('-');

	if(number_start == std::string::npos || number_end == std::string::npos) {
		std::cout << termcolor::red << "❌ " << std::left << std::setw(40) << filename 
				  << "Invalid filename format" << termcolor::reset << "\n";
		return false;
	}

	auto number = filename.substr(number_start, number_end - number_start);
	auto log_filename = std::format("{}.zip", number);
	auto log_path = rom_path.parent_path() / log_filename;

	if (!std::filesystem::exists(log_path)) {
		std::cout << termcolor::red << "❌ " << std::left << std::setw(40) << filename 
				  << "Log file not found: " << log_filename << termcolor::reset << "\n";
		return false;
	}

	moodycamel::BlockingReaderWriterCircularBuffer<std::string> log_queue(1024);
	std::stop_source execution_done;

	auto log_string = log_path.string();
	decompress_thread_state state = {
		.log_path = log_string,
		.log_queue = &log_queue,
		.execution_done = execution_done.get_token()
	};

	auto logs_done = state.logs_done.get_token();

	std::jthread dt(decompress_thread, std::move(state));
	DEFER(execution_done.request_stop());

	auto emu = std::make_unique<yahbog::dmg_emulator>();

	auto rom = yahbog::rom_t::load_rom(rom_path.string());
	if(!rom) {
		std::cout << termcolor::red << "❌ " << std::left << std::setw(40) << filename 
				  << "Failed to load ROM" << termcolor::reset << "\n";
		return false;
	}
	emu->set_rom(std::move(rom));

	auto regs = yahbog::registers{};
	regs.a = 0x01; regs.f = 0xB0;
	regs.b = 0x00; regs.c = 0x13;
	regs.d = 0x00; regs.e = 0xD8;
	regs.h = 0x01; regs.l = 0x4D;
	regs.sp = 0xFFFE; regs.pc = 0x0100;

	emu->z80.load_registers(regs);

	auto& cpu = emu->z80;
	auto& mem = emu->mmu;

	std::size_t instruction_count = 0;
	std::size_t cycle_count = 0;
	std::vector<std::string> logs{};
	constexpr auto max_logs = 10;

	auto is_skipped_instruction = [](std::uint16_t instruction) {
		return instruction == 0xCB || instruction == 0xE3 || instruction == 0xE4 || instruction == 0xEB || instruction == 0xEC || instruction == 0xED || instruction == 0x76;
	};

	while(!logs_done.stop_requested()) {

		std::uint16_t next_instruction = 0;

		do {
			next_instruction = emu->z80.r().ir;
			do {
				emu->z80.cycle(&emu->mem_fns);
				cycle_count++;
			} while (cpu.r().mupc != 0);
			instruction_count++;
		} while(is_skipped_instruction(next_instruction) || cpu.r().halted);

		auto my_log = gameboy_doctor_log(cpu.r(), [&mem](uint16_t addr) { return mem.read(addr); });
		auto expected = std::string{};

		while(true) {
			if (log_queue.wait_dequeue_timed(expected, std::chrono::milliseconds(100)) || logs_done.stop_requested()) {
				break;
			}
		}

		if(logs_done.stop_requested()) {
			break;
		}

		if(expected != my_log) {
			auto end_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
			auto real_time_ms = test_output::cycles_to_real_time_ms(cycle_count);
			
			std::cout << termcolor::red << "❌ " << std::left << std::setw(40) << filename;
			std::cout << termcolor::yellow << std::right << std::setw(8) << duration.count() << "ms" << termcolor::reset;
			std::cout << termcolor::red << " Log mismatch" << termcolor::reset << "\n";
			
			constexpr auto format_interrupts = [](std::uint8_t reg) {
				return std::format("{}{}{}{}{}",
					(reg & 16u) ? "J" : ".",
					(reg & 8u) ? "S" : ".",
					(reg & 4u) ? "T" : ".",
					(reg & 2u) ? "L" : ".",
					(reg & 1u) ? "V" : "."
				);
			};
			
			// Show detailed failure information with better formatting
			std::cout << "\n" << termcolor::red << termcolor::bold << "  🔍 DETAILED DIAGNOSTIC INFORMATION:" << termcolor::reset << "\n";
			
			std::cout << termcolor::cyan << "  📍 Execution Context:" << termcolor::reset << "\n";
			std::cout << "	  Last instruction: " << termcolor::yellow << std::format("{:02X}", next_instruction);
			if (next_instruction < yahbog::opinfo.size()) {
				std::cout << " (" << yahbog::opinfo[next_instruction].name << ")";
			}
			std::cout << termcolor::reset << "\n";
			std::cout << "	  Instructions executed: " << termcolor::magenta << instruction_count << termcolor::reset << "\n";
			std::cout << "	  Cycles executed: " << termcolor::magenta << cycle_count << termcolor::reset << "\n";
			
			std::cout << termcolor::cyan << "  🔧 System State:" << termcolor::reset << "\n";
			std::cout << "	  IME: " << termcolor::yellow << (cpu.r().ime ? "enabled" : "disabled") << termcolor::reset << "\n";
			std::cout << "	  IF: " << termcolor::yellow << format_interrupts(emu->mmu.read(0xFF0F)) << termcolor::reset << "\n";
			std::cout << "	  IE: " << termcolor::yellow << format_interrupts(emu->mmu.read(0xFFFF)) << termcolor::reset << "\n";
			
			// Show the last few agreeing log lines for context
			if (!logs.empty()) {
				std::cout << termcolor::cyan << "  📜 Last Agreeing States:" << termcolor::reset << "\n";
				for(const auto& log : logs) {
					std::cout << termcolor::dark << "	  " << log << termcolor::reset << "\n";
				}
			}
			
			// Show the colored diff of expected vs actual
			std::cout << termcolor::cyan << "  ⚡ State Mismatch (green = expected, red = actual):" << termcolor::reset << "\n";
			print_colored_diff(expected, my_log);
			
			std::cout << termcolor::dark << "	 " << std::string(50, '-') << termcolor::reset << "\n";
			
			return false;
		}

		logs.push_back(std::move(my_log));
		if(logs.size() > max_logs) {
			logs.erase(logs.begin());
		}
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
	auto real_time_ms = test_output::cycles_to_real_time_ms(cycle_count);
	
	auto serialized = emu->serialize();

	std::cout << termcolor::green << "✅ " << std::left << std::setw(40) << filename;
	std::cout << termcolor::yellow << std::right << std::setw(8) << duration.count() << "ms" << termcolor::reset;
	std::cout << termcolor::dark << " (real: " << test_output::format_real_time(real_time_ms) << ", " << cycle_count << " cycles, " << serialized.size() << " state bytes)" << termcolor::reset << "\n";
	



	return true;
}

bool run_blargg_cpu_instrs() {
	test_suite::test_suite_runner suite("Blargg CPU Instruction Tests");
	suite.start();

	std::vector<std::filesystem::path> roms{};

	// find all .gb files in the cpu_instrs directory
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

	suite.print_info("🔍 Found " + std::to_string(roms.size()) + " CPU instruction tests");
	suite.print_info("⚡ Using fast serial-check with detailed verification fallback");
	std::cout << "\n";

	for(const auto& rom : roms) {
		auto filename = rom.filename().string();
		
		// Fast path: Try serial check first (much faster)
		auto fast_result = test_suite::run_rom_with_serial_check(rom, yahbog::hardware_mode::dmg);
		
		if (fast_result.passed) {
			// Test passed with fast check - we're done!
			auto real_time_ms = test_output::cycles_to_real_time_ms(fast_result.cycles_executed);
			suite.print_test_line(
				filename,
				true,
				fast_result.execution_time,
				"(real: " + test_output::format_real_time(real_time_ms) + ", " + std::to_string(fast_result.cycles_executed) + " cycles)"
			);
			suite.add_result(filename, true, fast_result.execution_time);
		} else {
			// Fast check failed - run detailed log verification for diagnostics
			std::cout << termcolor::yellow << "🔍 " << std::left << std::setw(40) << filename;
			std::cout << "Running detailed verification..." << termcolor::reset << "\n";
			
			auto detailed_start = std::chrono::high_resolution_clock::now();
			bool detailed_success = run_test(rom, yahbog::hardware_mode::dmg);
			auto detailed_end = std::chrono::high_resolution_clock::now();
			auto detailed_duration = std::chrono::duration_cast<std::chrono::milliseconds>(detailed_end - detailed_start);
			
			suite.add_result(filename, detailed_success, detailed_duration);
		}
	}

	suite.finish();
	return suite.passed();
}