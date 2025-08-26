#pragma once

#include <memory>

#include <yahbog/registers.h>
#include <yahbog/mmu.h>
#include <yahbog/operations.h>
#include <yahbog/utility/serialization.h>
#include <yahbog/common.h>

namespace yahbog {

	template<hardware_mode Mode>
	class cpu_t : public serializable<cpu_t<Mode>> {
	public:

		consteval static auto address_range() {
			return std::array{
				mem_helpers::make_member_accessor<0xFF0F, &cpu_t::if_>(),
				mem_helpers::make_member_accessor<0xFFFF, &cpu_t::ie>()
			};
		}

		constexpr void reset(mem_fns_t* mem_fns) noexcept;

		// Fetches the next instruction over one machine cycle
		// This is only needed after resetting the CPU or handling an interrupt
		// as the instructions themselves will handle fetching the next instruction
		constexpr void prefetch(mem_fns_t* mem_fns) noexcept {
			reg.ir = mem_fns->read(reg.pc);
			m_next = opcodes::map[reg.ir].entry;
			reg.pc++;
			m_cycles++;
		}

		// Performs one machine cycle
		constexpr void cycle(mem_fns_t* mem_fns) noexcept;

		constexpr auto& r() const noexcept { return reg; }
		constexpr auto cycles() const noexcept { return m_cycles; }

		constexpr void load_registers(const registers& r) noexcept { 
			reg = r; 
			reg.ir &= 0x1FF;
		}


		consteval static auto serializable_members() {
			return std::tuple{
				&cpu_t::reg,
				&cpu_t::m_cycles,
				&cpu_t::ie,
				&cpu_t::if_,
			};
		}

	private:

		std::uint64_t m_cycles = 0;
		next_stage_t m_next = opcodes::map[0x00].entry;

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