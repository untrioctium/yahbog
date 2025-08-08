#pragma once

#include <yahbog/ppu.h>
#include <utility>

namespace yahbog {

	constexpr void gpu::tick() noexcept {

		dma_tick();

		if(!lcdc.v.lcd_display) [[unlikely]] {
			return;
		}

		mode_clock ++;
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
					swap_buffers();

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

	constexpr static auto extract_bits(std::uint8_t value, std::uint8_t start, std::uint8_t size) noexcept {
		return (value >> start) & ((1 << size) - 1);
	}

	constexpr void gpu::render_scanline() noexcept
	{
		// Each BG map row is 32 bytes wide; include the row stride in the offset
		const auto map_offset = (lcdc.v.bg_tile_map ? 0x1C00 : 0x1800)
			+ ((((ly + scy) & 255) >> 3) * 32);
		auto line_offset = static_cast<std::uint32_t>(scx) >> 3u;

		const auto tile_y = (ly + scy) & 7;
		auto tile_x = scx & 7;

		auto tile = static_cast<std::uint16_t>(vram[map_offset + line_offset]);

		// In 0x8800 addressing mode (bg_tile_data == 0), the tile index is signed
		if(!lcdc.v.bg_tile_data && tile < 128) tile += 256;

		for(std::size_t i = 0; i < 160; i++) {
			const auto palette_idx = tile_data[tile][tile_y][tile_x];
			const auto color_idx = extract_bits(bgp.as_byte(), palette_idx * 2, 2);

			set_pixel(i, ly, color_idx);

			tile_x++;

			if(tile_x == 8) {
				tile_x = 0;
				line_offset = (line_offset + 1) & 31;
				tile = vram[map_offset + line_offset];

				if(!lcdc.v.bg_tile_data && tile < 128) tile += 256;
			}
		}
		
	}
}