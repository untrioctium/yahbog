#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <span>

#include <yahbog.h>

#include "util/compute_shader.h"
#include "util/texture.h"

static void glfw_error_callback(int error, const char* description)
{
	std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

constexpr static std::string_view converter_source = R"shader(
	#version 430
	layout(local_size_x = 8, local_size_y = 8) in;

	layout(rgba8, binding = 0) writeonly uniform image2D uOutRGBA; // 160x144
	layout(binding = 1) uniform usampler2D uPacked2bpp;            // 40x144, GL_R8UI

	uniform vec4 uPalette[4];

	void main() {
		ivec2 p = ivec2(gl_GlobalInvocationID.xy);
		if (p.x >= 160 || p.y >= 144) return;

		int sx = p.x, sy = p.y;
		ivec2 texel = ivec2(sx >> 2, sy);
		uint packedv = texelFetch(uPacked2bpp, texel, 0).r;
		uint idx = (packedv >> uint((sx & 3) * 2)) & 3u;

		imageStore(uOutRGBA, p, uPalette[int(idx)]);
	}
)shader";

struct rendering_context {

	GLuint gameboy_image;
	GLuint rendered_image;

	yahbog::compute_shader converter;
	std::array<std::array<float, 4>, 4> palette;

	rendering_context() {
		auto converter_result = yahbog::compute_shader::make(converter_source);
		if(!converter_result) {
			std::cerr << "Failed to compile converter shader: " << converter_result.error() << std::endl;
			throw std::runtime_error(converter_result.error());
		}
		converter = std::move(converter_result.value());

		glGenTextures(1, &gameboy_image);
		glBindTexture(GL_TEXTURE_2D, gameboy_image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, 40, 144, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

		glGenTextures(1, &rendered_image);
		glBindTexture(GL_TEXTURE_2D, rendered_image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 160, 144, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void run_convert(const yahbog::ppu_t::framebuffer_t& framebuffer) {
		glBindTexture(GL_TEXTURE_2D, gameboy_image);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 40, 144, GL_RED_INTEGER, GL_UNSIGNED_BYTE, framebuffer.data());
		glBindTexture(GL_TEXTURE_2D, 0);

		converter.use();

		glBindImageTexture(0, rendered_image, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gameboy_image);

		converter.uniform("uOutRGBA", 0);
		converter.uniform("uPalette", palette);
		converter.uniform("uPacked2bpp", 1);

		converter.dispatch(160/8, 144/8);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
		glUseProgram(0);
	}
};

int main()
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	// GL 4.3 + GLSL 430
	const char* glsl_version = "#version 430";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "YahBog GUI", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	 // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;	  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		 // Enable Docking

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	bool show_demo_window = true;
	ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

	auto emu = std::make_unique<yahbog::emulator>(yahbog::hardware_mode::dmg);

	auto rom = yahbog::rom_t::load_rom(std::filesystem::path("testdata/blargg/cpu_instrs/01-special.gb"));
	if(!rom) {
		std::cerr << "Failed to load ROM" << std::endl;
		return -1;
	}
	emu->set_rom(std::move(rom));
	emu->z80.reset(&emu->mem_fns);
	emu->io.reset();
	emu->ppu.reset();

	rendering_context renderer;
	renderer.palette = std::array{
		std::array{1.0f, 1.0f, 1.0f, 1.0f},
		std::array{0.66f, 0.66f, 0.66f, 1.0f},
		std::array{0.33f, 0.33f, 0.33f, 1.0f},
		std::array{0.0f, 0.0f, 0.0f, 1.0f}
	};

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		while(!emu->ppu.framebuffer_ready()) {
			emu->tick();
		}

		while(emu->ppu.framebuffer_ready()) {
			emu->tick();
		}

		renderer.run_convert(emu->ppu.framebuffer());

		if(ImGui::Begin("Gameboy")) {
			ImGui::Image((ImTextureID)(intptr_t)(renderer.rendered_image), ImVec2(160 * 3, 144 * 3));
		}
		ImGui::End();


		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
