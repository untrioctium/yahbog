#pragma once

#include <yahbog/emulator.h>
#include <yahbog/opinfo.h>

#include <yahbog/impl/mmu_impl.h>

#include <yahbog/impl/apu_impl.h>
#include <yahbog/impl/cpu_impl.h>
#include <yahbog/impl/emulator_impl.h>
#include <yahbog/impl/io_impl.h>
#include <yahbog/impl/ppu_impl.h>
#include <yahbog/impl/rom_impl.h>
#include <yahbog/impl/timer_impl.h>

namespace yahbog {

    template<typename MemProvider>
    constexpr void request_interrupt(interrupt i, MemProvider* mem) noexcept {
        mem->write(0xFF0F, mem->read(0xFF0F) | (1 << static_cast<std::uint8_t>(i)));
    }

    template<typename MemProvider>
    constexpr void clear_interrupt(interrupt i, MemProvider* mem) noexcept {
        mem->write(0xFF0F, mem->read(0xFF0F) & ~(1 << static_cast<std::uint8_t>(i)));
    }
}