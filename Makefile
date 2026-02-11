all:
	@gcc src/main.c -o build/main -lm
run: all
	@./build/main
