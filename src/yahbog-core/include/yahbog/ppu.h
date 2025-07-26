#pragma once

#include <yahbog/mmu.h>
#include <yahbog/registers.h>

namespace yahbog {
	class gpu : public addressable {
	public:
		constexpr static auto address_range = std::array{
			address_range_t{ 0x8000, 0x9FFF },
			address_range_t{ 0xFE00, 0xFE9F },
			address_range_t{ 0xFF40, 0xFF4B },
			address_range_t{ 0xFF4F, 0xFF4F }
		};

		void tick(std::uint8_t cycles);
		const auto& framebuffer() const { return m_framebuffer; }
		bool framebuffer_ready() const { return mode == mode_t::vblank; }

		uint8_t read(uint16_t addr) override;
		void write(uint16_t addr, uint8_t value) override;

	private:

		void render_scanline();

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