#include <fstream>
#include <cstring>
#include <utility>

#include <yahbog/emulator.h>

namespace yahbog {

    std::size_t calc_ram_size(std::uint8_t code) {
        switch(code) {
            case 0x00: return 0;
            case 0x01: return 0;
            case 0x02: return 0x2000; // 8KB
            case 0x03: return 0x8000; // 32KB
            case 0x04: return 0x20000; // 128KB
            case 0x05: return 0x10000; // 64KB
            default: std::unreachable();
        }
    }

    bool rom_t::load_rom(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        return load_rom(std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), {}));
    }

    bool rom_t::load_rom(std::vector<std::uint8_t>&& data) {
        if(data.size() < 0x8000) {
            return false;
        }

        rom_data = std::move(data);
        std::memcpy(&header_, rom_data.data() + 0x0100, sizeof(rom_header_t));

        rom_bank = 1;
        ram_bank = 0;
        ram_enabled = false;

        auto ram_size = calc_ram_size(header_.ram_size);
        if(ram_size > 0) {
            ext_ram.resize(ram_size);
        }

        return true;
    }

    uint8_t rom_t::read(uint16_t addr) {
        if(addr < 0x4000) return rom_data[addr];
        else if(addr < 0x8000) return rom_data[addr - 0x4000 + rom_bank * rom_bank_size];
        else if(addr >= 0xA000 && addr < 0xC000) {
            if(ram_enabled) {
                auto offset = addr - 0xA000 + ram_bank * ram_bank_size;
                if(offset >= ext_ram.size()) offset %= ext_ram.size();
                return ext_ram[offset];
            }
            else return 0xFF;
        }
        else return 0xFF;
    }

    void rom_t::write(uint16_t addr, uint8_t value) {
        if(addr >= 0xA000 && addr < 0xC000) {
            if(ram_enabled) {
                auto offset = addr - 0xA000 + ram_bank * ram_bank_size;
                if(offset >= ext_ram.size()) offset %= ext_ram.size();
                ext_ram[offset] = value;
            }
        }
    }
}