#include <iostream>

#include <yahbog/ppu.h>

namespace yahbog {

	void gpu::tick(std::uint8_t cycles) {
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
		}

	}

	uint8_t gpu::read(uint16_t addr) {
		switch (addr & 0xF000) {
			case 0x8000:
			case 0x9000:
				return vram[addr - 0x8000];
			case 0xF000:
				switch (addr & 0x0F00) {
					case 0x0E00:
						return oam[addr - 0xFE00];
					case 0x0F00:
						switch (addr & 0x00FF) {
							case 0x0040:
								return lcdc.read();
							case 0x0041:
								return lcd_status.read();
							case 0x0042:
								return scy;
							case 0x0043:
								return scx;
							case 0x0044:
								return ly;
							case 0x0045:
								return lyc;
							case 0x0046:
								return dma;
							default: 
								std::cout << "(PPU) Unimplemented read from 0x" << std::hex << addr << std::endl;
								return 0;
						}
					default:
						std::cout << "(PPU) Unimplemented read from 0x" << std::hex << addr << std::endl;
						return 0;
				}
			default:
				std::cout << "(PPU) Unimplemented read from 0x" << std::hex << addr << std::endl;
				return 0;
			
		}
	}
	void gpu::write(uint16_t addr, uint8_t value) {
		switch (addr & 0xF000) {
		case 0x8000:
		case 0x9000:
			vram[addr - 0x8000] = value;
			break;
		case 0xFE00:
		case 0xFE9F:
			oam[addr - 0xFE00] = value;
			break;
		}
	}

	void gpu::render_scanline()
	{
	}
}