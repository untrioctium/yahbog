#include <yahbog-tests.h>
#include <yahbog/emulator.h>
#include <yahbog/opinfo.h>

#include <readerwritercircularbuffer.h>
#include <termcolor/termcolor.hpp>

#include <array>
#include <functional>
#include <fstream>
#include <iostream>
#include <vector>
#include <format>

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
	std::cout << "E: ";
	size_t max_sections = (std::max)(expected_sections.size(), actual_sections.size());
	
	for (size_t i = 0; i < max_sections; ++i) {
		if (i > 0) std::cout << " ";
		
		std::string exp_section = (i < expected_sections.size()) ? expected_sections[i] : "";
		std::string act_section = (i < actual_sections.size()) ? actual_sections[i] : "";
		
		if (exp_section != act_section) {
			std::cout << termcolor::green << exp_section << termcolor::reset;
		} else {
			std::cout << exp_section;
		}
	}
	std::cout << "\n";
	
	// Print actual line
	std::cout << "A: ";
	for (size_t i = 0; i < max_sections; ++i) {
		if (i > 0) std::cout << " ";
		
		std::string exp_section = (i < expected_sections.size()) ? expected_sections[i] : "";
		std::string act_section = (i < actual_sections.size()) ? actual_sections[i] : "";
		
		if (exp_section != act_section) {
			std::cout << termcolor::red << act_section << termcolor::reset;
		} else {
			std::cout << act_section;
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



static bool run_test(const std::filesystem::path& rom_path) {
	auto filename = rom_path.filename().string();
	std::println("Running test: {}", filename);

	auto number_start = filename.find_first_not_of("0");
	auto number_end = filename.find_first_of('-');

	if(number_start == std::string::npos || number_end == std::string::npos) {
		std::println("Invalid filename: {}", filename);
		return false;
	}

	auto number = filename.substr(number_start, number_end - number_start);
	auto log_filename = std::format("{}.zip", number);
	auto log_path = rom_path.parent_path() / log_filename;

	if (!std::filesystem::exists(log_path)) {
		std::println("Log file not found: {}", log_path.string());
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

	auto emu = std::make_unique<yahbog::emulator>();
	emu->hook_writing([](uint16_t addr, uint8_t value) {
		// ignore audio registers
		if(addr >= 0xFF10 && addr <= 0xFF3F) {
			return true;
		}

		// ignore serial transfer registers
		if(addr == 0xFF01 || addr == 0xFF02) {
			return true;
		}
		return false;
	});

	emu->hook_reading([](uint16_t addr) -> yahbog::emulator::reader_hook_response {
		// pin LCDC.LY to 0x90 for Gameboy Doctor log consistency
		if(addr == 0xFF44) {
			return std::uint8_t{0x90};
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

	std::size_t instruction_count = 0;
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
				emu->z80.cycle();
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
			constexpr auto format_interrupts = [](std::uint8_t reg) {
				return std::format("{}{}{}{}{}",
					(reg & 16) ? "J" : ".",
					(reg & 8) ? "S" : ".",
					(reg & 4) ? "T" : ".",
					(reg & 2) ? "L" : ".",
					(reg & 1) ? "V" : "."
				);
			};
			std::println("Test failed: {}", filename);
			std::println("Last instruction: {:02X} ({}) ({} executed)", next_instruction, yahbog::opinfo[next_instruction].name, instruction_count);
			std::println("Interrupts: IME:{} IF:{} IE:{}", cpu.r().ime, format_interrupts(emu->mmu.read(0xFF0F)), format_interrupts(emu->mmu.read(0xFFFF)));
			for(const auto& log : logs) {
				std::println("   {}", log);
			}
			print_colored_diff(expected, my_log);
			return false;
		}

		logs.push_back(std::move(my_log));
		if(logs.size() > max_logs) {
			logs.erase(logs.begin());
		}
	}

	return true;
}

bool run_blargg_cpu_instrs() {

	std::println("Running blargg CPU instruction tests with Gameboy Doctor logs");

	std::vector<std::filesystem::path> roms{};

	// find all .gb files in the cpu_instrs directory
	for(const auto& entry : std::filesystem::directory_iterator(BASE_DIR)) {
		if(entry.is_regular_file() && entry.path().extension() == ".gb") {
			roms.push_back(entry.path());
		}
	}

	std::sort(roms.begin(), roms.end());

	for(const auto& rom : roms) {
		if(!run_test(rom)) {
			return false;
		}
	}

	return true;
}