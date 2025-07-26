#pragma once

#include <cstdint>
#include <span>
#include <filesystem>
#include <vector>

#include <yahbog/mmu.h>

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

		constexpr static rom_header_t from_bytes(std::span<std::uint8_t> data);
	};

	class rom_t {
	public:
		consteval static auto address_range() {
			return std::array{
				address_range_t<rom_t>{0x0000, 0x3FFF, &rom_t::read_bank00, &rom_t::write_bank00},
				address_range_t<rom_t>{0x4000, 0x7FFF, &rom_t::read_banked, &rom_t::write_banked},
				address_range_t<rom_t>{0xA000, 0xBFFF, &rom_t::read_ext_ram, &rom_t::write_ext_ram}
			};
		}

		constexpr uint8_t read_bank00(uint16_t addr) { return rom_data[addr];}
		constexpr void write_bank00(uint16_t addr, uint8_t value) {}
		
		constexpr uint8_t read_banked(uint16_t addr) {
			return rom_data[addr - 0x4000 + rom_bank * rom_bank_size];
		}

		constexpr void write_banked(uint16_t addr, uint8_t value) {}

		constexpr uint8_t read_ext_ram(uint16_t addr) {
			if(ram_enabled()) {
				return ext_ram[addr - 0xA000 + ram_bank * ram_bank_size];
			}
			else return 0xFF;
		}

		constexpr void write_ext_ram(uint16_t addr, uint8_t value) {
			if(ram_enabled()) {
				ext_ram[addr - 0xA000 + ram_bank * ram_bank_size] = value;
			}
		}
		

		constexpr const rom_header_t& header() const { return header_; }

		bool load_rom(const std::filesystem::path& path);
		constexpr bool load_rom(std::vector<std::uint8_t>&& data);

	private:

		constexpr bool ram_enabled() const { return ram_bank != (std::numeric_limits<std::size_t>::max)(); }

		constexpr static auto rom_bank_size = 0x4000; // 16KB
		constexpr static auto ram_bank_size = 0x2000; // 8KB

		std::vector<std::uint8_t> rom_data;
		std::vector<std::uint8_t> ext_ram;
		rom_header_t header_;
		std::size_t rom_bank;
		std::size_t ram_bank = (std::numeric_limits<std::size_t>::max)();
	};

}

#include <yahbog/impl/rom_impl.h>