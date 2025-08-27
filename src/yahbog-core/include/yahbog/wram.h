#pragma once

#include <array>

#include <yahbog/common.h>
#include <yahbog/utility/serialization.h>

namespace yahbog {

    template<hardware_mode Mode>
    class wram_t : public serializable<wram_t<Mode>> {
    public:
        consteval static auto address_range() {
            return std::array{
                address_range_t<wram_t>{0xC000, 0xCFFF, &wram_t::read_bank00, &wram_t::write_bank00},
                address_range_t<wram_t>{0xD000, 0xDFFF, &wram_t::read_banked, &wram_t::write_banked},
                address_range_t<wram_t>{0xE000, 0xFDFF, &wram_t::read_echo, &wram_t::write_echo},
                address_range_t<wram_t>{0xFF80, 0xFFFE, &wram_t::read_hram, &wram_t::write_hram},
                address_range_t<wram_t>{0xFF70, &wram_t::read_bank_idx, &wram_t::write_bank_idx}
            };
        }

        constexpr static auto wram_size = Mode == hardware_mode::dmg ? 0x2000 : 0x8000;

        std::array<uint8_t, wram_size> wram;
        std::array<uint8_t, 0x7F> hram{0};
        std::uint8_t ram_bank_idx = 1;

        consteval static auto serializable_members() {
            return std::tuple{
                &wram_t::wram,
                &wram_t::hram,
                &wram_t::ram_bank_idx
            };
        }

    private:

        constexpr static std::size_t bank_size = 0x1000; // 4kb

        constexpr uint8_t read_bank00(uint16_t addr) { 
            ASSUME_IN_RANGE(addr, 0xC000, 0xCFFF);

            return wram[addr - 0xC000]; 
        }
        constexpr void write_bank00(uint16_t addr, uint8_t value) { 
            ASSUME_IN_RANGE(addr, 0xC000, 0xCFFF);

            wram[addr - 0xC000] = value; 
        }

        constexpr uint8_t read_banked(uint16_t addr) {
            ASSUME_IN_RANGE(addr, 0xD000, 0xDFFF);
            ASSUME_IN_RANGE(ram_bank_idx, 1, 7);

            return wram[addr - 0xD000 + ram_bank_idx * bank_size];
        }

        constexpr void write_banked(uint16_t addr, uint8_t value) {
            ASSUME_IN_RANGE(addr, 0xD000, 0xDFFF);
            ASSUME_IN_RANGE(ram_bank_idx, 1, 7);

            wram[addr - 0xD000 + ram_bank_idx * bank_size] = value;
        }

        constexpr uint8_t read_echo(uint16_t addr) {
            ASSUME_IN_RANGE(addr, 0xE000, 0xFDFF);

            const std::uint16_t effective_addr = static_cast<std::uint16_t>(addr - 0x2000);
            if (effective_addr < 0xD000) {
                return wram[effective_addr - 0xC000];
            } else {
                ASSUME_IN_RANGE(ram_bank_idx, 1, 7);
                return wram[effective_addr - 0xD000 + ram_bank_idx * bank_size];
            }
        }

        constexpr void write_echo(uint16_t addr, uint8_t value) {
            ASSUME_IN_RANGE(addr, 0xE000, 0xFDFF);

            const std::uint16_t effective_addr = static_cast<std::uint16_t>(addr - 0x2000);
            if (effective_addr < 0xD000) {
                wram[effective_addr - 0xC000] = value;
            } else {
                ASSUME_IN_RANGE(ram_bank_idx, 1, 7);
                wram[effective_addr - 0xD000 + ram_bank_idx * bank_size] = value;
            }
        }

        constexpr uint8_t read_hram(uint16_t addr) { 
            ASSUME_IN_RANGE(addr, 0xFF80, 0xFFFE);

            return hram[addr - 0xFF80]; 
        }
        constexpr void write_hram(uint16_t addr, uint8_t value) { 
            ASSUME_IN_RANGE(addr, 0xFF80, 0xFFFE);

            hram[addr - 0xFF80] = value; 
        }

        constexpr uint8_t read_bank_idx([[maybe_unused]] uint16_t addr) { 
            if constexpr (Mode == hardware_mode::dmg) {
                return 0xFF;
            }
            return ram_bank_idx; 
        }
        constexpr void write_bank_idx([[maybe_unused]] uint16_t addr, uint8_t value) { 
            if constexpr (Mode == hardware_mode::dmg) {
                return;
            }
            ram_bank_idx = std::clamp(value, std::uint8_t{1}, std::uint8_t{7}); 
        }
    };

}