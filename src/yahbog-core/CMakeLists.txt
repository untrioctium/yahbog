# mimalloc
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(mimalloc URL https://github.com/microsoft/mimalloc/archive/refs/tags/v3.0.8.zip)
FetchContent_MakeAvailable(mimalloc)

add_library(
    yahbog-core STATIC
    
    include/yahbog/cpu.h
    include/yahbog/emulator.h
    include/yahbog/mmu.h
    include/yahbog/operations.h
    include/yahbog/opinfo.h
    include/yahbog/ppu.h
    include/yahbog/registers.h
    include/yahbog/rom.h

    include/yahbog/impl/emulator_impl.h
    include/yahbog/impl/ppu_impl.h
    include/yahbog/impl/rom_impl.h

    include/yahbog/utility/constexpr_function.h
    
    opinfo.cpp
    rom.cpp
)

target_link_libraries(yahbog-core PRIVATE mimalloc-static)

if(WIN32)
    target_compile_definitions(yahbog-core PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

target_include_directories(
    yahbog-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    yahbog-core PUBLIC
    include
)