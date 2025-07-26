#include <yahbog-tests.h>

int main(int argc, char** argv) {

	auto all_passed = true;

	all_passed &= run_single_step_tests();
	all_passed &= run_blargg_cpu_instrs();
	all_passed &= run_blargg_general();

	return all_passed ? 0 : 1;
}