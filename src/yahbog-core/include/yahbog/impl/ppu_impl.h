#pragma once

#include <yahbog/ppu.h>
#include <utility>

namespace yahbog {

	constexpr void gpu::tick(std::uint8_t cycles) {
		mode_clock += cycles;
		switch (mode) {
		case mode_t::oam:
			if (mode_clock >= 80) {
				mode_clock = 0;
				mode = mode_t::vram;
			}
			break;
		case mode_t::vram:
			if (mode_clock >= 172) {
				mode_clock = 0;
				mode = mode_t::hblank;

				render_scanline();
			}
			break;
		case mode_t::hblank:
			if (mode_clock >= 204) {
				mode_clock = 0;
				ly++;
				if (ly == 144) {
					mode = mode_t::vblank;
				}
				else {
					mode = mode_t::oam;
				}
			}
			break;
		case mode_t::vblank:
			if (mode_clock >= 456) {
				mode_clock = 0;
				ly++;
				if (ly > 153) {
					ly = 0;
					mode = mode_t::oam;
				}
			}
			break;
		default: std::unreachable();
		}

	}

	constexpr void gpu::render_scanline()
	{
	}
}