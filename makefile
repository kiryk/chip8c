all:
	cc -ansi -o chip8tr main.c

clean:
	rm chip8tr

cleanall: clean
	rm mem.h *ch8.bin *.ch8.c

.PHONY: clean cleanall
