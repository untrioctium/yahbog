#pragma once

#include <vector>
#include <filesystem>
#include <span>
#include <variant>

#include <yahbog/mmu.h>
#include <yahbog/cpu.h>
#include <yahbog/ppu.h>
#include <yahbog/rom.h>

namespace yahbog {

	class emulator {
	public:
		using wram_t = simple_memory<0xC000, 0xDFFF>;
		using hram_t = simple_memory<0xFF80, 0xFFFE>;

		read_fn_t reader;
		write_fn_t writer;

		wram_t wram;
		hram_t hram;
		rom_t rom;
		cpu z80;
		gpu ppu;

		memory_dispatcher<0x10000, gpu, wram_t, hram_t, rom_t, cpu> mmu;

		constexpr emulator() : 
			reader(default_reader()),
			writer(default_writer()),
			z80(&reader, &writer)
			{
				mmu.set_handler(&wram);
				mmu.set_handler(&hram);
				mmu.set_handler(&ppu);
				mmu.set_handler(&rom);
				mmu.set_handler(&z80);
			}

		constexpr read_fn_t default_reader() noexcept {
			return [this](std::uint16_t addr) { return mmu.read(addr); };
		}

		constexpr write_fn_t default_writer() noexcept {
			return [this](std::uint16_t addr, std::uint8_t value) { mmu.write(addr, value); };
		}

		using reader_hook_response = std::variant<std::monostate, std::uint8_t>;

		constexpr void hook_reading(constexpr_function<reader_hook_response(std::uint16_t)>&& hook) noexcept {
			auto base_reader = default_reader();
			reader = [next = default_reader(), hook = std::move(hook)](std::uint16_t addr) {
				auto response = hook(addr);
				if (std::holds_alternative<std::uint8_t>(response)) {
					return std::get<std::uint8_t>(response);
				}
				return next(addr);
			};
		}

		constexpr void hook_writing(constexpr_function<bool(std::uint16_t, std::uint8_t)>&& hook) noexcept {
			auto base_writer = default_writer();
			writer = [next = default_writer(), hook = std::move(hook)](std::uint16_t addr, std::uint8_t value) {
				if (hook(addr, value)) {
					return;
				}
				next(addr, value);
			};
		}

	private:

	};

}