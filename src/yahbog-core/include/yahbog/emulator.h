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

namespace yahbog {

	class wram_t : public serializable<wram_t> {
	public:
		consteval static auto address_range() {
			return std::array{
				address_range_t<wram_t>{0xC000, 0xCFFF, &wram_t::read_bank00, &wram_t::write_bank00},
				address_range_t<wram_t>{0xD000, 0xDFFF, &wram_t::read_banked, &wram_t::write_banked},
				address_range_t<wram_t>{0xFF80, 0xFFFE, &wram_t::read_hram, &wram_t::write_hram},
				address_range_t<wram_t>{0xFF70, &wram_t::read_bank_idx, &wram_t::write_bank_idx}
			};
		}

		std::vector<uint8_t> wram;
		std::array<uint8_t, 0x7F> hram{0};
		std::uint8_t ram_bank_idx = 1;
		const hardware_mode mode;

		consteval static auto serializable_members() {
			return std::tuple{
				&wram_t::wram,
				&wram_t::hram,
				&wram_t::ram_bank_idx
			};
		}

		constexpr wram_t(hardware_mode mode) : mode(mode) {
			if(mode == hardware_mode::dmg) {
				wram.resize(0x2000);
			} else {
				wram.resize(0x8000);
			}
		}

	private:

		constexpr static std::size_t bank_size = 0x1000; // 4kb

		constexpr uint8_t read_bank00(uint16_t addr) { 
			ASSUME_IN_RANGE(addr, 0xC000, 0xCFFF);

			return wram[addr - 0xC000]; 
		}
		constexpr void write_bank00(uint16_t addr, uint8_t value) { 
			ASSUME_IN_RANGE(addr, 0xC000, 0xCFFF);

			wram[addr - 0xC000] = value; 
		}

		constexpr uint8_t read_banked(uint16_t addr) {
			ASSUME_IN_RANGE(addr, 0xD000, 0xDFFF);
			ASSUME_IN_RANGE(ram_bank_idx, 1, 7);

			return wram[addr - 0xD000 + ram_bank_idx * bank_size];
		}

		constexpr void write_banked(uint16_t addr, uint8_t value) {
			ASSUME_IN_RANGE(addr, 0xD000, 0xDFFF);
			ASSUME_IN_RANGE(ram_bank_idx, 1, 7);

			wram[addr - 0xD000 + ram_bank_idx * bank_size] = value;
		}

		constexpr uint8_t read_hram(uint16_t addr) { 
			ASSUME_IN_RANGE(addr, 0xFF80, 0xFFFE);

			return hram[addr - 0xFF80]; 
		}
		constexpr void write_hram(uint16_t addr, uint8_t value) { 
			ASSUME_IN_RANGE(addr, 0xFF80, 0xFFFE);

			hram[addr - 0xFF80] = value; 
		}

		constexpr uint8_t read_bank_idx([[maybe_unused]] uint16_t addr) { 
			if(mode == hardware_mode::dmg) {
				return 1;
			}
			return ram_bank_idx; 
		}
		constexpr void write_bank_idx([[maybe_unused]] uint16_t addr, uint8_t value) { 
			if(mode == hardware_mode::dmg) {
				return;
			}
			ram_bank_idx = std::clamp(value, std::uint8_t{1}, std::uint8_t{7}); 
		}
	};

	class scheduler {
	public:

		using event_t = constexpr_function<void()>;

		constexpr scheduler() noexcept = default;
		constexpr scheduler(const scheduler& other) noexcept = delete;
		constexpr scheduler(scheduler&& other) noexcept = delete;
		constexpr scheduler& operator=(const scheduler& other) noexcept = delete;
		constexpr scheduler& operator=(scheduler&& other) noexcept = delete;

	private:
		
	};

	class emulator {
	public:

		mem_fns_t mem_fns;

		wram_t wram;
		rom_t rom;
		cpu z80;
		gpu ppu;
		io_t io;		

		constexpr static std::size_t address_space_size = std::numeric_limits<std::uint16_t>::max() + 1;
		memory_dispatcher<address_space_size, gpu, wram_t, rom_t, cpu, io_t> mmu;

		constexpr emulator(hardware_mode mode) : 
			mem_fns{default_reader(), default_writer()},
			wram(mode),
			z80(&mem_fns),
			ppu(&mem_fns)
			{
				mmu.set_handler(&wram);
				mmu.set_handler(&ppu);
				mmu.set_handler(&rom);
				mmu.set_handler(&z80);
				mmu.set_handler(&io);
			}

		constexpr emulator(const emulator& other) = delete;
		constexpr emulator(emulator&& other) noexcept = delete;
		constexpr emulator& operator=(const emulator& other) = delete;
		constexpr emulator& operator=(emulator&& other) noexcept = delete;

		constexpr void tick() noexcept {
			z80.cycle();
			ppu.tick(1);
			ppu.tick(1);
			ppu.tick(1);
			ppu.tick(1);
		}

		constexpr read_fn_t default_reader() noexcept {
			return [this](std::uint16_t addr) noexcept { return mmu.read(addr); };
		}

		constexpr write_fn_t default_writer() noexcept { 
			return [this](std::uint16_t addr, std::uint8_t value) noexcept { mmu.write(addr, value); };
		}

		using reader_hook_response = std::variant<std::monostate, std::uint8_t>;

		constexpr void hook_reading(constexpr_function<reader_hook_response(std::uint16_t)>&& hook) noexcept {
			mem_fns.read = [next = default_reader(), hook = std::move(hook)](std::uint16_t addr) noexcept {
				auto response = hook(addr);
				if (std::holds_alternative<std::uint8_t>(response)) [[unlikely]] {
					return std::get<std::uint8_t>(response);
				}
				return next(addr);
			};
		}

		constexpr void hook_writing(constexpr_function<bool(std::uint16_t, std::uint8_t)>&& hook) noexcept {
			mem_fns.write = [next = default_writer(), hook = std::move(hook)](std::uint16_t addr, std::uint8_t value) noexcept {
				if (hook(addr, value)) [[unlikely]] {
					return;
				}
				next(addr, value);
			};
		}

		constexpr static sha1::digest_t version_signature = []() {
			auto hasher = sha1{};
			
			hasher.process_bytes("wram");
			wram_t::add_version_signature(hasher);

			hasher.process_bytes("rom");
			rom_t::add_version_signature(hasher);

			hasher.process_bytes("cpu");
			cpu::add_version_signature(hasher);

			hasher.process_bytes("gpu");
			gpu::add_version_signature(hasher);

			hasher.process_bytes("io");
			io_t::add_version_signature(hasher);

			return hasher.get_digest();
		}();

		constexpr std::vector<std::uint8_t> serialize() const noexcept {

			const std::size_t size = sizeof(version_signature)
				+ wram.serialized_size()
				+ rom.serialized_size()
				+ z80.serialized_size()
				+ ppu.serialized_size()
				+ io.serialized_size();

			std::vector<std::uint8_t> data(size);
			std::span<std::uint8_t> data_view = data;

			data_view = write_to_span(data_view, version_signature);
			data_view = wram.serialize(data_view);
			data_view = rom.serialize(data_view);
			data_view = z80.serialize(data_view);
			data_view = ppu.serialize(data_view);
			data_view = io.serialize(data_view);

			return data;
		}

		enum class deserialize_result {
			success,
			wrong_signature,
			wrong_size
		};

		constexpr deserialize_result deserialize(std::span<const std::uint8_t> data) noexcept {

			const std::size_t expected_size = sizeof(version_signature)
				+ wram.serialized_size()
				+ rom.serialized_size()
				+ z80.serialized_size()
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
			data = rom.deserialize(data);
			data = z80.deserialize(data);
			data = ppu.deserialize(data);
			data = io.deserialize(data);

			return deserialize_result::success;
		}

	private:

	};

	constexpr static auto emu_size = sizeof(emulator);

}