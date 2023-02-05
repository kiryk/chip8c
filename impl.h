#include <time.h>
#include <curses.h>

#define CLKHZ 60     /* sound and delay timers speed */
#define MEMSZ 0x1000 /* memory size */
#define SCRSZ 0x100  /* screen size */
#define SCRW 64      /* screen width */
#define SCRH 32      /* screen height */

/* registers */
static unsigned short stack[10000];
static unsigned char reg[128] = {0};
static unsigned short regi = 0, regd = 0;
static int sp = 0, pc = 0x200;
static int regk;

static struct timespec regd_timer;

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
	[0x200]
	#include "mem.h"
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

static void init(void)
{
	initscr();
}

static void deinit(void)
{
	endwin();
}

static void render(void)
{
	int i, x, y, hi, lo;

	clear();
	for (y = 0; y < SCRH; y ++) {
		for (x = 0; x < SCRW / 8; x++) {
			for (i = 7; i >= 0; i--) {
				printw((scr[y][x] >> i) & 0x1? "##" : "  ");
			}
		}
		printw("\n");
	}
	refresh();
	napms(20);
}

static void clrscr(void)
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

static int mapkey(char ch)
{
	if (ch >= 'a' && ch <= 'z')
		ch -= ('a' - 'A');
	switch (ch) {
		case 'X': return 0x0;
		case '1': return 0x1;
		case '2': return 0x2;
		case '3': return 0x3;
		case 'Q': return 0x4;
		case 'W': return 0x5;
		case 'E': return 0x6;
		case 'A': return 0x7;
		case 'S': return 0x8;
		case 'D': return 0x9;
		case 'Z': return 0xA;
		case 'C': return 0xB;
		case '4': return 0xC;
		case 'R': return 0xD;
		case 'F': return 0xE;
		case 'V': return 0xF;
		default:  return ch;
	}
}

static int lastkey(void)
{
	int k;

	timeout(0);
	k = getch();
	if (k != ERR)
		regk = mapkey(k);
	return regk;
}

static int waitkey(void)
{
	int k;

	timeout(-1);
	k = getch();
	if (k != ERR)
		regk = mapkey(k);
	return regk;
}

static void setdelay(int n)
{
	clock_gettime(CLOCK_REALTIME, &regd_timer);
	regd = n;
}

static int getdelay(void)
{
	struct timespec ts;
	unsigned long time_a, time_b;

	clock_gettime(CLOCK_REALTIME, &ts);

	time_b = ts.tv_sec * 100 + (ts.tv_nsec / 10000000);
	time_a = regd_timer.tv_sec * 100 + (regd_timer.tv_nsec / 10000000);

	time_a = (time_a - time_b) * CLKHZ / 100;

	return regd >= time_a? regd - time_a : 0;
}
