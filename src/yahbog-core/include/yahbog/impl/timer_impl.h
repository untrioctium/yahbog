#pragma once

#include <yahbog/timer.h>

namespace yahbog {

    template<hardware_mode Mode>
    constexpr void timer_t<Mode>::check_tick(std::uint32_t old_counter) {
        if(!tac.v.enable) {
			return;
		}

		constexpr std::uint8_t sel_bit[4]{9,3,5,7};
		const auto bit = sel_bit[tac.v.clock_select & 3u];

		const bool falling_edge = ((old_counter >> bit) & 1u) && !((internal_counter >> bit) & 1u);
		if(!falling_edge) return;

		if(tima == 0xFF) {
			tima = tma;
			request_interrupt(interrupt::timer);
		} else {
			tima++;
		}
    }

    template<hardware_mode Mode>
    constexpr void timer_t<Mode>::write_div([[maybe_unused]] uint16_t addr, [[maybe_unused]] uint8_t) {

		auto old_counter = internal_counter;
		internal_counter = 0;
		div = 0;

		check_tick(old_counter);
	}
}