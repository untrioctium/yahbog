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
				address_range_t<rom_t>{0xA000, 0xBFFF, &rom_t::read_ext_ram, &rom_t::write_ext_ram},
			};
		}

		constexpr virtual ~rom_t() = default;

		constexpr virtual bool has_battery_backed_ram() const noexcept = 0;

		constexpr const rom_header_t& header() const { return header_; }

		static std::unique_ptr<rom_t> load_rom(const std::filesystem::path& path);
		constexpr static std::unique_ptr<rom_t> load_rom(std::vector<std::uint8_t>&& data);

		consteval static auto serializable_members() {
			return std::tuple{
				&rom_t::rom_data,
				&rom_t::ext_ram,
				&rom_t::header_,
				&rom_t::rom_bank_idx,
				&rom_t::ext_ram_bank_idx,
				&rom_t::crc32_checksum,
				&rom_t::impl_regs,
			};
		}

	protected:

		constexpr virtual std::uint8_t read_bank00(std::uint16_t addr) noexcept = 0;
		constexpr virtual std::uint8_t read_banked(std::uint16_t addr) noexcept = 0;
		constexpr virtual std::uint8_t read_ext_ram(std::uint16_t addr) noexcept = 0;
		constexpr virtual void write_ext_ram(std::uint16_t addr, std::uint8_t value) noexcept = 0;
		constexpr virtual void write_banked(std::uint16_t addr, std::uint8_t value) noexcept = 0;
		constexpr virtual void write_bank00(std::uint16_t addr, std::uint8_t value) noexcept = 0;

		constexpr static auto rom_bank_size = 0x4000; // 16KB
		constexpr static auto ext_ram_bank_size = 0x2000; // 8KB

		std::vector<std::uint8_t> rom_data;
		std::vector<std::uint8_t> ext_ram;
		rom_header_t header_;
		std::uint16_t rom_bank_idx = 1;
		std::uint16_t ext_ram_bank_idx = 0;
		std::uint8_t ext_ram_enabled = 0;
		crc32_t crc32_checksum;

		std::uint16_t num_rom_banks = 0;
		std::uint16_t num_ext_ram_banks = 0;

		// Implementation-specific registers
		std::array<std::uint8_t, 16> impl_regs;
	};

}