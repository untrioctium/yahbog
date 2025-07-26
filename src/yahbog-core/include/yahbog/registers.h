#pragma once

#include <cstdint>
#include <bit>

namespace yahbog {
    struct registers {

        std::uint8_t a = 0;
        std::uint8_t f = 0;
        std::uint8_t b = 0;
        std::uint8_t c = 0;
        std::uint8_t d = 0;
        std::uint8_t e = 0;
        std::uint8_t h = 0;
        std::uint8_t l = 0;
        std::uint8_t w = 0;
        std::uint8_t z = 0;

        constexpr uint8_t FZ() const { return (f & 0x80) > 0; }
        constexpr uint8_t FN() const { return (f & 0x40) > 0; }
        constexpr uint8_t FH() const { return (f & 0x20) > 0; }
        constexpr uint8_t FC() const { return (f & 0x10) > 0; }

        constexpr void FZ(std::uint8_t val) { f = val ? f | 0x80 : f & ~0x80; }
        constexpr void FN(std::uint8_t val) { f = val ? f | 0x40 : f & ~0x40; }
        constexpr void FH(std::uint8_t val) { f = val ? f | 0x20 : f & ~0x20; }
        constexpr void FC(std::uint8_t val) { f = val ? f | 0x10 : f & ~0x10; }

        constexpr std::uint16_t af() const { return std::uint16_t(a << 8) | f; }
        constexpr std::uint16_t bc() const { return std::uint16_t(b << 8) | c; }
        constexpr std::uint16_t de() const { return std::uint16_t(d << 8) | e; }
        constexpr std::uint16_t hl() const { return std::uint16_t(h << 8) | l; }
        constexpr std::uint16_t wz() const { return std::uint16_t(w << 8) | z; }

        constexpr void af(std::uint16_t val) { a = val >> 8; f = val & 0xFF; }
        constexpr void bc(std::uint16_t val) { b = val >> 8; c = val & 0xFF; }
        constexpr void de(std::uint16_t val) { d = val >> 8; e = val & 0xFF; }
        constexpr void hl(std::uint16_t val) { h = val >> 8; l = val & 0xFF; }
        constexpr void wz(std::uint16_t val) { w = val >> 8; z = val & 0xFF; }

        std::uint16_t ir = 0;
        std::uint16_t sp = 0;
        std::uint16_t pc = 0;

        std::uint8_t ime = 0;
        std::uint8_t ie = 0;
        std::uint8_t mupc = 0;

        void reset() {
            a = 0x01;
            f = 0xB0;
            b = 0x00;
            c = 0x13;
            d = 0x00;
            e = 0xD8;
            h = 0x01;
            l = 0x4D;
            sp = 0xFFFE;
            pc = 0x0100;
            mupc = 0;
        }
    };

	template<typename Underlying>
		requires (sizeof(Underlying) == 1) &&
        requires (Underlying) {
            { Underlying::read_mask } -> std::convertible_to<std::uint8_t>;
            { Underlying::write_mask } -> std::convertible_to<std::uint8_t>;
        }
    struct io_register {
        Underlying value{};

		constexpr std::uint8_t read() const { return std::bit_cast<std::uint8_t>(value) & Underlying::read_mask; }
        constexpr void write(std::uint8_t val) { value = std::bit_cast<Underlying>(val & Underlying::write_mask); }

    };
}