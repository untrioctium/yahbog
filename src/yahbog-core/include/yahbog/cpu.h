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
				address_range_t<cpu>{ 0xFF04, &mem_helpers::read_byte<&cpu::div>, &cpu::write_div /* cannot use normal helpers because of the special case for div*/ },
				mem_helpers::make_member_accessor<0xFF05, &cpu::tima>(),
				mem_helpers::make_member_accessor<0xFF06, &cpu::tma>(),
				mem_helpers::make_member_accessor<0xFF07, &cpu::tac>(),
				mem_helpers::make_member_accessor<0xFF0F, &cpu::if_>(),
				mem_helpers::make_member_accessor<0xFFFF, &cpu::ie>()
			};
		}

		constexpr cpu(mem_fns_t* mem_fns) noexcept : mem_fns(mem_fns) {}

		constexpr void reset() noexcept {

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

			div = 0xAB;
			tima = 0x00;
			tma = 0x00;
			tac.set_byte(0xF8);
			if_.set_byte(0xE1);
			ie.set_byte(0x00);
			internal_counter = 58;

		}

		// Fetches the next instruction over one machine cycle
		// This is only needed after resetting the CPU or handling an interrupt
		// as the instructions themselves will handle fetching the next instruction
		constexpr void prefetch() noexcept {
			reg.ir = mem_fns->read(reg.pc);
			reg.pc++;
			m_cycles++;
		}

		// Performs one machine cycle
		constexpr void cycle() noexcept {

			div = (internal_counter >> 6) & 0xFF;

			if(tac.v.enable) {
				bool ticked = [this]() {
					switch(tac.v.clock_select) {
						case 0: return internal_counter % 256 == 0;
						case 1: return internal_counter % 4 == 0;
						case 2: return internal_counter % 16 == 0;
						case 3: return internal_counter % 64 == 0;
						default: std::unreachable();
					}
				}();

				if(ticked) {
					if(tima == 0xFF) {
						tima = tma;
						if_.v.timer = 1;
					} else {
						tima++;
					}
				}
				
			}

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

			internal_counter++;
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
				&cpu::div,
				&cpu::tima,
				&cpu::tma,
				&cpu::tac,
				&cpu::ie,
				&cpu::if_,
			};
		}

	private:

		// div resets on any write, so we must handle it separately
		constexpr void write_div([[maybe_unused]] uint16_t addr, uint8_t value) {
			internal_counter = 0;
			div = 0;
		}

		std::uint64_t m_cycles = 0;

		mem_fns_t* mem_fns = nullptr;

		registers reg{};

		std::uint64_t internal_counter = 0;
		std::uint8_t div = 0x00;
		std::uint8_t tima = 0x00;
		std::uint8_t tma = 0x00;

		struct tac_t {
			constexpr static std::uint8_t read_mask = 0b00000111;
			constexpr static std::uint8_t write_mask = 0b00000111;
			
			std::uint8_t clock_select : 2;
			std::uint8_t enable : 1;

			std::uint8_t _unused : 5;
		};

		io_register<tac_t> tac{};

		struct ieif_t {
			constexpr static std::uint8_t read_mask = 0b00011111;
			constexpr static std::uint8_t write_mask = 0b00011111;

			std::uint8_t vblank : 1;
			std::uint8_t lcd_stat : 1;
			std::uint8_t timer : 1;
			std::uint8_t serial : 1;
			std::uint8_t joypad : 1;

			std::uint8_t _unused : 3;
		};

		io_register<ieif_t> ie{};
		io_register<ieif_t> if_{};
	};

}

#include <yahbog/tests/cpu_tests.h>