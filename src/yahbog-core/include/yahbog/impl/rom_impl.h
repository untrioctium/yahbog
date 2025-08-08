#pragma once

#include <yahbog/rom.h>

namespace yahbog {

	namespace detail {
		constexpr std::size_t calc_ram_size(std::uint8_t code) {
			switch(code) {
				case 0x00: return 0;
				case 0x01: return 0;
				case 0x02: return 0x2000; // 8 KB
				case 0x03: return 0x8000; // 32 KB
				case 0x04: return 0x20000; // 128 KB
				case 0x05: return 0x10000; // 64 KB
				default: return 0;
			}
		}
	}

	constexpr std::unique_ptr<rom_t> mbc_factory(std::uint8_t mbc_type);

	constexpr std::unique_ptr<rom_t> rom_t::load_rom(std::vector<std::uint8_t>&& data) {

		if(data.size() < 0x1000) {
			return nullptr;
		}

		constexpr auto mbc_byte_idx = 0x100 + offsetof(rom_header_t, type);
		const auto mbc_type = data[mbc_byte_idx];
		auto mbc = mbc_factory(mbc_type);

		if(!mbc) {
			return nullptr;
		}

		mbc->rom_data = std::move(data);

		mbc->header_ = from_bytes<rom_header_t>({mbc->rom_data.data() + 0x0100, sizeof(rom_header_t)});

		auto ext_ram_size = detail::calc_ram_size(mbc->header_.ram_size);
		if(ext_ram_size > 0 && mbc->ext_ram.size() == 0) {
			mbc->ext_ram.resize(ext_ram_size);
		}

		mbc->rom_bank_idx = 1;
		mbc->ext_ram_bank_idx = (std::numeric_limits<decltype(mbc->ext_ram_bank_idx)>::max)();
		mbc->crc32_checksum = crc32({mbc->rom_data.data(), mbc->rom_data.size()});

		return mbc;
	}

	class nombc_rom_t : public rom_t {
	public:
		constexpr nombc_rom_t() = default;
		constexpr ~nombc_rom_t() = default;

		constexpr bool has_battery_backed_ram() const noexcept override {
			return false;
		}

	private:
		constexpr std::uint8_t read_bank00(std::uint16_t addr) noexcept override {
			return rom_data[addr];
		}

		constexpr std::uint8_t read_banked(std::uint16_t addr) noexcept override {
			return rom_data[addr];
		}

		constexpr std::uint8_t read_ext_ram(std::uint16_t addr) noexcept override {
			if(!ext_ram_enabled()) {
				return 0xFF;
			}

			return ext_ram[addr - 0xA000 + ext_ram_bank_idx * ext_ram_bank_size];
		}

		constexpr void write_ext_ram(std::uint16_t addr, std::uint8_t value) noexcept override {
			if(!ext_ram_enabled()) {
				return;
			}

			ext_ram[addr - 0xA000 + ext_ram_bank_idx * ext_ram_bank_size] = value;
		}

		constexpr void write_banked(std::uint16_t addr, std::uint8_t value) noexcept override {
			if(value == 0x0A) {
				ext_ram_bank_idx = 0;
			}
			else ext_ram_bank_idx = ext_ram_disabled_value;
		}

		constexpr void write_bank00(std::uint16_t addr, std::uint8_t value) noexcept override {
			if(value == 0x0A) {
				ext_ram_bank_idx = 0;
			}
			else ext_ram_bank_idx = ext_ram_disabled_value;
		}
	};

	class mbc1_rom_t : public rom_t {
	public:
		constexpr mbc1_rom_t() {
			// Ensure implementation registers start from a known state
			for(auto &reg : impl_regs) reg = 0;
		}
		constexpr ~mbc1_rom_t() = default;

		constexpr bool has_battery_backed_ram() const noexcept override {
			// 0x03 indicates MBC1+RAM+BATTERY
			return header_.type == 0x03;
		}

	private:
		// impl_regs layout for MBC1
		// [0] = ROM bank low 5 bits (0-31)
		// [1] = Upper bits register (bits 0-1) used as ROM bank bits 5-6 or RAM bank (0-3)
		// [2] = Banking mode (0 = ROM banking, 1 = RAM banking)
		// [3] = Cached bank index for 0x0000-0x3FFF region (bank00 index)

		constexpr std::uint16_t num_rom_banks() const noexcept {
			return static_cast<std::uint16_t>(rom_data.size() / rom_bank_size);
		}

		constexpr void recompute_cached_banks() noexcept {
			const auto banks = num_rom_banks();
			// Compute banked area bank index (0x4000-0x7FFF)
			std::uint16_t banked = static_cast<std::uint16_t>(((impl_regs[1] & 0x03) << 5) | (impl_regs[0] & 0x1F));
			if((banked & 0x1F) == 0) {
				banked |= 0x01; // avoid forbidden banks
			}
			rom_bank_idx = banks ? static_cast<std::uint16_t>(banked % banks) : 1;

			// Compute bank00 index (0x0000-0x3FFF)
			std::uint16_t bank00 = ((impl_regs[2] & 0x01) == 0)
				? 0
				: static_cast<std::uint16_t>((impl_regs[1] & 0x03) << 5);
			std::uint16_t bank00_mod = banks ? static_cast<std::uint16_t>(bank00 % banks) : 0;
			impl_regs[3] = static_cast<std::uint8_t>(bank00_mod);
		}

		constexpr void update_ext_ram_bank_after_change() noexcept {
			if(!ext_ram_enabled()) {
				return;
			}
			const std::uint16_t ram_banks = static_cast<std::uint16_t>(ext_ram.size() / ext_ram_bank_size);
			if((impl_regs[2] & 0x01) == 0) {
				// ROM banking mode -> RAM bank 0 selected
				ext_ram_bank_idx = 0;
			} else {
				// RAM banking mode -> select by upper bits register (0-3)
				std::uint16_t bank = static_cast<std::uint16_t>(impl_regs[1] & 0x03);
				if(ram_banks != 0) {
					ext_ram_bank_idx = static_cast<std::uint16_t>(bank % ram_banks);
				} else {
					ext_ram_bank_idx = 0;
				}
			}
		}

		constexpr std::uint8_t read_bank00(std::uint16_t addr) noexcept override {
			return rom_data[addr + static_cast<std::uint16_t>(impl_regs[3]) * rom_bank_size];
		}

		constexpr std::uint8_t read_banked(std::uint16_t addr) noexcept override {
			// Use currently latched bank index for the banked region
			return rom_data[(addr - 0x4000) + rom_bank_idx * rom_bank_size];
		}

		constexpr std::uint8_t read_ext_ram(std::uint16_t addr) noexcept override {
			if(!ext_ram_enabled()) {
				return 0xFF;
			}
			return ext_ram[(addr - 0xA000) + ext_ram_bank_idx * ext_ram_bank_size];
		}

		constexpr void write_ext_ram(std::uint16_t addr, std::uint8_t value) noexcept override {
			if(!ext_ram_enabled()) {
				return;
			}
			ext_ram[(addr - 0xA000) + ext_ram_bank_idx * ext_ram_bank_size] = value;
		}

		constexpr void write_banked(std::uint16_t addr, std::uint8_t value) noexcept override {
			if(addr <= 0x5FFF) {
				// Upper bits register (bits 0-1)
				impl_regs[1] = static_cast<std::uint8_t>(value & 0x03);
				// Update cached mappings
				recompute_cached_banks();
				update_ext_ram_bank_after_change();
			}
			else {
				// Banking mode select (0 = ROM banking, 1 = RAM banking)
				impl_regs[2] = static_cast<std::uint8_t>(value & 0x01);
				recompute_cached_banks();
				update_ext_ram_bank_after_change();
			}
		}

		constexpr void write_bank00(std::uint16_t addr, std::uint8_t value) noexcept override {
			if(addr <= 0x1FFF) {
				// RAM enable (lower 4 bits must be 0x0A)
				if(((value & 0x0F) == 0x0A) && !ext_ram.empty()) {
					// Enable and select proper RAM bank
					if((impl_regs[2] & 0x01) == 0) {
						ext_ram_bank_idx = 0;
					} else {
						const std::uint16_t ram_banks = static_cast<std::uint16_t>(ext_ram.size() / ext_ram_bank_size);
						std::uint16_t bank = static_cast<std::uint16_t>(impl_regs[1] & 0x03);
						ext_ram_bank_idx = ram_banks ? static_cast<std::uint16_t>(bank % ram_banks) : 0;
					}
				} else {
					// Disable RAM
					ext_ram_bank_idx = ext_ram_disabled_value;
				}
			}
			else {
				// ROM bank low 5 bits
				impl_regs[0] = static_cast<std::uint8_t>(value & 0x1F);
				// Update cached mappings
				recompute_cached_banks();
			}
		}
	};

	class mbc2_rom_t : public rom_t {
	public:
		constexpr mbc2_rom_t() {
			// MBC2 has internal 512 x 4-bit RAM, mirrored across 0xA000-0xBFFF.
			// Use base ext_ram to hold 512 bytes (store only low nibble).
			ext_ram.resize(512);
			for(auto &reg : impl_regs) reg = 0;
			rom_bank_idx = 1;
			ext_ram_bank_idx = ext_ram_disabled_value;
		}

		constexpr ~mbc2_rom_t() = default;

		constexpr bool has_battery_backed_ram() const noexcept override {
			// 0x06 indicates MBC2+BATTERY
			return header_.type == 0x06;
		}

	private:
		constexpr std::uint16_t num_rom_banks() const noexcept {
			return static_cast<std::uint16_t>(rom_data.size() / rom_bank_size);
		}

		constexpr std::uint8_t read_bank00(std::uint16_t addr) noexcept override {
			// Fixed to bank 0
			return rom_data[addr];
		}

		constexpr std::uint8_t read_banked(std::uint16_t addr) noexcept override {
			return rom_data[(addr - 0x4000) + rom_bank_idx * rom_bank_size];
		}

		constexpr std::uint8_t read_ext_ram(std::uint16_t addr) noexcept override {
			if(!ext_ram_enabled()) {
				return 0xFF;
			}
			const std::size_t offset = static_cast<std::size_t>(addr & 0x01FF);
			return static_cast<std::uint8_t>(0xF0 | (ext_ram[offset] & 0x0F));
		}

		constexpr void write_ext_ram(std::uint16_t addr, std::uint8_t value) noexcept override {
			if(!ext_ram_enabled()) {
				return;
			}
			const std::size_t offset = static_cast<std::size_t>(addr & 0x01FF);
			ext_ram[offset] = static_cast<std::uint8_t>(value & 0b1111);
		}

		constexpr void write_banked(std::uint16_t, std::uint8_t) noexcept override {
			// MBC2 doesn't use 0x4000-0x7FFF control registers; ignore writes.
		}

		constexpr void write_bank00(std::uint16_t addr, std::uint8_t value) noexcept override {
			constexpr auto control_bit = 0b1'0000'0000;

			if(addr & control_bit) {
				// ROM bank select (low 4 bits)
				rom_bank_idx = static_cast<std::uint16_t>(value & 0x0F);
				if(rom_bank_idx == 0) {
					rom_bank_idx = 1;
				}
			}
			else {
				// RAM enable/disable
				if(value == 0x0A) {
					ext_ram_bank_idx = 0;
				}
				else {
					ext_ram_bank_idx = ext_ram_disabled_value;
				}
			}
		}
	};

	constexpr std::unique_ptr<rom_t> mbc_factory(std::uint8_t mbc_type) {
		switch(mbc_type) {
			case 0x00: return std::make_unique<nombc_rom_t>();
			case 0x01: // MBC1
			case 0x02: // MBC1+RAM
			case 0x03: // MBC1+RAM+BATTERY
				return std::make_unique<mbc1_rom_t>();
			case 0x05: // MBC2
			case 0x06: // MBC2+BATTERY
				return std::make_unique<mbc2_rom_t>();
			default: return nullptr;
		}
	}

}