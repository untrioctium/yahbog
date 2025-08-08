#include <fstream>

#include <yahbog.h>

namespace yahbog {
	std::unique_ptr<rom_t> rom_t::load_rom(const std::filesystem::path& path) {
		std::ifstream file(path, std::ios::binary);
		if(!file.is_open()) {
			return nullptr;
		}

		std::vector<std::uint8_t> data(std::istreambuf_iterator<char>(file), {});
		return load_rom(std::move(data));
	}
}