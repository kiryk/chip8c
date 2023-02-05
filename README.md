# chip8c
As an alternative to an emulator, this tool translates CHIP-8 binaries into
equivalent C code, which can be compiled on POSIX-compliant OSes with ncurses.

This is just a proof of concept, and the way it works could be still improved
(see Details).

# Usage
Compiling the tool:
```
make
```

Translating a CHIP-8 binary:
```
./chip8c breakout.ch8
```

Running the above will produce 3 files:
1. mem.h - memory map of the binary
2. breakout.ch8.c - translated C
3. breakout.ch8.bin - runnable binary

CHIP-8 has its own keyboard layout. To interact with the binary, use mapped keys:
```
1 2 3 C --mapped as--> 1 2 3 4
4 5 6 D                Q W E R
7 8 9 E                A S D F
A 0 B F                Z X C V
```

Some CHIP-8 executables can be found on repos like
[this one](https://github.com/badlogic/chip8/tree/master/roms). The ones that
were tested and work well include `pong`, `breakout` and `trip8` demo.

# Details
What the tool was created for, is to check whether it's feasible to decompile
machine code into a long switch-case, where the switched value is the program
counter register, and the cases are instruction addresses coupled with code
performing register and IO manipulation equivalent to that of a given
instruction set.

Insides of the resulting switch-case look like the following example:
```c
[...]
case 0x0202: reg[1] = 0x001F;
case 0x0204: reg[7] &= reg[1];
case 0x0206: if (reg[7] != 0x001F) { pc = 0x0206 + 4; break; }
[...]
```

Worth noting, that breaking out from the switch-case is required only when
jumps are performed, in all other cases it suffices to allow for the default
linear flow.

There are a few disadvantages of the technique:
1. Code gets separated from data, so whatever takes advantage of von Neumann
   architecture won't work.
2. A hexdump of a translated binary must be included in the resulting C file,
   so it could access its static data.
3. Jumps to unexpected addresses aren't handled (odd addresses, for instance).

This implementation uses ncurses for IO, which was chosen for its popularity
and simplicity, but it's not a good fit for mimicking IO of CHIP-8. For instance,
the instructions `EX9E` and `EXA1` work best if the program maintains a map of
keys that are currently pressed. It seems that ncurses doesn't detect key
releases, so its hard to maintain such a map.

Also, it would be nice if Unicode block characters could be used for the
output, then neighbouring pairs of pixels could be packed into one of:
` `, `▀`, `▄` and `█` to simulate nice looking square pixels.
