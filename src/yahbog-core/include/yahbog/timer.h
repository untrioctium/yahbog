#pragma once

#include <cstdint>
#include <algorithm>
#include <array>
#include <yahbog/registers.h>
#include <yahbog/mmu.h>
#include <yahbog/utility/serialization.h>

namespace yahbog {

class timer_t : public serializable<timer_t> {
public:
	/*
	 * Memory-mapped IO address range for the Game Boy timers
	 *   FF04 – DIV   (Divider, read-only, write resets)
	 *   FF05 – TIMA  (Timer counter)
	 *   FF06 – TMA   (Modulo)
	 *   FF07 – TAC   (Control)
	 */
	consteval static auto address_range() {
		return std::array{
			address_range_t<timer_t>{0xFF04, &mem_helpers::read_byte<&timer_t::div>, &timer_t::write_div},
			mem_helpers::make_member_accessor<0xFF05, &timer_t::tima>(),
			mem_helpers::make_member_accessor<0xFF06, &timer_t::tma>(),
			mem_helpers::make_member_accessor<0xFF07, &timer_t::tac>()
		};
	}

	constexpr explicit timer_t(mem_fns_t* mem) noexcept : mem(mem) {
		// Power-on state observed on DMG/MGB
		internal_counter = (0xABu << 8) | 0xD0u; //   DIV=AB  sub-div(/4)=0x34
		div = static_cast<std::uint8_t>(internal_counter >> 8);
		tac.set_byte(0xF8);
	}

	// Called once per T-cycle (4 MHz) by the scheduler
	constexpr void tick() noexcept {
		const auto old_counter = internal_counter;
		internal_counter++;

		// Update DIV from bit 8-15
		div = static_cast<std::uint8_t>((internal_counter >> 8) & 0xFFu);

		check_tick(old_counter);
	}

	constexpr void check_tick(std::uint32_t old_counter) noexcept {
		if(!tac.v.enable) {
			return;
		}

		constexpr std::uint8_t sel_bit[4]{9,3,5,7};
		const auto bit = sel_bit[tac.v.clock_select & 3u];

		const bool falling_edge = ((old_counter >> bit) & 1u) && !((internal_counter >> bit) & 1u);
		if(!falling_edge) return;

		if(tima == 0xFF) {
			tima = tma;
			request_interrupt();
		} else {
			tima++;
		}
	}


	/* ----- serialization support ----- */
	consteval static auto serializable_members() {
		return std::tuple{
			&timer_t::internal_counter,
			&timer_t::div,
			&timer_t::tima,
			&timer_t::tma,
			&timer_t::tac
		};
	}

private:
	mem_fns_t* mem = nullptr;

	std::uint32_t internal_counter{0}; // 16-bit is enough but 32 keeps overflow defined
	std::uint8_t div{0};
	std::uint8_t tima{0};
	std::uint8_t tma{0};

	struct tac_t {
		static constexpr std::uint8_t read_mask  = 0b0000'0111;
		static constexpr std::uint8_t write_mask = 0b0000'0111;

		std::uint8_t clock_select : 2;
		std::uint8_t enable       : 1;
		std::uint8_t _unused      : 5;
	};
	io_register<tac_t> tac{};

	// ------ MMIO helpers ------
	constexpr void write_div([[maybe_unused]] uint16_t addr, [[maybe_unused]] uint8_t) {

		auto old_counter = internal_counter;
		internal_counter = 0;
		div = 0;

		check_tick(old_counter);
	}

	constexpr void request_interrupt() noexcept {
		// Set bit 2 in IF (0xFF0F)
		const auto if_val = mem->read(0xFF0F);
		mem->write(0xFF0F, static_cast<std::uint8_t>(if_val | 0x04));
	}
};

} // namespace yahbog