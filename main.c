#include <stdlib.h>
#include <stdio.h>

struct inst {
	char *fmt;
	char *action;
};

struct inst insts[] = {
	{ "00E0", "clrscr(); render();" },
	{ "00EE", "pc = stack[--sp]; break;" },
	{ "1NNN", "pc = N; break;" },
	{ "2NNN", "stack[sp++] = A + 2; pc = N; break;" },
	{ "3XNN", "if (reg[X] == N) { pc = A + 4; break; }" },
	{ "4XNN", "if (reg[X] != N) { pc = A + 4; break; }" },
	{ "5XY0", "if (reg[X] == reg[Y]) { pc = A + 4; break; }" },
	{ "6XNN", "reg[X] = N;" },
	{ "7XNN", "reg[X] += N;" },
	{ "8XY0", "reg[X] = reg[Y];" },
	{ "8XY1", "reg[X] |= reg[Y];" },
	{ "8XY2", "reg[X] &= reg[Y];" },
	{ "8XY3", "reg[X] ^= reg[Y];" },
	{ "8XY4", "reg[15] = (int)(reg[X] + reg[Y]) <= 0xFF; reg[X] += reg[Y];" },
	{ "8XY5", "reg[15] = reg[X] >= reg[Y]; reg[X] -= reg[Y];" },
	{ "8XY6", "reg[15] = reg[Y] & 0x1; reg[X] = reg[Y] >> 1;" },
	{ "8XY7", "reg[15] = reg[Y] >= reg[X]; reg[X] = reg[Y] - reg[X];" },
	{ "8XYE", "reg[15] = reg[Y] & 0x80 > 0; reg[X] = reg[Y] << 1;" },
	{ "9XY0", "if (reg[X] != reg[Y]) { pc = A + 4; break; }" },
	{ "ANNN", "regi = N;" },
	{ "BNNN", "pc = reg[0] + regi; break;" },
	{ "CXNN", "reg[X] = rand()&N;" },
	{ "DXYN", "draw(reg[X], reg[Y], N); render();" },
	{ "EX9E", "if (lastkey() == reg[X]) { pc = A + 4; break; }" },
	{ "EXA1", "if (lastkey() != reg[X]) { pc = A + 4; break; }" },
	{ "FX07", "reg[X] = getdelay();" },
	{ "FX0A", "reg[X] = waitkey();" },
	{ "FX15", "setdelay(X);" },
	{ "FX18", "/* set sound timer, unsup */" },
	{ "FX1E", "regi += reg[X];" },
	{ "FX29", "regi = reg[X] <= 0xF? reg[X]*5 : 16*5;" },
	{ "FX33", "bcd(reg[X]);" },
	{ "FX55", "for (i = 0; i <= X; i++) mem[regi+i] = reg[i];" },
	{ "FX65", "for (i = 0; i <= X; i++) reg[i] = mem[regi+i];" },
	{ "NNNN", "/* unknown */" },
	{ NULL },
};

int readopcode(FILE *f, int *opc) {
	unsigned char b[2];
	int r;

	r = fread(b, sizeof(char), 2, f);
	*opc = (b[0] << 8) | (b[1] << 0);
	return r;
}

void decode(int opc, char fmt[4], int *x, int *y, int *n) {
	int i = 0;

	*x = *y = *n = 0;

	for (i = 3; i >= 0; i--) {
		switch (fmt[3-i]) {
		case 'X': *x = (*x << 4) | ((opc >> (4*i)) & 0xF); break;
		case 'Y': *y = (*y << 4) | ((opc >> (4*i)) & 0xF); break;
		case 'N': *n = (*n << 4) | ((opc >> (4*i)) & 0xF); break;
		default: break;
		}
	}
}

void emit(FILE *f, char *fmt, int addr, int x, int y, int n) {
	char *p;

	for (p = fmt; *p; p++) {
		switch (*p) {
		case 'A': fprintf(f, "0x%04X", addr); break;
		case 'N': fprintf(f, "%d", n); break;
		case 'X': fprintf(f, "%d", x); break;
		case 'Y': fprintf(f, "%d", y); break;
		default: fputc(*p, f); break;
		}
	}
	fputc('\n', f);
}

int match(int opc, char fmt[4]) {
	int i;

	for (i = 3; i >= 0; i--) {
		if (fmt[i] >= '0' && fmt[i] <= '9')
			if ((opc & 0xF) != fmt[i] - '0')
				return 0;
		if (fmt[i] >= 'A' && fmt[i] <= 'F')
			if ((opc & 0xF) != fmt[i] - 'A' + 10)
				return 0;
		opc >>= 4;
	}
	return 1;
}

int main(int argc, char *argv[]) {
	FILE *f = fopen(argv[1], "r");
	int i, opc = 0, addr = 0x200;

	int x, y, n;

	fprintf(stdout,
		"#include <stdlib.h>\n"
		"#include \"impl.h\"\n"
		"\n"
		"int main() {\n"
		"	int i = 0;\n"
		"\n"
		"	init();\n"
		"\n"
		"	for (;;) {\n"
		"		switch (pc) {\n");


	while (readopcode(f, &opc) == 2) {
		for (i = 0; insts[i].fmt; i++) {
			if (match(opc, insts[i].fmt)) {
				fprintf(stdout, "\t\tcase 0x%04X: ", addr);
				decode(opc, insts[i].fmt, &x, &y, &n);
				emit(stdout, insts[i].action, addr, x, y, n);
				break;
			}
		}
		addr += 2;
	}

	fprintf(stdout,
		"		default: goto stop;\n"
		"		}\n"
		"	}\n"
		"stop:\n"
		" deinit();\n"
		"	exit(0);\n"
		"}\n");

	fclose(f);
	exit(0);
}
