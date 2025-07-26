FetchContent_Declare(
    any_invocable
    GIT_REPOSITORY https://github.com/ofats/any_invocable
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(any_invocable)

# mimalloc
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

target_include_directories(
    yahbog-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    yahbog-core PUBLIC
    include
    ${any_invocable_SOURCE_DIR}/include
)