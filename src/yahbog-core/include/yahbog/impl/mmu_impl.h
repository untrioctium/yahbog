#pragma once

#include <tuple>
#include <array>

#include <yahbog/cpu.h>
#include <yahbog/rom.h>
#include <yahbog/ppu.h>
#include <yahbog/apu.h>
#include <yahbog/timer.h>
#include <yahbog/io.h>
#include <yahbog/wram.h>

#include <yahbog/mmu.h>

namespace yahbog {

    namespace mmu_impl {

        template<hardware_mode Mode>
        using storage_t = std::tuple<
            ppu_t<Mode, mmu_t<Mode>>,
            wram_t<Mode>,
            std::unique_ptr<rom_t>,
            timer_t<Mode>,
            cpu_t<Mode, mmu_t<Mode>>,
            io_t<Mode>,
            apu_t<Mode>
        >;

    }

}