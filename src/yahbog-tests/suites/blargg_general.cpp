#include <yahbog-tests.h>
#include <yahbog/emulator.h>

namespace {

    struct test_info {
        std::string_view name;

        std::string_view rom_src;
        std::string_view rom_path;
    };

    #define BASE_DIR TEST_DATA_DIR "/blargg/general"
    #define GIT_ROM_BASE "https://github.com/retrio/gb-test-roms/raw/refs/heads/master"

    #define MAKE_TEST(desc, rom_name) \
        test_info{ \
            .name = desc, \
            .rom_src = GIT_ROM_BASE "/" rom_name, \
            .rom_path = BASE_DIR "/" desc ".gb" \
        }

    constexpr auto tests = std::array<test_info, 5>{
        MAKE_TEST("instr_timing", "instr_timing/instr_timing.gb"),
        MAKE_TEST("interrupts", "cpu_instrs/individual/02-interrupts.gb"),
        MAKE_TEST("01-read_timing", "mem_timing/individual/01-read_timing.gb"),
        MAKE_TEST("02-write_timing", "mem_timing/individual/02-write_timing.gb"),
        MAKE_TEST("03-modify_timing", "mem_timing/individual/03-modify_timing.gb")
    };

    bool run_test(const test_info& test) {
        std::println("Running test: {}", test.name);

        if (!std::filesystem::exists(test.rom_path)) {
            std::println("Downloading rom: {}", test.rom_src);
            if (!download_to_path(test.rom_src, test.rom_path)) {
                std::println("Failed to download rom");
                return false;
            }
        }

        auto emu = std::make_unique<yahbog::emulator>();
        std::string serial_data{};
        emu->hook_writing([&serial_data](uint16_t addr, uint8_t value) {
            // ignore audio registers
            if(addr >= 0xFF10 && addr <= 0xFF3F) {
                return true;
            }

            // ignore serial transfer registers
            if(addr == 0xFF02) {
                return true;
            }

            // ignore joypad
            if(addr == 0xFF00) {
                return true;
            }

            // print serial transfer data
            if(addr == 0xFF01) {
                serial_data += static_cast<char>(value);
                return true;
            }

            return false;
        });

        emu->hook_reading([](uint16_t addr) -> yahbog::emulator::reader_hook_response {
            if(addr == 0xFF00) {
                return std::uint8_t{0x00};
            }

            // ignore audio registers
            if(addr >= 0xFF10 && addr <= 0xFF3F) {
                return std::uint8_t{0x00};
            }

            return {};
        });

        auto regs = yahbog::registers{};
        regs.a = 0x01; regs.f = 0xB0;
        regs.b = 0x00; regs.c = 0x13;
        regs.d = 0x00; regs.e = 0xD8;
        regs.h = 0x01; regs.l = 0x4D;
        regs.sp = 0xFFFE; regs.pc = 0x0100;

        emu->z80.load_registers(regs);
        emu->rom.load_rom(test.rom_path);

        auto& cpu = emu->z80;
        auto& mem = emu->mmu;

        while(true) {
            cpu.cycle();

            if(serial_data.ends_with("Passed")) {
                return true;
            } else if(serial_data.ends_with("Failed")) {
                std::println("Test failed: {}", test.name);
                std::println("Serial data: {}", serial_data);
                return false;
            }
        }

        return true;
    }
}

bool run_blargg_general() {
    std::println("Running blargg general tests");

    std::filesystem::create_directories(BASE_DIR);

    for(const auto& test : tests) {
        run_test(test);
    }

    return true;
}