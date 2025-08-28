#pragma once

#include <vector>
#include <filesystem>
#include <span>
#include <variant>

#include <yahbog/mmu.h>
#include <yahbog/cpu.h>
#include <yahbog/ppu.h>
#include <yahbog/rom.h>
#include <yahbog/io.h>
#include <yahbog/timer.h>
#include <yahbog/apu.h>
#include <yahbog/wram.h>

namespace yahbog {

	template<hardware_mode Mode>
	struct mem_fns_t;

	template<hardware_mode Mode>
	using emu_mmu_t = memory_dispatcher<std::numeric_limits<std::uint16_t>::max() + 1, ppu_t<Mode, mem_fns_t<Mode>>, wram_t<Mode>, rom_t, timer_t<Mode>, cpu_t<Mode, mem_fns_t<Mode>>, io_t<Mode>, apu_t<Mode>>;

	template<hardware_mode Mode>
	using emu_reader_t = function_ref<std::uint8_t(std::uint16_t), emu_mmu_t<Mode>>;
	template<hardware_mode Mode>
	using emu_writer_t = function_ref<void(std::uint16_t, std::uint8_t), emu_mmu_t<Mode>>;

	template<hardware_mode Mode>
	struct mem_fns_t {
		emu_reader_t<Mode> read;
		emu_writer_t<Mode> write;
		bus_state state;
	};

	template<hardware_mode Mode>
	class emulator {
	public:

		mem_fns_t<Mode> mem_fns;
		mem_fns_t<Mode> blocked_mem_fns;

		wram_t<Mode> wram;
		std::unique_ptr<rom_t> rom;
		cpu_t<Mode, mem_fns_t<Mode>> z80;
		ppu_t<Mode, mem_fns_t<Mode>> ppu;
		timer_t<Mode> timer;
		io_t<Mode> io;
		apu_t<Mode> apu;


		emu_mmu_t<Mode> mmu;

		constexpr emulator() : 
			mem_fns{default_reader(), default_writer(), bus_state::normal},
			blocked_mem_fns{blocked_reader(), blocked_writer(), bus_state::dma_blocked},
			wram(),
			ppu(&mem_fns, int_req_t<Mode, mem_fns_t<Mode>>{[](cpu_t<Mode, mem_fns_t<Mode>>* cpu, interrupt i) {
				cpu->request_interrupt(i);
			}, &z80}),

			timer([this](interrupt i) {
				this->z80.request_interrupt(i);
			})
			{
				mmu.set_handler(&wram);
				mmu.set_handler(&ppu);
				mmu.set_handler(&timer);
				mmu.set_handler(&z80);
				mmu.set_handler(&io);
				mmu.set_handler(&apu);
			}

		constexpr emulator(const emulator& other) = delete;
		constexpr emulator(emulator&& other) noexcept = delete;
		constexpr emulator& operator=(const emulator& other) = delete;
		constexpr emulator& operator=(emulator&& other) noexcept = delete;

		constexpr void set_rom(std::unique_ptr<rom_t>&& rom) {
			this->rom = std::move(rom);
			if(!this->rom) {
				return;
			}

			mmu.set_handler(this->rom.get());
		}

		constexpr void tick() noexcept {
			z80.cycle(mem_fns.state == bus_state::dma_blocked ? &blocked_mem_fns : &mem_fns);

			timer.tick();
			ppu.tick();

			timer.tick();
			ppu.tick();

			timer.tick();
			ppu.tick();

			timer.tick();
			ppu.tick();
		}

		constexpr emu_reader_t<Mode> default_reader() noexcept {
			return {[](emu_mmu_t<Mode>* mmu, std::uint16_t addr) noexcept { return mmu->read(addr); }, &mmu};
		}

		constexpr emu_writer_t<Mode> default_writer() noexcept { 
			return {[](emu_mmu_t<Mode>* mmu, std::uint16_t addr, std::uint8_t value) noexcept { mmu->write(addr, value); }, &mmu};
		}

		constexpr emu_reader_t<Mode> blocked_reader() noexcept {
			return {[](emu_mmu_t<Mode>* mmu, std::uint16_t addr) noexcept -> std::uint8_t { 
				if(addr >= 0xFE00 && addr <= 0xFE9F) [[unlikely]] {
					return 0xFF;
				} else {
					return mmu->read(addr);
				}
			 }, &mmu};
		}
		constexpr emu_writer_t<Mode> blocked_writer() noexcept {
			return {[](emu_mmu_t<Mode>* mmu, std::uint16_t addr, std::uint8_t value) noexcept -> void { 
				if(addr >= 0xFE00 && addr <= 0xFE9F) [[unlikely]] {
					return;
				} else {
					mmu->write(addr, value);
				}
			}, &mmu};
		}

		constexpr static sha1::digest_t version_signature = []() {
			auto hasher = sha1{};
			
			hasher.process_bytes("wram");
			wram_t<Mode>::add_version_signature(hasher);

			hasher.process_bytes("rom");
			rom_t::add_version_signature(hasher);

			hasher.process_bytes("cpu");
			cpu_t<Mode, mem_fns_t<Mode>>::add_version_signature(hasher);

			hasher.process_bytes("gpu");
			ppu_t<Mode, mem_fns_t<Mode>>::add_version_signature(hasher);

			hasher.process_bytes("io");
			io_t<Mode>::add_version_signature(hasher);

			hasher.process_bytes("timer");
			timer_t<Mode>::add_version_signature(hasher);

			return hasher.get_digest();
		}();

		constexpr std::vector<std::uint8_t> serialize() const noexcept {

			if(!rom) {
				return {};
			}

			const std::size_t size = sizeof(version_signature)
				+ wram.serialized_size()
				+ rom->serialized_size()
				+ z80.serialized_size() + timer.serialized_size()
				+ ppu.serialized_size()
				+ io.serialized_size();

			std::vector<std::uint8_t> data(size);
			std::span<std::uint8_t> data_view = data;

			data_view = write_to_span(data_view, version_signature);
			data_view = wram.serialize(data_view);
			data_view = rom->serialize(data_view);
			data_view = z80.serialize(data_view);
			data_view = timer.serialize(data_view);
			data_view = ppu.serialize(data_view);
			data_view = io.serialize(data_view);

			return data;
		}

		enum class deserialize_result {
			success,
			wrong_signature,
			wrong_size,
			rom_not_loaded
		};

		constexpr deserialize_result deserialize(std::span<const std::uint8_t> data) noexcept {

			if(!rom) {
				return deserialize_result::rom_not_loaded;
			}

			const std::size_t expected_size = sizeof(version_signature)
				+ wram.serialized_size()
				+ rom->serialized_size()
				+ z80.serialized_size() + timer.serialized_size()
				+ ppu.serialized_size()
				+ io.serialized_size();

			if (data.size() != expected_size) {
				return deserialize_result::wrong_size;
			}

			sha1::digest_t actual_signature;
			data = read_from_span(data, actual_signature);
			if (actual_signature != version_signature) {
				return deserialize_result::wrong_signature;
			}

			data = wram.deserialize(data);
			data = rom->deserialize(data);
			data = z80.deserialize(data);
			data = timer.deserialize(data);
			data = ppu.deserialize(data);
			data = io.deserialize(data);

			return deserialize_result::success;
		}

	};

	using dmg_emulator = emulator<hardware_mode::dmg>;
	using cgb_emulator = emulator<hardware_mode::cgb>;

}