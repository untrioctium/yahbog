#pragma once

#include <array>
#include <string_view>

namespace yahbog {

	struct opinfo_t {
		std::string_view name;
	};

	extern constinit const std::array<opinfo_t, 512> opinfo;

}