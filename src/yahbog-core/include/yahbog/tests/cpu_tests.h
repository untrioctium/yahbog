#pragma once

#include <yahbog/cpu.h>

consteval bool basic_test() {

	auto rom = std::array<std::uint8_t, 256 + 3>{0x00};

	rom[0x100] = 0x06;
	rom[0x101] = 0x42;

	auto reader = yahbog::read_fn_t{ [&rom](uint16_t addr) { return rom[addr]; } };
	auto writer = yahbog::write_fn_t{ [&rom](uint16_t addr, uint8_t value) { rom[addr] = value; } };

	auto z80 = yahbog::cpu{ &reader, &writer };

	z80.reset();
	z80.prefetch();
	
	while(z80.r().ir != 0x00) {
		z80.cycle();
	}

	return z80.r().b == 0x42;
}

//static_assert(basic_test());