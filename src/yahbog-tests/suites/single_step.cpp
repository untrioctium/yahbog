#include <yahbog-tests.h>

using memory_map = std::unordered_map<std::uint16_t, std::uint8_t>;

struct test_mmu {

    memory_map memory;

	test_mmu() {
	}

	uint8_t read(uint16_t addr) {
		return memory[addr];
	}

	void write(uint16_t addr, uint8_t value) {
		memory[addr] = value;
	}
};

struct test_state {

	yahbog::registers regs{};
    memory_map memory;

    static test_state from_json(nlohmann::json& js) {
        test_state state;
        state.regs.pc = js["pc"];
        state.regs.sp = js["sp"];
        state.regs.a = js["a"];
        state.regs.b = js["b"];
        state.regs.c = js["c"];
        state.regs.d = js["d"];
        state.regs.e = js["e"];
        state.regs.f = js["f"];
        state.regs.h = js["h"];
        state.regs.l = js["l"];
        state.regs.ime = js["ime"];
        state.regs.mupc = 0;

        if (js.contains("ie"))
            state.regs.ie = js["ie"];
        else if (js.contains("ei"))
            state.regs.ie = js["ei"];
        else state.regs.ie = 0;

        for (auto& values : js["ram"]) {
            std::uint16_t addr = values[0];
            std::uint8_t value = values[1];

			state.memory[addr] = value;
        }
        return state;
    }

    std::string to_str() {
		std::string str;
		str += std::format("PC:0x{:04X} ", regs.pc);
		str += std::format("SP:0x{:04X} ", regs.sp);
		str += std::format("A:0x{:02X} ", regs.a);
		str += std::format("B:0x{:02X} ", regs.b);
		str += std::format("C:0x{:02X} ", regs.c);
		str += std::format("D:0x{:02X} ", regs.d);
		str += std::format("E:0x{:02X} ", regs.e);
		str += std::format("F:0x{:02X} ", regs.f);
		str += std::format("H:0x{:02X} ", regs.h);
		str += std::format("L:0x{:02X} ", regs.l);
		str += std::format("IME:0x{:02X} ", regs.ime);
		str += std::format("IE:0x{:02X} ", regs.ie);
        str += "MEM:[";
		for (auto& [addr, value] : memory) {
			str += std::format("$0x{:04X}:0x{:02X},", addr, value);
		}
		str += "]\n";
		return str;
    }
};

#define CHECK_MISMATCH(name, expected, actual) \
if (expected != actual) { \
    good = false; \
}

#define CHECK_MISMATCH_BIT(name, bit, expected, actual) \
if ((expected) != (actual)) { \
    good = false; \
}

struct test_info {
    std::string name;
    std::size_t ncycles;

    test_state initial_state;
    test_state final_state;

    static test_info from_json(nlohmann::json& js) {
        test_info info;
        info.name = js["name"];
        info.ncycles = js["cycles"].size();
        info.initial_state = test_state::from_json(js["initial"]);
        info.final_state = test_state::from_json(js["final"]);

        if (info.initial_state.regs.ie == 1) {
            info.final_state.regs.ie = 0;
            info.final_state.regs.ime = 1;
        }

        return info;
    }

    bool run() {

		auto mem = test_mmu{};

        for (const auto& [addr, value] : initial_state.memory) {
            mem.memory[addr] = value;
        }

		auto reader = yahbog::read_fn_t{ [&mem](uint16_t addr) { return mem.read(addr); } };
		auto writer = yahbog::write_fn_t{ [&mem](uint16_t addr, uint8_t value) { mem.write(addr, value); } };

		yahbog::cpu cpu( &reader, &writer );
		cpu.reset();
		cpu.load_registers(initial_state.regs);
		cpu.prefetch();

		bool ie = cpu.r().ie;
        for (std::size_t i = 0; i < ncycles; i++) {
            cpu.cycle();
        }

		auto regs = cpu.r();

		// in most cases, the last instruction overlaps with the next fetch
		// so we need to roll back the pc
		if (regs.ir != 0x76)
			regs.pc--;

		// simulate enabling interrupts if it was pending
        if (ie) {
            regs.ime = 1;
            regs.ie = 0;
        }

        bool good = true;
        CHECK_MISMATCH("uPC", 0, regs.mupc);
        CHECK_MISMATCH("PC", final_state.regs.pc, regs.pc);
        CHECK_MISMATCH("SP", final_state.regs.sp, regs.sp);
        CHECK_MISMATCH("A", final_state.regs.a, regs.a);
        CHECK_MISMATCH("B", final_state.regs.b, regs.b);
        CHECK_MISMATCH("C", final_state.regs.c, regs.c);
        CHECK_MISMATCH("D", final_state.regs.d, regs.d);
        CHECK_MISMATCH("E", final_state.regs.e, regs.e);

        CHECK_MISMATCH_BIT("F", 'Z', final_state.regs.f & 0x80, regs.f & 0x80);
        CHECK_MISMATCH_BIT("F", 'N', final_state.regs.f & 0x40, regs.f & 0x40);
        CHECK_MISMATCH_BIT("F", 'H', final_state.regs.f & 0x20, regs.f & 0x20);
        CHECK_MISMATCH_BIT("F", 'C', final_state.regs.f & 0x10, regs.f & 0x10);

        CHECK_MISMATCH("H", final_state.regs.h, regs.h);
        CHECK_MISMATCH("L", final_state.regs.l, regs.l);
        CHECK_MISMATCH("IME", final_state.regs.ime, regs.ime);
        CHECK_MISMATCH("IE", final_state.regs.ie, regs.ie);
		
        for (auto& [addr, value] : final_state.memory) {
            CHECK_MISMATCH(std::format("Memory 0x{:04X}", addr), value, mem.memory[addr]);
        }

        return good;
    }
};

void test_thread(std::span<const std::string> data, std::span<uint8_t> results) {
	for (auto i = 0; i < data.size(); i++) {

		results[i] = 1;

		try {
			nlohmann::json j = nlohmann::json::parse(data[i]);
		
			for (auto& test : j) {
				test_info info = test_info::from_json(test);
				
				auto result = info.run();

				if(!result) {
					results[i] = 0;
					std::print("Test {} failed\n", info.name);
				}
			}
		}
		catch (const std::exception& e) {
			results[i] = 0;
			std::print("Test {} failed\n", i);
		}
	}
}

bool run_single_step_tests() {

	std::println("Loading tests from {}", TEST_DATA_DIR);

	if (!std::filesystem::exists(TEST_DATA_DIR)) {
		std::filesystem::create_directory(TEST_DATA_DIR);
	}

	if (!std::filesystem::exists(TEST_DATA_DIR "/sm83.zip")) {
		std::println("Downloading tests from {}", TEST_DATA_DIR "/sm83.zip");
		download_to_path("https://github.com/SingleStepTests/sm83/archive/refs/heads/main.zip", TEST_DATA_DIR "/sm83.zip");
	}

	std::println("Opening archive: {}", TEST_DATA_DIR "/sm83.zip");
	std::unique_ptr<zip_t, decltype(&zip_close)> archive_ptr(zip_open(TEST_DATA_DIR "/sm83.zip", 0, 'r'), &zip_close);

	if (!archive_ptr) {
		std::println("Failed to open archive");
		return false;
	}

	auto archive = archive_ptr.get();
	std::set<std::size_t> files{};

	const auto archive_total = zip_entries_total(archive);
	auto file_total = 0;
	for (auto i = 0; i < archive_total; i++) {
		zip_entry_openbyindex(archive, i);
		std::string_view name = zip_entry_name(archive);
		if (name.ends_with("json") && !name.ends_with("10.json")) {
			files.insert(i);
			file_total++;
		}
		zip_entry_close(archive);
	}

	std::println("Found {} tests", file_total);

	std::vector<std::string> test_data{};
	test_data.resize(file_total);
	std::vector<std::uint8_t> test_results{};
	test_results.resize(file_total);

	std::memset(test_results.data(), 0, test_results.size());

	std::size_t fc = 0;
	for (auto i: files) {
		zip_entry_openbyindex(archive, i);

		const auto size = zip_entry_size(archive);
		std::string data(size, '\0');

		zip_entry_noallocread(archive, data.data(), size);
		test_data[fc] = std::move(data);
		fc++;

		zip_entry_close(archive);
	}

	std::println("Loaded {} tests", test_data.size());

	const auto num_threads = std::thread::hardware_concurrency();
	const auto per_thread = test_data.size() / num_threads;

    std::vector<std::thread> threads{};

	std::span<const std::string> data_span{ test_data.data(), test_data.size() };
	std::span<std::uint8_t> results_span{ test_results.data(), test_results.size() };

	for (auto i = 0; i < num_threads; i++) {
		auto start = i * per_thread;
		auto end = (i + 1) * per_thread;
		if (i == num_threads - 1) {
			end = test_data.size();
		}
		threads.emplace_back(test_thread, data_span.subspan(start, end - start), results_span.subspan(start, end - start));
	}

	for (auto& thread : threads) {
		thread.join();
	}
	bool good = std::all_of(test_results.begin(), test_results.end(), [](auto result) { return result == 1; });
	if (good) {
		std::println("All tests passed");
	}
	return good;
}