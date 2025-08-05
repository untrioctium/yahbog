#pragma once

#include <yahbog/ppu.h>
#include <utility>

namespace yahbog {

	constexpr void gpu::tick(std::uint8_t cycles) noexcept {

		if(!lcdc.v.lcd_display) [[unlikely]] {
			return;
		}

		mode_clock += cycles;
		switch (lcd_status.v.mode) {
		case mode_t::oam: // mode 2
			if (mode_clock >= 80) {
				mode_clock = 0;
				lcd_status.v.mode = mode_t::vram;
			}
			break;
		case mode_t::vram: // mode 3
			if (mode_clock >= 172) {
				mode_clock = 0;
				lcd_status.v.mode = mode_t::hblank;

				if(lcd_status.v.mode_hblank) {
					request_interrupt(interrupt::stat, mem_fns);
				}

				render_scanline();
			}
			break;
		case mode_t::hblank: // mode 0
			if (mode_clock >= 204) {
				mode_clock = 0;
				ly++;
				if (ly == 144) {
					lcd_status.v.mode = mode_t::vblank;

					request_interrupt(interrupt::vblank, mem_fns);

					if(lcd_status.v.mode_vblank) {
						request_interrupt(interrupt::stat, mem_fns);
					}
				}
				else {
					lcd_status.v.mode = mode_t::oam;

					if(lcd_status.v.mode_oam) {
						request_interrupt(interrupt::stat, mem_fns);
					}
				}
			}
			break;
		case mode_t::vblank: // mode 1
			if (mode_clock >= 456) {
				mode_clock = 0;
				ly++;
				if (ly > 153) {
					ly = 0;
					lcd_status.v.mode = mode_t::oam;

					if(ly < 144 && lcd_status.v.mode_oam) {
						request_interrupt(interrupt::stat, mem_fns);
					}
				}
			}
			break;
		default: std::unreachable();
		}

		const bool prev_coincidence = lcd_status.v.coincidence;
		lcd_status.v.coincidence = lyc == ly;

		if(!prev_coincidence && lcd_status.v.coincidence && lcd_status.v.lyc_condition) {
			request_interrupt(interrupt::stat, mem_fns);
		}
	}

	constexpr void gpu::render_scanline() noexcept
	{
		const auto map_offset = (lcdc.v.bg_tile_map ? 0x1C00 : 0x1800) + (((ly + scy) & 255) >> 3);
		const auto line_offset = scx >> 3;

		const auto tile_y = (ly + scy) & 7;
		const auto tile_x = scx & 7;
	}
}