#!/bin/bash
BIN=fuzz_readwritetest
INDIR=fuzz_test_inputs
OUTDIR=fuzz_test_outputs

#set up folders
rm -f $BIN
rm -rf $OUTDIR
mkdir $OUTDIR

#build for fuzzing
afl-gcc -g -o $BIN main_test.c apg_bmp.c

#run with fuzz
AFL_EXIT_WHEN_DONE=1 AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
AFL_SKIP_CPUFREQ=1 afl-fuzz -i $INDIR/ -o $OUTDIR/ -- ./$BIN @@ $OUTDIR/current_out.bmp	
