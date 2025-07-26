#include <yahbog-tests.h>
#include <yahbog/emulator.h>

namespace {

	struct test_info {
		std::string_view name;

		std::string_view rom_src;
		std::string_view rom_path;
	};

	#define BASE_DIR TEST_DATA_DIR "/blargg/general"

	bool run_test(const std::filesystem::path& rom_path) {
		auto filename = rom_path.filename().string();
		std::println("Running test: {}", filename);

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

		while(true) {
			cpu.cycle();

			if(serial_data.ends_with("Passed")) {
				return true;
			} else if(serial_data.ends_with("Failed")) {
				std::println("Test failed: {}", filename);
				std::println("Serial data: {}", serial_data);
				return false;
			}
		}

		return true;
	}
}

bool run_blargg_general() {
	std::println("Running blargg general tests");

	std::vector<std::filesystem::path> roms{};

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