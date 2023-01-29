#define MEMSZ 0x1000 /* memory size */
#define SCRSZ 0x100  /* screen size */
#define SCRW 64     /* screen width */
#define SCRH 32     /* screen height */

/* registers */
static unsigned short stack[10000];
static unsigned char reg[128] = {0};
static unsigned short regi = 0, regs = 0, regd = 0;
static int sp = 0, pc = 0x200;

/* memory with predefined fonts */
static unsigned char mem[MEMSZ] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,
	0x10, 0x10, 0x10, 0x10, 0x10,
	0xF0, 0x10, 0xF0, 0x80, 0xF0,
	0xF0, 0x10, 0xF0, 0x10, 0xF0,
	0x90, 0x90, 0xF0, 0x10, 0x10,
	0xF0, 0x80, 0xF0, 0x10, 0xF0,
	0xF0, 0x80, 0xF0, 0x90, 0xF0,
	0xF0, 0x10, 0x10, 0x10, 0x10,
	0xF0, 0x90, 0xF0, 0x90, 0xF0,
	0xF0, 0x90, 0xF0, 0x10, 0xF0,
	0xF0, 0x90, 0xF0, 0x90, 0x90,
	0xE0, 0x90, 0xE0, 0x90, 0xE0,
	0xF0, 0x80, 0x80, 0x80, 0xF0,
	0xE0, 0x90, 0x90, 0x90, 0xE0,
	0xF0, 0x80, 0xF0, 0x80, 0xF0,
	0xF0, 0x80, 0xF0, 0x80, 0x80,
	0x60, 0x60, 0x60, 0x00, 0x60,
};

static unsigned char *scr[] = {
	mem + 0xF00, mem + 0xF08, mem + 0xF10, mem + 0xF18,
	mem + 0xF20, mem + 0xF28, mem + 0xF30, mem + 0xF38,
	mem + 0xF40, mem + 0xF48, mem + 0xF50, mem + 0xF58,
	mem + 0xF60, mem + 0xF68, mem + 0xF70, mem + 0xF78,
	mem + 0xF80, mem + 0xF88, mem + 0xF90, mem + 0xF98,
	mem + 0xFA0, mem + 0xFA8, mem + 0xFB0, mem + 0xFB8,
	mem + 0xFC0, mem + 0xFC8, mem + 0xFD0, mem + 0xFD8,
	mem + 0xFE0, mem + 0xFE8, mem + 0xFF0, mem + 0xFF8,
};

void render(void)
{
	int i, x, y, hi, lo;

	fputs("\033c", stdout);
	for (y = 0; y < SCRH; y += 2) {
		for (x = 0; x < SCRW / 8; x++) {
			for (i = 7; i >= 0; i--) {
				hi = (scr[y+0][x] >> i) & 0x1;
				lo = (scr[y+1][x] >> i) & 0x1;
				fputs(!hi && !lo? " " :
				      !hi &&  lo? "▄" :
							 hi && !lo? "▀" : "█", stdout);
			}
		}
		fputs("\n", stdout);
	}
	for (i = 0; i < 10000000; i++)
		;
}

void clear(void)
{
	int x, y;

	for (x = SCRW / 8 - 1; x >= 0; x--)
		for (y = SCRH - 1; y >= 0; y--)
			scr[y][x] = 0;
}

static void draw(int x, int y, int n)
{
	int i;

	reg[15] = 0;
	for (i = 0; i < n; ++i, ++y) {
		if (y >= SCRH)
			break;
		if (reg[15] == 0 && (scr[y][x/8] & mem[regi + i] >> x % 8))
			reg[15] = 1;
		scr[y][x/8] ^= (mem[regi + i] >> x % 8) & 0xFF;

		if (x + 8 >= SCRW || x % 8 == 0)
			continue;
		if (reg[15] == 0 && (scr[y][x/8 + 1] & mem[regi + i] << (8 - x % 8)))
			reg[15] = 1;
		scr[y][x/8 + 1] ^= (mem[regi + i] << (8 - x % 8)) & 0xFF;
	}
}

static void bcd(int x)
{
	mem[regi + 2] = x % 10, x /= 10;
	mem[regi + 1] = x % 10, x /= 10;
	mem[regi] = x % 10;
}

static int key()
{
	int n = 0;

	printf("> ");
	fflush(stdout);
	scanf("%x", &n);

	return n;
}
