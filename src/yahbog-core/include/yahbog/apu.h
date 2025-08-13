#pragma once

#include <yahbog/registers.h>
#include <yahbog/mmu.h>

namespace yahbog {

	class apu_t {
	public:

		consteval static auto address_range() {
			return std::array{
				mem_helpers::make_member_accessor<0xFF10, &apu_t::nr10>(),
				mem_helpers::make_member_accessor<0xFF11, &apu_t::nr11>()
			};
		}

		constexpr void reset() noexcept {
			nr10.set_byte(0x80);
			nr11.set_byte(0xBF);
		}

	private:

		struct nr10_t {
			constexpr static std::uint8_t write_mask = 0b01111111;

			std::uint8_t step : 3;
			std::uint8_t direction : 1;
			std::uint8_t pace : 3;
			std::uint8_t _unused : 1;
		};

		io_register<nr10_t> nr10;

		struct nr11_t {
			constexpr static std::uint8_t write_mask = 0b01111111;
			std::uint8_t length : 6;
			std::uint8_t duty : 2;
		};

		io_register<nr11_t> nr11;		

	};

}