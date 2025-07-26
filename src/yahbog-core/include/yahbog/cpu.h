#pragma once

#include <memory>
#include <string>

#include <yahbog/registers.h>
#include <yahbog/mmu.h>

namespace yahbog {
	class cpu {
	public:

		explicit cpu(addressable* mem) noexcept : mem{ mem } {}

		void reset() noexcept;

		// Fetches the next instruction over one machine cycle
		// This is only needed after resetting the CPU or handling an interrupt
		// as the instructions themselves will handle fetching the next instruction
		void prefetch() noexcept;

		// Performs one machine cycle
		void cycle() noexcept;

		const auto& r() const noexcept { return reg; }
		auto cycles() const noexcept { return m_cycles; }

		void load_registers(const registers& r) noexcept { 
			reg = r; 
			reg.ir &= 0x1FF;
		}

		std::string gameboy_doctor_log() const;
	private:

		std::size_t m_cycles = 0;

		addressable* mem{};
		registers reg{};

	};
}