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

		auto mem_fns = yahbog::mem_fns_t{};
		mem_fns.read = [&mem](uint16_t addr) { return mem.read(addr); };
		mem_fns.write = [&mem](uint16_t addr, uint8_t value) { mem.write(addr, value); };

		yahbog::cpu cpu{};
		cpu.reset( &mem_fns );
		cpu.load_registers(initial_state.regs);
		cpu.prefetch( &mem_fns );

		bool ie = cpu.r().ie;
		for (std::size_t i = 0; i < ncycles; i++) {
			cpu.cycle( &mem_fns );
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

// sm83-main/v1/0b.json -> 0x0B
// sm83-main/v1/cb 0b.json -> 0x10B (0x0B + 0x100)
// invalid -> std::nullopt
std::optional<std::uint16_t> get_opcode_from_test_name(std::string_view name) {

	auto last_slash = name.find_last_of('/');
	if(last_slash == std::string_view::npos) {
		return std::nullopt;
	}

	auto opcode_str = name.substr(last_slash + 1);
	auto add = 0;
	if(opcode_str.starts_with("cb")) {
		opcode_str = opcode_str.substr(2);
		add = 0x100;
	}

	if(opcode_str.empty()) {
		return std::nullopt;
	}

	auto opcode = std::stoul(std::string(opcode_str), nullptr, 16);
	return opcode + add;
}

bool run_single_step_tests() {
	
	test_suite::test_suite_runner suite("SM83 Single Step Tests");
	suite.start();

	if(!std::filesystem::exists(TEST_DATA_DIR "/sm83.zip")) {
		std::cout << termcolor::red << "❌ sm83.zip not found, please run scripts/get_testing_deps.py" << termcolor::reset << "\n";
		return false;
	}

	suite.print_info("📁 Opening archive: " + std::string(TEST_DATA_DIR) + "/sm83.zip");
	std::unique_ptr<zip_t, decltype(&zip_close)> archive_ptr(zip_open(TEST_DATA_DIR "/sm83.zip", 0, 'r'), &zip_close);

	if (!archive_ptr) {
		std::cout << termcolor::red << "❌ Failed to open archive" << termcolor::reset << "\n";
		return false;
	}

	auto archive = archive_ptr.get();
	std::set<std::size_t> files{};

	const auto archive_total = zip_entries_total(archive);
	auto file_total = 0;
	for (auto i = 0; i < archive_total; i++) {
		zip_entry_openbyindex(archive, i);
		std::string_view name = zip_entry_name(archive);
		if (name.ends_with("json") && !name.ends_with("/10.json")) {
			files.insert(i);
			file_total++;
		}
		zip_entry_close(archive);
	}

	suite.print_info("🔍 Found " + std::to_string(file_total) + " tests");

	std::vector<std::string> test_data{};
	test_data.resize(file_total);
	std::vector<std::uint8_t> test_results{};
	test_results.resize(file_total);
	std::vector<std::string> test_names{};
	test_names.resize(file_total);

	std::memset(test_results.data(), 0, test_results.size());

	// Loading progress
			test_suite::progress_tracker load_progress(file_total);
	load_progress.start("Loading tests...");

	std::size_t fc = 0;
	for (auto i: files) {
		zip_entry_openbyindex(archive, i);

		test_names[fc] = zip_entry_name(archive);
		auto opcode = get_opcode_from_test_name(test_names[fc]);
		if(opcode) {
			test_names[fc] = std::format("{} ({})", test_names[fc], yahbog::opinfo[*opcode].name);
		}

		const auto size = zip_entry_size(archive);
		std::string data(size, '\0');

		zip_entry_noallocread(archive, data.data(), size);
		test_data[fc] = std::move(data);
		fc++;

		load_progress.update(fc);
		zip_entry_close(archive);
	}

	load_progress.finish();

	const auto num_threads = std::thread::hardware_concurrency();
	suite.print_info("⚡ Running tests with " + std::to_string(num_threads) + " threads...");

	// Test execution progress
	test_suite::progress_tracker test_progress(file_total);
	test_progress.start("Running...");

	const auto per_thread = test_data.size() / num_threads;
	std::vector<std::thread> threads{};
	std::atomic<int> completed_tests{0};

	std::span<const std::string> data_span{ test_data.data(), test_data.size() };
	std::span<std::uint8_t> results_span{ test_results.data(), test_results.size() };

	// Enhanced test thread function with progress tracking
	auto enhanced_test_thread = [&](std::span<const std::string> data, std::span<std::uint8_t> results) {
		for (std::size_t i = 0; i < data.size(); i++) {
			results[i] = 1;

			try {
				nlohmann::json j = nlohmann::json::parse(data[i]);

				for (auto& test : j) {
					test_info info = test_info::from_json(test);
					auto result = info.run();

					if(!result) {
						results[i] = 0;
					}
				}
			}
			catch (const std::exception& e) {
				results[i] = 0;
			}
			
			int current = completed_tests.fetch_add(1) + 1;
			test_progress.update(current);
		}
	};

	for (auto i = 0; i < num_threads; i++) {
		auto start = i * per_thread;
		auto end = (i + 1) * per_thread;
		if (i == num_threads - 1) {
			end = test_data.size();
		}
		threads.emplace_back(enhanced_test_thread, data_span.subspan(start, end - start), results_span.subspan(start, end - start));
	}

	for (auto& thread : threads) {
		thread.join();
	}

	test_progress.finish();

	// Collect results
	int passed = 0, failed = 0;
	for (std::size_t i = 0; i < test_results.size(); i++) {
		if (test_results[i] == 1) {
			passed++;
			suite.add_result(test_names[i], true, std::chrono::milliseconds(0));
		} else {
			failed++;
			suite.add_result(test_names[i], false, std::chrono::milliseconds(0));
		}
	}

	// Add extra statistics specific to single step tests
	std::stringstream extra_stats;
	extra_stats << "  " << termcolor::blue << "🧵 Threads used: " << num_threads << termcolor::reset << "\n";

	suite.add_extra_stats(extra_stats.str());
	suite.finish();
	return suite.passed();
}