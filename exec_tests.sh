#!/bin/bash

export PRINT_GSHARE_LOG=true
export PRINT_CACHE_MISSES_LOG=true

for f in benchmarks/*.bin; do
	echo "Executing $f"
	./rvsim $f 4 10
	echo 
	echo
done
