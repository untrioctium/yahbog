#pragma once

#include <yahbog/cpu.h>

consteval bool basic_test() {

	auto rom = std::array<std::uint8_t, 256 + 3>{0x00};

	rom[0x100] = 0x06;
	rom[0x101] = 0x42;

	auto mem_fns = yahbog::mem_fns_t{};
	mem_fns.read = [&rom](uint16_t addr) { return rom[addr]; };
	mem_fns.write = [&rom](uint16_t addr, uint8_t value) { rom[addr] = value; };

	auto z80 = yahbog::cpu_t<yahbog::hardware_mode::dmg>{};

	z80.reset(&mem_fns);
	z80.prefetch(&mem_fns);
	
	while(z80.r().ir != 0x00) {
		z80.cycle(&mem_fns);
	}

	return z80.r().b == 0x42;
}

//static_assert(basic_test());