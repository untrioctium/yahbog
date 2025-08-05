#pragma once

#include <cstdint>
#include <span>
#include <filesystem>
#include <vector>

#include <yahbog/mmu.h>
#include <yahbog/utility/serialization.h>

namespace yahbog {

	struct rom_header_t {
		std::uint8_t entry_point[4];
		std::uint8_t nintendo_logo[48];
		std::uint8_t title[15];
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
		std::uint8_t global_checksum_low;
		std::uint8_t global_checksum_high;
	};

	namespace typeinfo {
		template<>
		inline constexpr std::size_t id_v<rom_header_t> = 0x0D;
	}

	static_assert(sizeof(rom_header_t) == 80, "rom_header_t must be 80 bytes");

	class rom_t : public serializable<rom_t> {
	public:
		consteval static auto address_range() {
			return std::array{
				address_range_t<rom_t>{0x0000, 0x3FFF, &rom_t::read_bank00, &rom_t::write_bank00},
				address_range_t<rom_t>{0x4000, 0x7FFF, &rom_t::read_banked, &rom_t::write_banked},
				address_range_t<rom_t>{0xA000, 0xBFFF, &rom_t::read_ext_ram, &rom_t::write_ext_ram}
			};
		}

		constexpr uint8_t read_bank00(uint16_t addr) { 
			ASSUME_IN_RANGE(addr, 0x0000, 0x3FFF);

			return rom_data[addr];
		}

		constexpr void write_bank00(uint16_t addr, uint8_t value) {}
		
		constexpr uint8_t read_banked(uint16_t addr) {
			ASSUME_IN_RANGE(addr, 0x4000, 0x7FFF);

			return rom_data[addr - 0x4000 + rom_bank_idx * rom_bank_size];
		}

		constexpr void write_banked(uint16_t addr, uint8_t value) {}

		constexpr uint8_t read_ext_ram(uint16_t addr) {
			ASSUME_IN_RANGE(addr, 0xA000, 0xBFFF);

			if(ram_enabled()) {
				return ext_ram[addr - 0xA000 + ram_bank_idx * ram_bank_size];
			}
			else return 0xFF;
		}

		constexpr void write_ext_ram(uint16_t addr, uint8_t value) {
			ASSUME_IN_RANGE(addr, 0xA000, 0xBFFF);

			if(ram_enabled()) {
				ext_ram[addr - 0xA000 + ram_bank_idx * ram_bank_size] = value;
			}
		}

		constexpr const rom_header_t& header() const { return header_; }

		bool load_rom(const std::filesystem::path& path);
		constexpr bool load_rom(std::vector<std::uint8_t>&& data);

		consteval static auto serializable_members() {
			return std::tuple{
				&rom_t::rom_data,
				&rom_t::ext_ram,
				&rom_t::header_,
				&rom_t::rom_bank_idx,
				&rom_t::ram_bank_idx,
				&rom_t::crc32_checksum
			};
		}
	private:

		constexpr bool ram_enabled() const { return ram_bank_idx != (std::numeric_limits<std::uint16_t>::max)(); }

		constexpr static auto rom_bank_size = 0x4000; // 16KB
		constexpr static auto ram_bank_size = 0x2000; // 8KB

		std::vector<std::uint8_t> rom_data;
		std::vector<std::uint8_t> ext_ram;
		rom_header_t header_;
		std::uint16_t rom_bank_idx = 1;
		std::uint16_t ram_bank_idx = (std::numeric_limits<std::uint16_t>::max)();
		crc32_t crc32_checksum;
	};

}

#include <yahbog/impl/rom_impl.h>