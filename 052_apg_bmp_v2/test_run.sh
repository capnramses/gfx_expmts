#!/bin/bash
SAVEIFS=$IFS
IFS=$(echo -en "\n\b")
mkdir test_outputs
cd test_inputs/
for filename in *.*; do
	../readwritetest $filename ../test_outputs/$filename
	#if [[ $ret != 0 ]]; then exit $ret; fi
done
cd ..
IFS=$SAVEIFS
