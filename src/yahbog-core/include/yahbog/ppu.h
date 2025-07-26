#pragma once

#include <yahbog/mmu.h>
#include <yahbog/registers.h>

namespace yahbog {
	class gpu {
	public:

		constexpr void tick(std::uint8_t cycles);
		constexpr const auto& framebuffer() const { return m_framebuffer; }
		constexpr bool framebuffer_ready() const { return mode == mode_t::vblank; }

		constexpr uint8_t read(uint16_t addr);
		constexpr void write(uint16_t addr, uint8_t value);

		consteval static auto address_range() {
			return std::array{
				address_range_t<gpu>{ 0x8000, 0x9FFF, &gpu::read_vram, &gpu::write_vram },
				address_range_t<gpu>{ 0xFE00, 0xFE9F, &gpu::read_oam, &gpu::write_oam },
				address_range_t<gpu>{ 0xFF40, 0xFF40, &gpu::read_register<&gpu::lcdc>, &gpu::write_register<&gpu::lcdc> },
				address_range_t<gpu>{ 0xFF41, 0xFF41, &gpu::read_register<&gpu::lcd_status>, &gpu::write_register<&gpu::lcd_status> },
				address_range_t<gpu>{ 0xFF42, 0xFF42, &gpu::read_member<&gpu::scy>, &gpu::write_member<&gpu::scy> },
				address_range_t<gpu>{ 0xFF43, 0xFF43, &gpu::read_member<&gpu::scx>, &gpu::write_member<&gpu::scx> },
				address_range_t<gpu>{ 0xFF44, 0xFF44, &gpu::read_member<&gpu::ly>, &gpu::write_member<&gpu::ly> },
				address_range_t<gpu>{ 0xFF45, 0xFF45, &gpu::read_member<&gpu::lyc>, &gpu::write_member<&gpu::lyc> },
				address_range_t<gpu>{ 0xFF46, 0xFF46, &gpu::read_member<&gpu::dma>, &gpu::write_member<&gpu::dma> },
				address_range_t<gpu>{ 0xFF47, 0xFF47, &gpu::read_register<&gpu::bgp>, &gpu::write_register<&gpu::bgp> },
				address_range_t<gpu>{ 0xFF48, 0xFF48, &gpu::read_register<&gpu::obp0>, &gpu::write_register<&gpu::obp0> },
				address_range_t<gpu>{ 0xFF49, 0xFF49, &gpu::read_register<&gpu::obp1>, &gpu::write_register<&gpu::obp1> },
				address_range_t<gpu>{ 0xFF4A, 0xFF4A, &gpu::read_member<&gpu::wy>, &gpu::write_member<&gpu::wy> },
				address_range_t<gpu>{ 0xFF4B, 0xFF4B, &gpu::read_member<&gpu::wx>, &gpu::write_member<&gpu::wx> }
			};
		};

	private:

		template<auto RegisterPtr>
		constexpr uint8_t read_register([[maybe_unused]] uint16_t addr) {
			return (this->*RegisterPtr).read();
		}

		template<auto RegisterPtr>
		constexpr void write_register([[maybe_unused]] uint16_t addr, uint8_t value) {
			(this->*RegisterPtr).write(value);
		}

		template<auto MemberPtr>
		constexpr uint8_t read_member([[maybe_unused]] uint16_t addr) {
			return this->*MemberPtr;
		}

		template<auto MemberPtr>
		constexpr void write_member([[maybe_unused]] uint16_t addr, uint8_t value) {
			this->*MemberPtr = value;
		}

		constexpr uint8_t read_vram(uint16_t addr) {
			return vram[addr - 0x8000];
		}

		constexpr void write_vram(uint16_t addr, uint8_t value) {
			vram[addr - 0x8000] = value;
		}

		constexpr uint8_t read_oam(uint16_t addr) {
			return oam[addr - 0xFE00];
		}

		constexpr void write_oam(uint16_t addr, uint8_t value) {
			oam[addr - 0xFE00] = value;
		}

		constexpr void render_scanline();

		enum class mode_t {
			hblank = 0,
			vblank = 1,
			oam = 2,
			vram = 3
		};

		std::size_t mode_clock = 0;
		mode_t mode = mode_t::oam;

		constexpr static std::size_t bpp = 2;
		constexpr static auto framebuffer_size = 160 * 144 / (8 / bpp);

		std::array<std::uint8_t, framebuffer_size> m_framebuffer{};

		std::array<std::uint8_t, 0x2000> vram{};
		std::array<std::uint8_t, 0xA0> oam{};

		struct lcdc_t {
			constexpr static std::uint8_t read_mask = 0xFF;
			constexpr static std::uint8_t write_mask = 0xFF;

			std::uint8_t bg_display : 1;
			std::uint8_t obj_display : 1;
			std::uint8_t obj_size : 1;
			std::uint8_t bg_tile_map : 1;
			std::uint8_t bg_tile_data : 1;
			std::uint8_t window_display : 1;
			std::uint8_t window_tile_map : 1;
			std::uint8_t lcd_display : 1;
		};
		io_register<lcdc_t> lcdc{};

		struct lcd_status_t {
			constexpr static std::uint8_t read_mask  = 0b01111111;
			constexpr static std::uint8_t write_mask = 0b01111000;

			std::uint8_t mode : 2;
			std::uint8_t coincidence : 1;
			std::uint8_t mode0 : 1;
			std::uint8_t mode1 : 1;
			std::uint8_t mode2 : 1;
			std::uint8_t lyc_condition : 1;

			std::uint8_t _unused : 1;
		};
		io_register<lcd_status_t> lcd_status{};

		std::uint8_t scy{};
		std::uint8_t scx{};
		std::uint8_t ly{};
		std::uint8_t lyc{};
		std::uint8_t dma{};

		struct bgp_t {
			constexpr static std::uint8_t read_mask = 0xFF;
			constexpr static std::uint8_t write_mask = 0xFF;
			std::uint8_t c0 : 2;
			std::uint8_t c1 : 2;
			std::uint8_t c2 : 2;
			std::uint8_t c3 : 2;
		};
		io_register<bgp_t> bgp{};

		struct obp_t {
			constexpr static std::uint8_t read_mask = 0b11111100;
			constexpr static std::uint8_t write_mask = 0b11111100;
			std::uint8_t _ : 2;
			std::uint8_t c1 : 2;
			std::uint8_t c2 : 2;
			std::uint8_t c3 : 2;
		};
		io_register<obp_t> obp0{};
		io_register<obp_t> obp1{};

		std::uint8_t wy{};
		std::uint8_t wx{};

	};
}