#pragma once

#include <yahbog/cpu.h>

namespace yahbog {

	template<hardware_mode Mode>
	constexpr void cpu_t<Mode>::reset(mem_fns_t* mem_fns) noexcept {
        reg.a = 0x01;
        reg.b = 0x00;
        reg.c = 0x13;
        reg.d = 0x00;
        reg.e = 0xD8;
        reg.h = 0x01;
        reg.l = 0x4D;

        reg.pc = 0x101;
        reg.ir = mem_fns->read(0x100);
        m_next = opcodes::map[reg.ir].entry;
        reg.sp = 0xFFFE;
        reg.ime = 0;
        reg.mupc = 0;
        m_cycles = 0;

        if_.set_byte(0xE1);
        ie.set_byte(0x00);
	}

	template<hardware_mode Mode>
	constexpr void cpu_t<Mode>::cycle(mem_fns_t* mem_fns) noexcept {
        const auto old_ie = reg.ie;
        const auto old_ir = reg.ir;

        if(!reg.halted) {
            m_next = m_next(reg, mem_fns);
        }
        
        if(reg.halted && (ie.read() & if_.read())) {
            reg.halted = 0;
        }

        const bool instruction_ended = reg.mupc == 0 && old_ir != 0xCB;

        if(old_ie && instruction_ended && old_ir != 0xFB) {
            reg.ime = 1;
            reg.ie = 0;
        }

        if(instruction_ended && reg.ime) {
            if(ie.v.vblank && if_.v.vblank) {
                if_.v.vblank = 0;
                reg.ir = 0xE3;
                m_next = opcodes::map[0xE3].entry;
            } else if(ie.v.lcd_stat && if_.v.lcd_stat) {
                if_.v.lcd_stat = 0;
                reg.ir = 0xE4;
                m_next = opcodes::map[0xE4].entry;
            } else if(ie.v.timer && if_.v.timer) {
                if_.v.timer = 0;
                reg.ir = 0xEB;
                m_next = opcodes::map[0xEB].entry;
            } else if(ie.v.serial && if_.v.serial) {
                if_.v.serial = 0;
                reg.ir = 0xEC;
                m_next = opcodes::map[0xEC].entry;
            } else if(ie.v.joypad && if_.v.joypad) {
                if_.v.joypad = 0;
                reg.ir = 0xED;
                m_next = opcodes::map[0xED].entry;
            }
        }

        m_cycles++;
	}
	
}