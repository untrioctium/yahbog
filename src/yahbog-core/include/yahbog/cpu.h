#pragma once

#include <memory>

#include <yahbog/registers.h>
#include <yahbog/mmu.h>
#include <yahbog/operations.h>
#include <yahbog/utility/serialization.h>


namespace yahbog {
	class cpu : public serializable<cpu> {
	public:

		consteval static auto address_range() {
			return std::array{
				mem_helpers::make_member_accessor<0xFF0F, &cpu::if_>(),
				mem_helpers::make_member_accessor<0xFFFF, &cpu::ie>()
			};
		}

		constexpr void reset(mem_fns_t* mem_fns) noexcept {

			reg.a = 0x01;
			reg.b = 0x00;
			reg.c = 0x13;
			reg.d = 0x00;
			reg.e = 0xD8;
			reg.h = 0x01;
			reg.l = 0x4D;

			reg.pc = 0x101;
			reg.ir = mem_fns->read(0x100);
			reg.sp = 0xFFFE;
			reg.ime = 0;
			reg.mupc = 0;
			m_cycles = 0;

			if_.set_byte(0xE1);
			ie.set_byte(0x00);

		}

		// Fetches the next instruction over one machine cycle
		// This is only needed after resetting the CPU or handling an interrupt
		// as the instructions themselves will handle fetching the next instruction
		constexpr void prefetch(mem_fns_t* mem_fns) noexcept {
			reg.ir = mem_fns->read(reg.pc);
			reg.pc++;
			m_cycles++;
		}

		// Performs one machine cycle
		constexpr void cycle(mem_fns_t* mem_fns) noexcept {

			const auto old_ie = reg.ie;
			const auto old_ir = reg.ir;

			if(!reg.halted) {
				opcodes::map[reg.ir](reg, mem_fns);
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
				} else if(ie.v.lcd_stat && if_.v.lcd_stat) {
					if_.v.lcd_stat = 0;
					reg.ir = 0xE4;
				} else if(ie.v.timer && if_.v.timer) {
					if_.v.timer = 0;
					reg.ir = 0xEB;
				} else if(ie.v.serial && if_.v.serial) {
					if_.v.serial = 0;
					reg.ir = 0xEC;
				} else if(ie.v.joypad && if_.v.joypad) {
					if_.v.joypad = 0;
					reg.ir = 0xED;
				}
			}

			m_cycles++;
		}

		constexpr auto& r() const noexcept { return reg; }
		constexpr auto cycles() const noexcept { return m_cycles; }

		constexpr void load_registers(const registers& r) noexcept { 
			reg = r; 
			reg.ir &= 0x1FF;
		}


		consteval static auto serializable_members() {
			return std::tuple{
				&cpu::reg,
				&cpu::m_cycles,
				&cpu::ie,
				&cpu::if_,
			};
		}

	private:

		std::uint64_t m_cycles = 0;

		registers reg{};

		struct ieif_t {
			constexpr static std::uint8_t read_mask  = 0b0001'1111;
			constexpr static std::uint8_t write_mask = 0b0001'1111;

			std::uint8_t vblank   : 1;
			std::uint8_t lcd_stat : 1;
			std::uint8_t timer    : 1;
			std::uint8_t serial   : 1;
			std::uint8_t joypad   : 1;

			std::uint8_t _unused  : 3;
		};

		io_register<ieif_t> ie{};
		io_register<ieif_t> if_{};
	};

}

#include <yahbog/tests/cpu_tests.h>