#include <yahbog.h>

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

bool run_single_step_tests();
bool run_blargg_cpu_instrs();
bool run_blargg_general();