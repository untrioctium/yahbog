#include <yahbog/opinfo.h>
#include <yahbog/cpu.h>

#include <nlohmann/json.hpp>
#include <termcolor/termcolor.hpp>

#include <print>
#include <iostream>
#include <zip.h>
#include <filesystem>

#include <memory>
#include <thread>
#include <span>
#include <unordered_map>
#include <set>

#define TEST_DATA_DIR "testdata"

bool download_to_path(std::string_view url, std::string_view path);
void clear_console_line();

bool run_single_step_tests();
bool run_blargg_cpu_instrs();
bool run_blargg_general();