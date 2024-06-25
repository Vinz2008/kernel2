#include <ctype.h>
#include <kernel/chip8_emulator.h>
#include <kernel/fb.h>
#include <kernel/initrd.h>
#include <kernel/io.h>
#include <kernel/misc.h>
#include <kernel/paging.h>
#include <kernel/serial.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <multiboot.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char keyboard_us[];
// extern multiboot_module_t *mod;

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
// static uint16_t* const VGA_MEMORY = (uint16_t*) PHYS_TO_VIRT(0xB8000);
static uint16_t* VGA_MEMORY = (uint16_t*)0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint8_t old_terminal_color;
static uint16_t* terminal_buffer;
static size_t terminal_keypress_index = 0;
static size_t terminal_tick_index = 0;
int IsBacklash = false;
bool tick_animation_enabled = true;
bool keypress_animation_enabled = true;

int x_pos_cursor;
int y_pos_cursor;

static char line_cli[100];
char last_line_cli[100];

void empty_line_cli() { memset(line_cli, 0, 100); }

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
  outb(0x3D4, 0x0A);
  outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
  outb(0x3D4, 0x0B);
  outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void disable_cursor() {
  outb(0x3D4, 0x0A);
  outb(0x3D5, 0x20);
}

void update_cursor(int x, int y) {
  x_pos_cursor = x;
  y_pos_cursor = y;
  uint16_t pos = y * VGA_WIDTH + x;
  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

uint16_t get_cursor_position(void) {
  uint16_t pos = 0;
  outb(0x3D4, 0x0F);
  pos |= inb(0x3D5);
  outb(0x3D4, 0x0E);
  pos |= ((uint16_t)inb(0x3D5)) << 8;
  return pos;
}

int get_cursor_position_x() { return x_pos_cursor; }

int get_cursor_position_y() { return y_pos_cursor; }

void move_cursor_right() {
  uint16_t x = x_pos_cursor;
  uint16_t y = y_pos_cursor;
  x++;
  update_cursor(x, y);
}

void move_cursor_left() {
  uint16_t x = x_pos_cursor;
  uint16_t y = y_pos_cursor;
  x--;
  update_cursor(x, y);
}

void move_cursor_next_line() {
  uint16_t y = y_pos_cursor;
  y++;
  update_cursor(3, y);
}

void move_cursor_last_line() {
  uint16_t y = y_pos_cursor;
  y--;
  update_cursor(3, y);
}

void terminal_clear() {
  terminal_row = 0;
  terminal_column = 0;
  terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  terminal_buffer = VGA_MEMORY;
  for (size_t y = 0; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t index = y * VGA_WIDTH + x;
      terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
  }
  // terminal_ps1();
  terminal_disable_tick();
  terminal_disable_keypress();
  update_cursor(3, 1);
}

void terminal_initialize() {
  // VGA_MEMORY = (uint16_t*) PHYS_TO_VIRT(0xB8000);
  // fb_t fb = fb_get_info();
  // VGA_MEMORY = (uint16_t*) fb.address;
  log(LOG_SERIAL, false, "vga memory : %p\n", VGA_MEMORY);
  enable_cursor(0, 0);
  terminal_clear();
  // update_cursor(3, 17);
}

void terminal_setcolor(uint8_t color) {
  old_terminal_color = terminal_color;
  terminal_color = color;
}

void terminal_setcolors(uint8_t fg, uint8_t bg) {
  old_terminal_color = terminal_color;
  terminal_color = vga_entry_color(fg, bg);
}

void terminal_reset_color() { terminal_color = old_terminal_color; }

void terminal_ps1() {
  if (!is_chip8_emulator_mode()) {
    printf("> ");
  }
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
  if (y >= VGA_HEIGHT || (y >= VGA_HEIGHT && x >= VGA_WIDTH)) {
    terminal_clear();
    terminal_ps1();
    move_cursor_last_line();
    move_cursor_right();
  }
  const size_t index = y * VGA_WIDTH + x;
  terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
  unsigned char uc = c;
  switch (uc) {
  case '\n':
    terminal_row++;
    terminal_column = 0;
    move_cursor_next_line();
    break;
  case '\t':
    for (int i = 0; i < 4; i++) {
      terminal_putchar(' ');
    }
  default:
    terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
    break;
  }
  if (++terminal_column == VGA_WIDTH) {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT) {
      terminal_row = 0;
    }
  }
}

void terminal_write(const char* data, size_t size) {
  bool controlSequence = false;
  for (size_t i = 0; i < size; i++) {
    log(LOG_SERIAL, false, "data[i] : %c in ascii %d\n", data[i], data[i]);
    switch (data[i]) {
    case '\x1B':
      log(LOG_SERIAL, false, "found escape character\n");
      if (data[i + 1] == '[') {
        controlSequence = true;
        i++;
      }
      break;
    /*case '\x9B':
            controlSequence = true;
            break;*/
    default:
      /*if (controlSequence == true){
              log(LOG_SERIAL, false, "control sequence found\n");
      } else {*/
      terminal_putchar(data[i]);
      //}
      break;
    }
  }
}

void terminal_writestring(const char* data) {
  terminal_write(data, strlen(data));
}

void terminal_keypress_init(uint8_t n) {
  terminal_keypress_index = VGA_WIDTH * terminal_row + n;
}

void terminal_enable_tick() { tick_animation_enabled = true; }

void terminal_tick_init(uint8_t n) {
  terminal_tick_index = VGA_WIDTH * terminal_row + n;
}

void terminal_tick(char c) {
  terminal_buffer[terminal_tick_index] = vga_entry(c, terminal_color);
}

void terminal_disable_tick() { tick_animation_enabled = false; }

void terminal_keypress(uint8_t scan_code) {
  char c = keyboard_us[scan_code];
  append(line_cli, c);
  if (keypress_animation_enabled == true) {
    terminal_buffer[terminal_keypress_index] = vga_entry(c, terminal_color);
  }
  // printf("%c", c);
  terminal_putentryat(c, terminal_color, get_cursor_position_x(),
                      get_cursor_position_y() - 1);
  move_cursor_right();
}

void terminal_enable_keypress() { keypress_animation_enabled = true; }

void terminal_disable_keypress() { keypress_animation_enabled = false; }

void launch_command() {
  if (startswith("clear", line_cli)) {
    terminal_clear();
  } else if (startswith("echo", line_cli)) {
    char temp[95];
    int i2 = 5;
    for (int i = 0; i < strlen(line_cli); i++) {
      temp[i] = line_cli[i2];
      i2++;
    }
    printf("\n%s", temp);
  } else if (startswith("ls", line_cli)) {
    uint32_t addr = get_initrd_address();
    initrd_list_filenames(addr);
  } else if (startswith("help", line_cli)) {
    printf("\nusage help : \n");
    printf("echo : print string\n");
    printf("ls : list directory and files\n");
    printf("clear : clear screen\n");
    printf("reboot : reboot the computer\n");
    printf("thirdtemple : ...\n");
    printf("help : print this help\n");
  } else if (startswith("arch", line_cli)) {
    printf("\ni386");
  } else if (startswith("gui", line_cli)) {

  } else if (startswith("reboot", line_cli)) {
    reboot();
  } else if (startswith("beep", line_cli)) {
    // beep();
  } else if (startswith("sleep", line_cli)) {
    char temp[95];
    int i2 = 6;
    for (int i = 0; i < strlen(line_cli); i++) {
      if (!isalnum(line_cli[i2])) {
        break;
      }
      printf("\nc[%d] : %c\n", i2, line_cli[i2]);
      temp[i] = line_cli[i2];
      i2++;
    }
    // int seconds = atoi(temp);
    // sys_sleep(seconds);
  } else if (startswith("thirdtemple", line_cli)) {
    printf("\nIf you search the third temple of god, you are in the wrong OS. "
           "\n Install TempleOS");
  } else {
    printf("\ncommand not found");
  }
  strcpy(last_line_cli, line_cli);
}

void terminal_print_last_command() {
  int temp_pos_column = 0;
  for (int i = 0; i < strlen(last_line_cli); i++) {
    empty_line_cli();
    terminal_putentryat(last_line_cli[i], terminal_color, terminal_row,
                        temp_pos_column);
  }
}

void remove_character() {
  if (terminal_column > 1) {
    terminal_putentryat(' ', terminal_color, terminal_column--, terminal_row);
    move_cursor_left();
  }
}