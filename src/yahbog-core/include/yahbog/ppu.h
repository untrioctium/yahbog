#pragma once

#include <yahbog/mmu.h>
#include <yahbog/registers.h>
#include <yahbog/operations.h>
#include <yahbog/utility/serialization.h>

namespace yahbog {
	class gpu : public serializable<gpu> {
	public:

		constexpr gpu(mem_fns_t* mem_fns) noexcept : mem_fns(mem_fns) {
			lcd_status.v.mode = mode_t::hblank;
		}

		constexpr void tick(std::uint8_t cycles) noexcept;
		constexpr const auto& framebuffer() const noexcept { return m_framebuffer; }
		constexpr bool framebuffer_ready() const noexcept { return lcd_status.v.mode == mode_t::vblank; }

		consteval static auto address_range() {
			return std::array{
				address_range_t<gpu>{ 0x8000, 0x9FFF, &gpu::read_vram, &gpu::write_vram },
				address_range_t<gpu>{ 0xFE00, 0xFE9F, &gpu::read_oam, &gpu::write_oam },
				address_range_t<gpu>{ 0xFF40, &mem_helpers::read_io_register<&gpu::lcdc>, &gpu::write_lcdc /* special handling to reset ppu */ },
				mem_helpers::make_member_accessor<0xFF41, &gpu::lcd_status>(),
				mem_helpers::make_member_accessor<0xFF42, &gpu::scy>(),
				mem_helpers::make_member_accessor<0xFF43, &gpu::scx>(),
				address_range_t<gpu>{ 0xFF44, &mem_helpers::read_byte<&gpu::ly>, [](gpu*, std::uint16_t, std::uint8_t) { /* do nothing */ } },
				mem_helpers::make_member_accessor<0xFF45, &gpu::lyc>(),
				mem_helpers::make_member_accessor<0xFF46, &gpu::dma>(),
				mem_helpers::make_member_accessor<0xFF47, &gpu::bgp>(),
				mem_helpers::make_member_accessor<0xFF48, &gpu::obp0>(),
				mem_helpers::make_member_accessor<0xFF49, &gpu::obp1>(),
				mem_helpers::make_member_accessor<0xFF4A, &gpu::wy>(),
				mem_helpers::make_member_accessor<0xFF4B, &gpu::wx>()
			};
		};

		constexpr void reset() noexcept {
			lcdc.set_byte(0x91);
			lcd_status.set_byte(0x91);
			bgp.set_byte(0xFC);
			obp0.set_byte(0xFF);
			obp1.set_byte(0xFF);
			scx = 0;
			scy = 0;
			wx = 0;
			wy = 0;
			ly = 0;
			lyc = 0;
		}

		consteval static auto serializable_members() {
			return std::tuple{
				&gpu::mode_clock,
				&gpu::m_framebuffer,
				&gpu::vram,
				&gpu::oam,
				&gpu::lcdc,
				&gpu::lcd_status,
				&gpu::scy,
				&gpu::scx,
				&gpu::ly,
				&gpu::lyc,
				&gpu::dma,
				&gpu::bgp,
				&gpu::obp0,
				&gpu::obp1,
				&gpu::wy,
				&gpu::wx
			};
		}

	private:

		constexpr void write_lcdc(std::uint16_t addr, std::uint8_t value) noexcept {
			auto old_enable = lcdc.v.lcd_display;
			lcdc.set_byte(value);
			if(old_enable && !lcdc.v.lcd_display) {
				lcd_status.v.mode = mode_t::hblank;
				mode_clock = 0;
				ly = 0;
			}
		}

		enum class mode_t : std::uint8_t {
			hblank = 0,
			vblank = 1,
			oam = 2,
			vram = 3
		};

		mem_fns_t* mem_fns = nullptr;

		constexpr uint8_t read_vram(uint16_t addr) noexcept {
			ASSUME_IN_RANGE(addr, 0x8000, 0x9FFF);

			if(lcd_status.v.mode == mode_t::vram) {
				return 0xFF;
			}

			return vram[addr - 0x8000];
		}

		constexpr void write_vram(uint16_t addr, uint8_t value) noexcept {
			ASSUME_IN_RANGE(addr, 0x8000, 0x9FFF);

			if(lcd_status.v.mode == mode_t::vram) {
				return;
			}

			vram[addr - 0x8000] = value;
		}

		constexpr uint8_t read_oam(uint16_t addr) noexcept {
			ASSUME_IN_RANGE(addr, 0xFE00, 0xFE9F);

			if(lcd_status.v.mode == mode_t::oam || lcd_status.v.mode == mode_t::vram) {
				return 0xFF;
			}

			const auto& oam_entry = oam[(addr - 0xFE00) / sizeof(oam_entry_t)];
			const auto oam_byte = (addr - 0xFE00) % sizeof(oam_entry_t);
			
			switch(oam_byte) {
				case 0: return oam_entry.y;
				case 1: return oam_entry.x;
				case 2: return oam_entry.tile_index;
				case 3: return std::bit_cast<std::uint8_t>(oam_entry.flags);
				default: std::unreachable();
			}

			std::unreachable();
		}

		constexpr void write_oam(uint16_t addr, uint8_t value) noexcept {
			ASSUME_IN_RANGE(addr, 0xFE00, 0xFE9F);

			if(lcd_status.v.mode == mode_t::oam || lcd_status.v.mode == mode_t::vram) {
				return;
			}

			auto& oam_entry = oam[(addr - 0xFE00) / sizeof(oam_entry_t)];
			const auto oam_byte = (addr - 0xFE00) % sizeof(oam_entry_t);

			switch(oam_byte) {
				case 0: oam_entry.y = value; return;
				case 1: oam_entry.x = value; return;
				case 2: oam_entry.tile_index = value; return;
				case 3: oam_entry.flags = std::bit_cast<oam_flags_t>(value); return;
				default: std::unreachable();
			}

			std::unreachable();
		}

		constexpr void set_pixel(std::size_t x, std::size_t y, std::uint8_t color) noexcept {
			ASSUME_IN_RANGE(x, 0, screen_width - 1);
			ASSUME_IN_RANGE(y, 0, screen_height - 1);

			const auto n_bits = y * screen_width + x;
			const auto n_bytes = n_bits / 8;
			const auto bit_offset = n_bits % 8;

			auto current_color = m_framebuffer[n_bytes];
			current_color &= ~(1 << bit_offset);
			current_color |= ((color & 0b11) << bit_offset);
			m_framebuffer[n_bytes] = current_color;
		}

		constexpr void render_scanline() noexcept;

		std::uint16_t mode_clock = 0;

		constexpr static std::size_t bpp = 2;
		constexpr static std::size_t screen_width = 160;
		constexpr static std::size_t screen_height = 144;
		constexpr static auto framebuffer_size = screen_width * screen_height / (8 / bpp);

		std::array<std::uint8_t, framebuffer_size> m_framebuffer{};

		std::array<std::uint8_t, 0x2000> vram{};

		struct oam_flags_t {
			std::uint8_t cgb_palette : 3;
			std::uint8_t cgb_bank : 1;
			std::uint8_t palette : 1;
			std::uint8_t x_flip : 1;
			std::uint8_t y_flip : 1;
			std::uint8_t cgb_priority : 1;
		};

		static_assert(sizeof(oam_flags_t) == 1);

		struct oam_entry_t {
			std::uint8_t y;
			std::uint8_t x;
			std::uint8_t tile_index;
			oam_flags_t flags;
		};

		static_assert(sizeof(oam_entry_t) == 4);

		std::array<oam_entry_t, 40> oam{};
		static_assert(sizeof(oam) == 0xA0);

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

			mode_t mode : 2;
			std::uint8_t coincidence : 1;
			std::uint8_t mode_hblank : 1;
			std::uint8_t mode_vblank : 1;
			std::uint8_t mode_oam : 1;
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

#include <yahbog/impl/ppu_impl.h>