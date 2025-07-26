#include <yahbog-tests.h>

#include <yahbog/emulator.h>

int main(int argc, char** argv) {
	run_single_step_tests();
	run_blargg_cpu_instrs();
	return 0;
}