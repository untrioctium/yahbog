#pragma once

#include <yahbog/registers.h>
#include <yahbog/operations.h>
#include <yahbog/mmu.h>
#include <yahbog/utility/serialization.h>

namespace yahbog {

	enum class button_t : std::uint8_t {
		a = 0,
		b,
		select,
		start,
		right,
		left,
		up,
		down
	};

	enum class button_state_t {
		pressed = 0,
		released = 1
	};

	template<hardware_mode Mode>
	class io_t : public serializable<io_t<Mode>> {
	public:
		consteval static auto address_range() {
			return std::array{ 
				address_range_t<io_t>{0xFF00, &io_t::read_joypad, &mem_helpers::write_io_register<&io_t::joypad>},
				address_range_t<io_t>{0xFF01, &mem_helpers::read_byte<&io_t::serial_data>, &io_t::write_serial},
				mem_helpers::make_member_accessor<0xFF02, &io_t::sc>()
			};
		}

		constexpr void set_button_state(button_t button, button_state_t state) noexcept {
			if (state == button_state_t::pressed) {
				joypad_status &= ~(1 << static_cast<std::uint8_t>(button));
			} else {
				joypad_status |= (1 << static_cast<std::uint8_t>(button));
			}
		}

		consteval static auto serializable_members() {
			return std::tuple{
				&io_t::joypad,
				&io_t::joypad_status
			};
		}

		constexpr void reset() noexcept {
			joypad.set_byte(0xCF);
			sc.set_byte(0x7E);
		}

		constexpr std::uint8_t peek_serial() noexcept {
			return serial_fifo.peek();
		}

		constexpr std::uint8_t pop_serial() noexcept {
			return serial_fifo.pop();
		}

		constexpr bool serial_empty() noexcept {
			return serial_fifo.empty();
		}

	private:

		constexpr void write_serial(std::uint16_t addr, std::uint8_t value) noexcept {
			serial_fifo.push(value);
		}

		constexpr std::uint8_t read_joypad([[maybe_unused]] std::uint16_t addr) noexcept {
			std::uint8_t result = joypad.read() & 0b00001111;

			if (!joypad.v.read_buttons) {
				result &= joypad_status & 0b00001111;
			}

			if (!joypad.v.read_dpad) {
				result &= joypad_status >> 4;
			}

			return result | 0b11000000;;
		}

		struct joypad_t {
			constexpr static std::uint8_t read_mask = 0b00111111;
			constexpr static std::uint8_t write_mask = 0b00110000;

			std::uint8_t a_right : 1;
			std::uint8_t b_left : 1;
			std::uint8_t select_up : 1;
			std::uint8_t start_down : 1;

			std::uint8_t read_dpad : 1;
			std::uint8_t read_buttons : 1;

			std::uint8_t _unused : 2;
		};

		io_register<joypad_t> joypad;
		std::uint8_t joypad_status = 0b11111111;
		fifo<std::uint8_t, 16> serial_fifo;

		std::uint8_t serial_data = 0x00;

		struct sc_t {
			constexpr static std::uint8_t read_mask = 0b10000011;
			constexpr static std::uint8_t write_mask = 0b10000011;

			std::uint8_t clock_select : 1;
			std::uint8_t clock_speed : 1;
			std::uint8_t _unused : 5;
			std::uint8_t transfer_enable : 1;
		};

		io_register<sc_t> sc;


	};

}