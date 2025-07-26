#pragma once

#include <vector>
#include <filesystem>

#include <yahbog/mmu.h>
#include <yahbog/cpu.h>
#include <yahbog/ppu.h>

namespace yahbog {

	struct rom_header_t {
		std::uint8_t entry_point[4];
		std::uint8_t nintendo_logo[48];
		std::uint8_t title[16];
		std::uint8_t manufacturer_code[4];
		std::uint8_t cgb_flag;
		std::uint8_t new_licensee_code[2];
		std::uint8_t sgb_flag;
		std::uint8_t type;
		std::uint8_t rom_size;
		std::uint8_t ram_size;
		std::uint8_t destination_code;
		std::uint8_t old_licensee_code;
		std::uint8_t version;
		std::uint8_t checksum;
	};

	class rom_t : public addressable {
	public:
		constexpr static auto address_range = address_range_t{0x0000, 0xBFFF};
		
		uint8_t read(uint16_t addr) override;
		void write(uint16_t addr, uint8_t value) override;

		const rom_header_t& header() const { return header_; }

		bool load_rom(const std::filesystem::path& path);
		bool load_rom(std::vector<std::uint8_t>&& data);

	private:

		constexpr static auto rom_bank_size = 0x4000; // 16KB
		constexpr static auto ram_bank_size = 0x2000; // 8KB

		std::vector<std::uint8_t> rom_data;
		std::vector<std::uint8_t> ext_ram;
		rom_header_t header_;
		std::size_t rom_bank;
		std::size_t ram_bank;
		bool ram_enabled;
	};

	using wram_t = simple_memory<0xC000, 0xDFFF>;
	using hram_t = simple_memory<0xFF80, 0xFFFE>;

	class emulator {

		wram_t wram;
		hram_t hram;
		rom_t rom;

		memory_dispatcher<0x10000, gpu, wram_t, hram_t, rom_t> mmu;
		cpu z80;
		gpu ppu;

		emulator() : z80(&mmu) {
			mmu.set_handler(&wram);
			mmu.set_handler(&hram);
			mmu.set_handler(&ppu);
			mmu.set_handler(&rom);
		}
	};

}