#include <fstream>

#include <yahbog.h>

namespace yahbog {
	bool rom_t::load_rom(const std::filesystem::path& path) {
		std::ifstream file(path, std::ios::binary);
		if(!file.is_open()) {
			return false;
		}

		std::vector<std::uint8_t> data(std::istreambuf_iterator<char>(file), {});
		return load_rom(std::move(data));
	}
}