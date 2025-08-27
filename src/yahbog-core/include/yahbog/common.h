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

	template<hardware_mode Mode, typename MemProvider>
	class cpu_t;

	template<hardware_mode Mode, typename MemProvider>
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

	template<hardware_mode Mode>
	class mmu_t;

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

	template<typename MemProvider>
    constexpr void request_interrupt(interrupt i, MemProvider* mem) noexcept;

	template<typename MemProvider>
	constexpr void clear_interrupt(interrupt i, MemProvider* mem) noexcept;

	template<typename T, std::size_t N>
	class fifo {
	public:

		constexpr void push(T value) noexcept {
			data[tail++] = value;
			if(tail == N) tail = 0;
		}

		constexpr T pop() noexcept {
			if(head == tail) return T{};
			const auto value = data[head++];
			if(head == N) head = 0;
			return value;
		}

		constexpr T peek() const noexcept {
			if(head == tail) return T{};
			return data[head];
		}

		constexpr bool empty() const noexcept { return head == tail; }
		constexpr bool full() const noexcept { return (tail + 1) % N == head; }

		constexpr std::size_t size() const noexcept {
			if(tail >= head) return tail - head;
			return N - (head - tail);
		}

		constexpr std::size_t capacity() const noexcept { return N; }
		
	private:
		std::array<T, N> data;
		std::size_t head = 0; // location of the next element to be read
		std::size_t tail = 0; // location of the next element to be written
	};

	template<typename F, typename Context>
	struct function_ref;

	template<typename R, typename... Args, typename Context>
	struct function_ref<R(Args...), Context> {
		R (*fn)(Context*, Args...);
		Context* ctx;

		constexpr function_ref(R (*fn)(Context*, Args...), Context* ctx) : fn(fn), ctx(ctx) {}

		constexpr R operator()(Args... args) const noexcept {
			return fn(ctx, std::forward<Args>(args)...);
		}
	};
}