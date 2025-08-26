#pragma once

#include <cstdint>
#include <cassert>

#include <yahbog/utility/constexpr_function.h>
#include <yahbog/utility/traits.h>

// optimization hint to the compiler to assume the value is in the range [start, end]
#define ASSUME_IN_RANGE(value, start, end) assert(value >= (start) && value <= (end)); [[assume(value >= (start))]]; [[assume(value <= (end))]]

namespace yahbog {

    enum class hardware_mode {
		dmg,
		cgb
	};

    enum class bus_state : std::uint8_t {
		normal,
		dma_blocked
	};

	template<hardware_mode Mode>
	class cpu_t;

	template<hardware_mode Mode>
	class ppu_t;

	template<hardware_mode Mode>
	class apu_t;

	template<hardware_mode Mode>
	class timer_t;

	template<hardware_mode Mode>
	class wram_t;

	template<hardware_mode Mode>
	class emulator_t;

	template<hardware_mode Mode>
	class io_t;

	class rom_t;

	class mmu_t;

    using read_fn_t = yahbog::constexpr_function<uint8_t(uint16_t)>;
	using write_fn_t = yahbog::constexpr_function<void(uint16_t, uint8_t)>;

    struct mem_fns_t {
		read_fn_t read;
		write_fn_t write;

		bus_state state;
	}; 

    template<typename T>
	struct address_range_t {
		uint16_t start;
		uint16_t end;

		using read_fn_member_t = uint8_t (T::*)(uint16_t);
		using write_fn_member_t = void (T::*)(uint16_t, uint8_t);

		using read_fn_ext_t = uint8_t (*)(T*, uint16_t);
		using write_fn_ext_t = void (*)(T*, uint16_t, uint8_t);

		using read_fn_t = std::variant<read_fn_member_t, read_fn_ext_t>;
		using write_fn_t = std::variant<write_fn_member_t, write_fn_ext_t>;

		read_fn_t read_fn;
		write_fn_t write_fn;

		consteval address_range_t(uint16_t start, uint16_t end, read_fn_t read_fn, write_fn_t write_fn) : start(start), end(end), read_fn(read_fn), write_fn(write_fn) {}
		consteval address_range_t(uint16_t addr, read_fn_t read_fn, write_fn_t write_fn) : address_range_t(addr, addr, read_fn, write_fn) {}

	};

    enum class interrupt : std::uint8_t {
		vblank = 0x00,
		stat = 0x01,
		timer = 0x02,
		serial = 0x03,
		joypad = 0x04,
	};

    constexpr void request_interrupt(interrupt i, mem_fns_t* mem) noexcept {
		mem->write(0xFF0F, mem->read(0xFF0F) | (1 << static_cast<std::uint8_t>(i)));
	}

	constexpr void clear_interrupt(interrupt i, mem_fns_t* mem) noexcept {
		mem->write(0xFF0F, mem->read(0xFF0F) & ~(1 << static_cast<std::uint8_t>(i)));
	}

}