#include <yahbog-tests.h>

int main(int argc, char** argv) {
	run_single_step_tests();
	run_blargg_cpu_instrs();
	run_blargg_general();
	return 0;
}