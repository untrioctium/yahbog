#pragma once

#include <yahbog/registers.h>
#include <yahbog/mmu.h>
#include <yahbog/utility/serialization.h>
namespace yahbog {
/*    class timer_t {
    public:

    private:

        std::uint64_t internal_counter = 0;
        std::uint8_t div = 0x00;
        std::uint8_t tima = 0x00;
        std::uint8_t tma = 0x00;

        struct tac_t {
            constexpr static std::uint8_t read_mask = 0b00000111;
            constexpr static std::uint8_t write_mask = 0b00000111;
            
            std::uint8_t clock_select : 2;
            std::uint8_t enable : 1;

            std::uint8_t _unused : 5;
        };

        io_register<tac_t> tac{};
    };*/
}