#!/bin/bash
#build for fuzzing
afl-gcc -g -o fuzz_readwritetest main_test.c apg_bmp.c
mkdir fuzz_test_outputs/
#run with fuzz
AFL_EXIT_WHEN_DONE=1 AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
AFL_SKIP_CPUFREQ=1 afl-fuzz -i fuzz_test_inputs/ -o fuzz_test_outputs/ -- ./fuzz_readwritetest @@ current_out.bmp	
