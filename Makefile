build:
	@echo "Compiling."
	g++ libs/*.cpp -I include/ -I $$DINERO_INSTALL_DIR -L $$DINERO_INSTALL_DIR -ld4 -o rvsim  

test:
	@echo "Running an sample program with comparison"
	./rvsim testes/add.c.bin > output.txt
	diff output.txt testes/add.correct
	rm output.txt
	
run:
	@echo "Running an sample program"
	./rvsim testes/add.c.bin

clean:
	@echo "Cleaning"
	rm rvsim