add_library(
    yahbog-core STATIC
    
    include/yahbog/cpu.h
    include/yahbog/registers.h
    include/yahbog/mmu.h
    include/yahbog/opinfo.h
    include/yahbog/ppu.h
    include/yahbog/emulator.h
    
    emulator.cpp
    opinfo.cpp
    cpu.cpp
    ppu.cpp
    operations.h
)

target_include_directories(
    yahbog-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    yahbog-core PUBLIC
    include
)