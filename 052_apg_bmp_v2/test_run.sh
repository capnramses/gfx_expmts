#!/bin/bash
SAVEIFS=$IFS
IFS=$(echo -en "\n\b")
cd fuzz_test_inputs/
for filename in *.*; do
	../readwritetest $filename ../test_output/$filename
	#if [[ $ret != 0 ]]; then exit $ret; fi
done
cd ..
IFS=$SAVEIFS
