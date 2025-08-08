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

		// execute once per T-cycle (4 MHz)
		constexpr void tick() noexcept;
		
		constexpr const auto& framebuffer() const noexcept { return m_framebuffers[display_buffer_idx()]; }
		constexpr bool framebuffer_ready() const noexcept { return lcd_status.v.mode == mode_t::vblank; }

		consteval static auto address_range() {
			return std::array{
				address_range_t<gpu>{ 0x8000, 0x9FFF, &gpu::read_vram, &gpu::write_vram },
				address_range_t<gpu>{ 0xFE00, 0xFE9F, &gpu::read_oam, &gpu::write_oam<false> },
				address_range_t<gpu>{ 0xFF40, &mem_helpers::read_io_register<&gpu::lcdc>, &gpu::write_lcdc /* special handling to reset ppu */ },
				mem_helpers::make_member_accessor<0xFF41, &gpu::lcd_status>(),
				mem_helpers::make_member_accessor<0xFF42, &gpu::scy>(),
				mem_helpers::make_member_accessor<0xFF43, &gpu::scx>(),
				address_range_t<gpu>{ 0xFF44, &mem_helpers::read_byte<&gpu::ly>, [](gpu*, std::uint16_t, std::uint8_t) { /* do nothing */ } },
				mem_helpers::make_member_accessor<0xFF45, &gpu::lyc>(),
				address_range_t<gpu>{ 0xFF46, &mem_helpers::read_byte<&gpu::dma>, &gpu::write_dma },
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
			dma_source = 0;
			dma_offset = std::numeric_limits<std::uint8_t>::max();
		}

		consteval static auto serializable_members() {
			return std::tuple{
				&gpu::mode_clock,
				&gpu::m_framebuffers,
				&gpu::m_framebuffer_idx,
				&gpu::tile_data,
				&gpu::vram,
				&gpu::oam,
				&gpu::lcdc,
				&gpu::lcd_status,
				&gpu::scy,
				&gpu::scx,
				&gpu::ly,
				&gpu::lyc,
				&gpu::dma,
				&gpu::dma_source,
				&gpu::dma_offset,
				&gpu::dma_countdown,
				&gpu::queued_dma_pending,
				&gpu::bgp,
				&gpu::obp0,
				&gpu::obp1,
				&gpu::wy,
				&gpu::wx
			};
		}

		constexpr static std::size_t bpp = 2;
		constexpr static std::size_t screen_width = 160;
		constexpr static std::size_t screen_height = 144;

		constexpr std::uint8_t get_pixel(std::size_t x, std::size_t y) const noexcept {
			ASSUME_IN_RANGE(x, 0, screen_width - 1);
			ASSUME_IN_RANGE(y, 0, screen_height - 1);
			
			constexpr auto pixels_per_byte = 8 / bpp;
			const auto n_bytes = (y * screen_width + x) / pixels_per_byte;
			const auto bit_offset = (y * screen_width + x) % pixels_per_byte * bpp;

			const auto& buffer = m_framebuffers[m_framebuffer_idx];
			const auto color = (buffer[n_bytes] >> bit_offset) & 0b11;

			return color;
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

		constexpr void update_tile_data(std::uint16_t addr) noexcept {
			auto saddr = addr;
			if(addr & 1) {
				saddr--;
				addr--;
			}

			const auto tile_idx = (addr >> 4) & 511;

			if(tile_idx >= 384) {
				return;
			}

			const auto y = (addr >> 1) & 7;

			for(std::size_t x = 0; x < 8; x++) {
				const auto sx = 1 << (7 - x);
				tile_data[tile_idx][y][x] = ((vram[saddr] & sx) ? 1 : 0) + ((vram[saddr + 1] & sx) ? 2 : 0);
			}
		}

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
			update_tile_data(addr & 0x1FFF);
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

		template<bool Force>
		constexpr void write_oam(uint16_t addr, uint8_t value) noexcept {
			ASSUME_IN_RANGE(addr, 0xFE00, 0xFE9F);

			if(!Force && (lcd_status.v.mode == mode_t::oam || lcd_status.v.mode == mode_t::vram)) {
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

			constexpr auto pixels_per_byte = 8 / bpp;
			const auto n_bytes = (y * screen_width + x) / pixels_per_byte;
			const auto bit_offset = (y * screen_width + x) % pixels_per_byte * bpp;
			
			auto& buffer = m_framebuffers[next_buffer_idx()];
			auto& current_color = buffer[n_bytes];
			current_color &= ~(0b11 << bit_offset);
			current_color |= ((color & 0b11) << bit_offset);
		}

		constexpr void render_scanline() noexcept;

		std::uint16_t mode_clock = 0;

		constexpr static auto framebuffer_size = screen_width * screen_height / (8 / bpp);
		using framebuffer_t = std::array<std::uint8_t, framebuffer_size>;

		std::array<framebuffer_t, 2> m_framebuffers{};
		std::uint8_t m_framebuffer_idx = 0; // index of the framebuffer to display

		constexpr std::uint8_t display_buffer_idx() const noexcept {
			return m_framebuffer_idx;
		}

		constexpr std::uint8_t next_buffer_idx() const noexcept {
			return m_framebuffer_idx ^ 1;
		}

		constexpr void swap_buffers() noexcept {
			m_framebuffer_idx ^= 1;
		}

		std::array<std::uint8_t, 0x2000> vram{};

		using tile_t = std::array<std::array<std::uint8_t, 8>, 8>;
		std::array<tile_t, 384> tile_data{};

		struct oam_flags_t {
			std::uint8_t cgb_palette  : 3;
			std::uint8_t cgb_bank     : 1;
			std::uint8_t palette      : 1;
			std::uint8_t x_flip       : 1;
			std::uint8_t y_flip       : 1;
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
			constexpr static std::uint8_t read_mask  = 0xFF;
			constexpr static std::uint8_t write_mask = 0xFF;

			std::uint8_t bg_display      : 1;
			std::uint8_t obj_display     : 1;
			std::uint8_t obj_size        : 1;
			std::uint8_t bg_tile_map     : 1;
			std::uint8_t bg_tile_data    : 1;
			std::uint8_t window_display  : 1;
			std::uint8_t window_tile_map : 1;
			std::uint8_t lcd_display     : 1;
		};
		io_register<lcdc_t> lcdc{};

		struct lcd_status_t {
			constexpr static std::uint8_t read_mask  = 0b0111'1111;
			constexpr static std::uint8_t write_mask = 0b0111'1000;

			mode_t mode                : 2;
			std::uint8_t coincidence   : 1;
			std::uint8_t mode_hblank   : 1;
			std::uint8_t mode_vblank   : 1;
			std::uint8_t mode_oam      : 1;
			std::uint8_t lyc_condition : 1;

			std::uint8_t _unused : 1;
		};
		io_register<lcd_status_t> lcd_status{};

		std::uint8_t scy{};
		std::uint8_t scx{};
		std::uint8_t ly{};
		std::uint8_t lyc{};
		std::uint8_t dma{};

		std::uint16_t dma_source{};
		std::uint16_t dma_offset = std::numeric_limits<std::uint16_t>::max();
		std::uint8_t dma_countdown = 0;
		std::uint8_t dma_countdown_queued = 0;
		bool queued_dma_pending = false;

		constexpr void write_dma(std::uint16_t addr, std::uint8_t value) noexcept {
			ASSUME_IN_RANGE(addr, 0xFF46, 0xFF46);

			dma = value;

			if(dma_offset != std::numeric_limits<std::uint16_t>::max()) {
				queued_dma_pending = true;
				dma_countdown_queued = 8;
				return;
			}

			dma_source = value * 0x100;
			dma_offset = 0;
			dma_countdown = 8;
		}

		constexpr void dma_tick() noexcept {
			if(dma_offset == std::numeric_limits<std::uint16_t>::max()) [[likely]] {
				return;
			}

			if(dma_countdown > 0) {
				dma_countdown--;

				if(dma_countdown == 0) {
					mem_fns->state = bus_state::dma_blocked;
				}

				return;
			}

			if(dma_countdown_queued > 0) {
				dma_countdown_queued--;
				return;
			}

			if(dma_countdown_queued == 0 && queued_dma_pending) {
				dma_source = dma * 0x100;
				queued_dma_pending = false;
				dma_offset = 0;
				dma_countdown = 0;
			}
			
			if(dma_offset % 4 == 0) {
				write_oam<true>(0xFE00 + dma_offset / 4, mem_fns->read(dma_source + dma_offset / 4));
			}
			dma_offset++;

			if(dma_offset == 640) {

				dma_offset = std::numeric_limits<std::uint16_t>::max();
				mem_fns->state = bus_state::normal;

			}
		}

		struct bgp_t {
			constexpr static std::uint8_t read_mask  = 0xFF;
			constexpr static std::uint8_t write_mask = 0xFF;

			std::uint8_t c0 : 2;
			std::uint8_t c1 : 2;
			std::uint8_t c2 : 2;
			std::uint8_t c3 : 2;
		};
		io_register<bgp_t> bgp{};

		struct obp_t {
			constexpr static std::uint8_t read_mask  = 0b1111'1100;
			constexpr static std::uint8_t write_mask = 0b1111'1100;
			
			std::uint8_t _unused : 2;
			std::uint8_t c1      : 2;
			std::uint8_t c2      : 2;
			std::uint8_t c3      : 2;
		};
		io_register<obp_t> obp0{};
		io_register<obp_t> obp1{};

		std::uint8_t wy{};
		std::uint8_t wx{};

	};
}

#include <yahbog/impl/ppu_impl.h>