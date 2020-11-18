#!/bin/bash

for f in testes/bin/*.bin; do
	echo "Executing $f"
	./rvsim $f 4 10
	echo 
	echo
done
