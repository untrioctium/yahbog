add_executable(yahbog-gui 
    main.cpp
    util/compute_shader.h
    util/texture.h
)

# freetype
FetchContent_Declare(
  freetype
  GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
  GIT_TAG VER-2-13-3
)
FetchContent_MakeAvailable(freetype)
target_include_directories(freetype PRIVATE ${ZLIB_INCLUDE_DIR})
target_link_libraries(freetype PUBLIC zlib)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# glad
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG master
)
FetchContent_MakeAvailable(glad)

#imgui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)
file(GLOB IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/*.cpp
  ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR}/backends)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR}/misc/freetype)
target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE)
target_compile_definitions(imgui PUBLIC IMGUI_USE_WCHAR32)
target_link_libraries(imgui PUBLIC freetype glfw glad)

target_precompile_headers(yahbog-gui REUSE_FROM yahbog-core)
target_link_libraries(yahbog-gui PRIVATE imgui yahbog-core)

if(ENABLE_XRAY)
    enable_xray(yahbog-gui)
endif()
