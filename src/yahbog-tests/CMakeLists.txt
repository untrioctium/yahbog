add_executable(
    yahbog-tests
    
    main.cpp

    suites/single_step.cpp
    suites/blargg_cpu_instrs.cpp
    suites/blargg_general.cpp
    suites/mooneye.cpp
    suites/gbmicrotest.cpp

    yahbog-tests.h
    yahbog-tests.cpp
)

# nlohmann/json
FetchContent_Declare(zip URL https://github.com/kuba--/zip/archive/refs/tags/v0.3.3.tar.gz)
FetchContent_MakeAvailable(zip)

# termcolor
FetchContent_Declare(termcolor URL https://github.com/ikalnytskyi/termcolor/archive/refs/tags/v2.1.0.zip SOURCE_SUBDIR dummy)
FetchContent_MakeAvailable(termcolor)

add_library(termcolor INTERFACE)
target_include_directories(termcolor INTERFACE ${termcolor_SOURCE_DIR}/include)

# readerwriterqueue
FetchContent_Declare(readerwriterqueue URL https://github.com/cameron314/readerwriterqueue/archive/refs/tags/v1.0.7.tar.gz)
FetchContent_MakeAvailable(readerwriterqueue)

# stb
FetchContent_Declare(stb URL https://github.com/nothings/stb/archive/refs/heads/master.zip)
FetchContent_MakeAvailable(stb)


set_target_properties(yahbog-tests PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

target_include_directories(yahbog-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${stb_SOURCE_DIR})

target_precompile_headers(yahbog-tests REUSE_FROM yahbog-core)

target_link_libraries(
    yahbog-tests PRIVATE
    yahbog-core
    zip
    nlohmann_json::nlohmann_json
    termcolor
    readerwriterqueue
)

#if(UNIX)
#    target_compile_options(yahbog-tests PRIVATE -O3 -march=native)
#endif()