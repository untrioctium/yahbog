#pragma once

#include <memory>

#include <yahbog/registers.h>
#include <yahbog/mmu.h>
#include <yahbog/operations.h>


namespace yahbog {
	class cpu {
	public:

		consteval static auto address_range() {
			return std::array{
				address_range_t<cpu>{ 0xFF04, 0xFF04, &cpu::read_member<&cpu::div>, &cpu::write_div },
				address_range_t<cpu>{ 0xFF05, 0xFF05, &cpu::read_member<&cpu::tima>, &cpu::write_member<&cpu::tima> },
				address_range_t<cpu>{ 0xFF06, 0xFF06, &cpu::read_member<&cpu::tma>, &cpu::write_member<&cpu::tma> },
				address_range_t<cpu>{ 0xFF07, 0xFF07, &cpu::read_register<&cpu::tac>, &cpu::write_register<&cpu::tac> },
				address_range_t<cpu>{ 0xFF0F, 0xFF0F, &cpu::read_register<&cpu::if_>, &cpu::write_register<&cpu::if_> },
				address_range_t<cpu>{ 0xFFFF, 0xFFFF, &cpu::read_register<&cpu::ie>, &cpu::write_register<&cpu::ie> }
			};
		}

		constexpr cpu(read_fn_t* read, write_fn_t* write) noexcept : mem_fns{ read, write } {}

		constexpr void reset() noexcept {
			reg.pc = 0x100;
			reg.sp = 0xFFFE;
			reg.ime = 0;
			reg.mupc = 0;
			m_cycles = 0;

			div = 0x00;
			tima = 0x00;
			tma = 0x00;
			tac.write(0x00);
		}

		// Fetches the next instruction over one machine cycle
		// This is only needed after resetting the CPU or handling an interrupt
		// as the instructions themselves will handle fetching the next instruction
		constexpr void prefetch() noexcept {
			reg.ir = mem_fns.read(reg.pc);
			reg.pc++;
			m_cycles++;
		}

		// Performs one machine cycle
		constexpr void cycle() noexcept {

			const auto old_ie = reg.ie;
			const auto old_ir = reg.ir;

			if(!reg.halted) {
				opcodes::map[reg.ir](reg, &mem_fns);
			}
			
			if(reg.halted && (ie.read() & if_.read())) {
				reg.halted = 0;
			}

			const bool instruction_ended = reg.mupc == 0 && old_ir != 0xCB;

			if(old_ie && instruction_ended && old_ir != 0xFB) {
				reg.ime = 1;
				reg.ie = 0;
			}

			if(m_cycles % 64 == 0) {
				div++;
			}

			if(tac.v.enable) {
				bool ticked = [this]() {
					switch(tac.v.clock_select) {
						case 0: return m_cycles % 256 == 0;
						case 1: return m_cycles % 4 == 0;
						case 2: return m_cycles % 16 == 0;
						case 3: return m_cycles % 64 == 0;
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

		constexpr void set_reader(read_fn_t& read) noexcept { mem_fns.read_ = &read; }
		constexpr void set_writer(write_fn_t& write) noexcept { mem_fns.write_ = &write; }

	private:

		constexpr void write_div([[maybe_unused]] uint16_t addr, uint8_t value) {
			div = 0;
		}

		template<auto RegisterPtr>
		constexpr uint8_t read_register([[maybe_unused]] uint16_t addr) {
			return (this->*RegisterPtr).read();
		}

		template<auto RegisterPtr>
		constexpr void write_register([[maybe_unused]] uint16_t addr, uint8_t value) {
			(this->*RegisterPtr).write(value);
		}

		template<auto MemberPtr>
		constexpr uint8_t read_member([[maybe_unused]] uint16_t addr) {
			return this->*MemberPtr;
		}

		template<auto MemberPtr>
		constexpr void write_member([[maybe_unused]] uint16_t addr, uint8_t value) {
			this->*MemberPtr = value;
		}

		std::size_t m_cycles = 0;

		mem_fns_t mem_fns{};

		registers reg{};

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