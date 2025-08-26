FetchContent_Declare(nameof URL https://github.com/Neargye/nameof/archive/refs/heads/master.zip)
FetchContent_MakeAvailable(nameof)

add_library(
    yahbog-core STATIC
    include/yahbog/common.h
    
    include/yahbog/cpu.h
    include/yahbog/emulator.h
    include/yahbog/io.h
    include/yahbog/mmu.h
    include/yahbog/operations.h
    include/yahbog/opinfo.h
    include/yahbog/ppu.h
    include/yahbog/registers.h
    include/yahbog/rom.h
    include/yahbog/timer.h

    include/yahbog/impl/emulator_impl.h
    include/yahbog/impl/ppu_impl.h
    include/yahbog/impl/rom_impl.h
    include/yahbog/impl/cpu_impl.h
    include/yahbog/impl/timer_impl.h
    include/yahbog/impl/apu_impl.h
    include/yahbog/impl/mmu_impl.h
    include/yahbog/impl/io_impl.h

    include/yahbog/utility/constexpr_function.h
    include/yahbog/utility/serialization.h
    include/yahbog/utility/sha1.h
    include/yahbog/utility/traits.h

    include/yahbog.h
    
    opinfo.cpp
    rom.cpp
)

target_link_libraries(yahbog-core PUBLIC nameof::nameof zlib)

target_precompile_headers(yahbog-core PUBLIC include/yahbog.h)

if(WIN32)
    target_compile_definitions(yahbog-core PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if(ENABLE_XRAY)
    enable_xray(yahbog-core)
endif()

target_include_directories(
    yahbog-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    yahbog-core PUBLIC
    include
)