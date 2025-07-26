#include <format>

#include <operations.h>
#include <yahbog/cpu.h>

namespace yahbog {

	void cpu::reset() noexcept {
		reg.pc = 0x100;
		reg.sp = 0xFFFE;
		reg.ime = 1;
		reg.mupc = 0;
		m_cycles = 0;
	}

	void cpu::cycle() noexcept {
		opcodes::map[reg.ir](reg, mem);
	}

	void cpu::prefetch() noexcept {
		reg.ir = mem->read(reg.pc);
		reg.pc++;
		m_cycles++;
	}

	// outputs in the format:
	// A:00 F:11 B:22 C:33 D:44 E:55 H:66 L:77 SP:8888 PC:9999 PCMEM:AA,BB,CC,DD
	std::string cpu::gameboy_doctor_log() const {
		return std::format("A:{:02X} F:{:02X} B:{:02X} C:{:02X} D:{:02X} E:{:02X} H:{:02X} L:{:02X} SP:{:04X} PC:{:04X} PCMEM:{:02X},{:02X},{:02X},{:02X}",
			reg.a, reg.f, reg.b, reg.c, reg.d, reg.e, reg.h, reg.l, reg.sp, reg.pc - 1,
			mem->read(reg.pc - 1), mem->read(reg.pc), mem->read(reg.pc + 1), mem->read(reg.pc + 2));
	}

}