#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif
// Source - https://stackoverflow.com/q/2488563
// Posted by skydoor, modified by community. See post 'Timeline' for change history
// Retrieved 2026-09-14, License - CC BY-SA 2.5

char *
strcat(char *dest, const char *src)
{
    size_t i,j;
    for (i = 0; dest[i] != '\0'; i++)
        ;
    for (j = 0; src[j] != '\0'; j++)
        dest[i+j] = src[j];
    dest[i+j] = '\0';
    return dest;
}

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif
char ps2kd[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', /* <-- Tab */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     
    0, /* <-- control key */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
   0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

uint8_t inb(uint16_t port) {
    uint8_t result;
    
    // "=a" indica che il risultato sarà memorizzato nel registro AL (accumulatore a 8 bit)
    // "dN" indica che il numero di porta deve andare nel registro DX (o essere una costante immediata)
    __asm__ __volatile__ (
        "inb %1, %0"
        : "=a" (result)
        : "dN" (port)
    );
    
    return result;
}
void outb(uint16_t port, uint8_t data) {
    // "a" (data) inserisce il dato nel registro AL (accumulatore a 8 bit)
    // "dN" (port) inserisce l'indirizzo della porta nel registro DX (o usa un valore immediato se costante)
    __asm__ __volatile__ (
        "outb %0, %1"
        :
        : "a" (data), "dN" (port)
    );
}
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}
void disable_cursor()
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000 

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;
void update_cursor(int x, int y)
{
	uint16_t pos = y * VGA_WIDTH + x;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) 
{
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
	
}

void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		{ terminal_putchar(data[i]);  }
}
// Source - https://stackoverflow.com/a/34873406
// Posted by Gianluca Ghettini, modified by community. See post 'Timeline' for change history
// Retrieved 2026-09-14, License - CC BY-SA 4.0

int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
	terminal_column = 0;
	update_cursor(strlen(data), terminal_row);
}

void kernel_main(void) 
{
	char irps2kdv[4096];
	char buf[4096];
	/* Initialize terminal interface */
	terminal_initialize();
	terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
	char ls;
	unsigned char s;
	unsigned char c;
	int text_x = strlen("@ studio .: ");
	int text_y = 0;
	size_t gotx = strlen("@ studio .: ");
	size_t goty = terminal_row;
	terminal_writestring("@ studio .: ");
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	while (1) {
		s = inb(0x64);
		c = inb(0x60);
		if (ls == c) continue;
		ls = c;
		if (c == 0xE0) continue;
		if (c & 0x80) {
		unsigned char rels = c & -0x80;
		continue;
		}
		char ps2kdv = ps2kd[c];
		if (c != 0x0E && ps2kdv != '\n') {
		terminal_putentryat(ps2kdv, terminal_color, text_x, terminal_row);
		text_x = text_x + 1;
		char *extrakb = malloc(len + 1 + 1);
		strcpy(extrakb, buf);
		extrakb[strlen(buf)] = ps2kdv;
		extrakb[strlen(buf) + 1] = '\0';
		free(extrakb);
		update_cursor(text_x, terminal_row);
		} else if (ps2kdv == '\n') {
			
			text_x = strlen("@ studio .: ");
			terminal_row = terminal_row + 1;
			terminal_writestring("meow");
			terminal_row = terminal_row + 1;
			terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
			terminal_writestring("@ studio .: ");
			terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
		}
	}
}
