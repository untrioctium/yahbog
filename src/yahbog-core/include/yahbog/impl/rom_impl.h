#pragma once

#include <yahbog/rom.h>

namespace yahbog {

    constexpr rom_header_t rom_header_t::from_bytes(std::span<std::uint8_t> data) {
        rom_header_t header;
        
        #define COPY_FIELD(field) header.field = data[offsetof(rom_header_t, field)]
        #define COPY_FIELD_ARRAY(field) do { for(std::size_t i = 0; i < sizeof(header.field); i++) header.field[i] = data[offsetof(rom_header_t, field) + i]; } while(false)

        COPY_FIELD_ARRAY(entry_point);
        COPY_FIELD_ARRAY(nintendo_logo);
        COPY_FIELD_ARRAY(title);
        COPY_FIELD_ARRAY(manufacturer_code);
        COPY_FIELD(cgb_flag);
        COPY_FIELD_ARRAY(new_licensee_code);
        COPY_FIELD(sgb_flag);
        COPY_FIELD(type);
        COPY_FIELD(rom_size);
        COPY_FIELD(ram_size);
        COPY_FIELD(destination_code);
        COPY_FIELD(old_licensee_code);
        COPY_FIELD(version);
        COPY_FIELD(checksum);

        #undef COPY_FIELD
        #undef COPY_FIELD_ARRAY

        return header;
	}

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
        if(data.size() < 0x8000) {
            return false;
        }
        
        rom_data = std::move(data);
        header_ = rom_header_t::from_bytes({rom_data.data() + 0x0100, sizeof(rom_header_t)});

        auto ram_size = detail::calc_ram_size(header_.ram_size);
        if(ram_size > 0) {
            ext_ram.resize(ram_size);
        }

        rom_bank = 1;
        ram_bank = std::numeric_limits<std::size_t>::max();

        return true;
    }

}