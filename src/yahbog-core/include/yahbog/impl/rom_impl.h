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

	constexpr bool rom_t::load_rom(std::vector<std::uint8_t>&& data) {
		if(data.size() < 0x1000) {
			return false;
		}
		
		rom_data = std::move(data);
		header_ = from_bytes<rom_header_t>({rom_data.data() + 0x0100, sizeof(rom_header_t)});

		auto ram_size = detail::calc_ram_size(header_.ram_size);
		if(ram_size > 0) {
			ext_ram.resize(ram_size);
		}

		rom_bank_idx = 1;
		ram_bank_idx = (std::numeric_limits<decltype(ram_bank_idx)>::max)();
		crc32_checksum = crc32({rom_data.data(), rom_data.size()});

		return true;
	}

}