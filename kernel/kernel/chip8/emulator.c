#include <kernel/chip8_emulator.h>
#include <kernel/fb.h>
#include <kernel/graphics.h>
#include <kernel/irq_handlers.h>
#include <kernel/stack.h>
#include <kernel/vfs.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool chip8_emulator_mode = false;

static fb_t fb;

extern char keyboard_us[];

static chip8_emulator_t emulator;

enum chip8_key_t {
  key_0,
  key_1,
  key_2,
  key_3,
  key_4,
  key_5,
  key_6,
  key_7,
  key_8,
  key_9,
  key_A,
  key_B,
  key_C,
  key_D,
  key_E,
  key_F,
};

#define SPRITE_CHARS_LENGTH 80

#define FPS_NUMBER 60

char sprite_chars[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

enum chip8_key_t chip8_key;

uint32_t chip8_int_to_hex(int num, int base, char sign) {
  char outbuf[10];
  memset(outbuf, 0, 10);
  int i = 12;
  int j = 0;

  do {
    outbuf[i] = "0123456789ABCDEF"[num % base];
    i--;
    num = num / base;
  } while (num > 0);

  if (sign != ' ') {
    outbuf[0] = sign;
    ++j;
  }

  while (++i < 13) {
    outbuf[j++] = outbuf[i];
  }

  outbuf[j] = 0;
  return strtol(outbuf, NULL, base);
}

void set_chip8_emulator_mode(bool mode) { chip8_emulator_mode = mode; }

bool is_chip8_emulator_mode() { return chip8_emulator_mode; }

int write_chip8_pixel(uint8_t x, uint8_t y, bool state) {
  for (int xtemp = 0; xtemp < fb.width / CHIP8_SCREEN_WIDTH; xtemp++) {
    for (int ytemp = 0; ytemp < fb.height / CHIP8_SCREEN_HEIGHT; ytemp++) {
      draw_pixel(fb, x * fb.width / CHIP8_SCREEN_WIDTH + xtemp,
                 y * fb.height / CHIP8_SCREEN_HEIGHT + ytemp,
                 (state) ? COLOR_RED : COLOR_BLACK);
    }
  }
  return 0;
}

void chip8_update_framebuffer() {
  for (int xtemp = 0; xtemp < CHIP8_SCREEN_WIDTH; xtemp++) {
    for (int ytemp = 0; ytemp < CHIP8_SCREEN_HEIGHT; ytemp++) {
      uint8_t index = xtemp + (ytemp * CHIP8_SCREEN_WIDTH);
      bool state = emulator.display[index];
      write_chip8_pixel(xtemp, ytemp, state);
    }
  }
}

void write_chip8_screen(int8_t x, int8_t y, bool state) {
  if (x > CHIP8_SCREEN_WIDTH) {
    x -= CHIP8_SCREEN_WIDTH;
  } else if (x < 0) {
    x += CHIP8_SCREEN_WIDTH;
  }
  if (y > CHIP8_SCREEN_HEIGHT) {
    y -= CHIP8_SCREEN_HEIGHT;
  } else if (y < 0) {
    y += CHIP8_SCREEN_HEIGHT;
  }
  uint8_t index = x + (y * CHIP8_SCREEN_WIDTH);
  emulator.display[index] = state;
}

void fill_chip8_screen() {
  for (int xtemp = 0; xtemp < CHIP8_SCREEN_WIDTH; xtemp++) {
    for (int ytemp = 0; ytemp < CHIP8_SCREEN_HEIGHT; ytemp++) {
      if (xtemp % 2 == 0 && ytemp % 2 == 0) {
        write_chip8_pixel(xtemp, ytemp, true);
        // write_chip8_screen(xtemp, ytemp, true);
      }
    }
  }
}

void chip8_keyboard_handler(int scan_code) {
  /*if (scan_code == ENTER_KEY){

  } else*/
  if (scan_code < 0x81) {
    char c = keyboard_us[scan_code];
    switch (c) {
    case '1':
      chip8_key = key_1;
      break;
    case '2':
      chip8_key = key_2;
      break;
    case '3':
      chip8_key = key_3;
      break;
    case '4':
      chip8_key = key_C;
      break;
    case 'q':
      chip8_key = key_4;
      break;
    case 'w':
      chip8_key = key_5;
      break;
    case 'e':
      chip8_key = key_6;
      break;
    case 'r':
      chip8_key = key_D;
      break;
    case 'a':
      chip8_key = key_7;
      break;
    case 's':
      chip8_key = key_8;
      break;
    case 'd':
      chip8_key = key_8;
      break;
    case 'f':
      chip8_key = key_E;
      break;
    case 'z':
      chip8_key = key_A;
      break;
    case 'x':
      chip8_key = key_0;
      break;
    case 'c':
      chip8_key = key_B;
      break;
    case 'v':
      chip8_key = key_F;
      break;
    default:
      log(LOG_SERIAL, false, "Chip8 emulator : unknown key pressed\n");
    }
  }
}

uint16_t chip8_get_combined_opcode(uint8_t opcode_1, uint8_t opcode_2) {
  uint16_t result = (opcode_1 << 8) | opcode_2; // TODO : fix this function
  return result;
}

void chip8_execute_instruction(uint8_t opcode_1, uint8_t opcode_2) {
  emulator.pc += 2;
  log(LOG_SERIAL, false, "chip8_get_combined_opcode(0x00, 0xE0) : %x\n",
      chip8_get_combined_opcode(0x00, 0xE0));
  log(LOG_SERIAL, false,
      "opcode 1 (in decimal) : %d, opcode 1 >> 4 : %x, opcode 1 & 0x0F : %x\n",
      opcode_1, opcode_1 >> 4, opcode_1 & 0x0F);
  // first number : opcode >> 4
  // second number : opcode & 0x0F

  switch (opcode_1 >> 4) {
  case 0x0:
    if (opcode_2 == 0xE0) {
      fill_screen(fb, COLOR_BLACK);
    } else if (opcode_2 == 0xEE) {
      // return from subroutine
      stack_pop(&emulator.stack);
    }
    break;
  case 0x1:
    emulator.pc = chip8_get_combined_opcode(opcode_1, opcode_2) & 0xFFF;
    log(LOG_SERIAL, false, "combined opcode : %x %d\n", emulator.pc,
        emulator.pc);
    break;
  case 0x2:
    stack_push(&emulator.stack, &emulator.pc);
    emulator.pc = chip8_get_combined_opcode(opcode_1, opcode_2) & 0xFFF;
    break;
  case 0x3:
    if (emulator.vregisters[opcode_1 & 0x0F] == opcode_2) {
      emulator.pc += 2;
    }
    break;
  case 0x4:
    if (emulator.vregisters[opcode_1 & 0x0F] != opcode_2) {
      emulator.pc += 2;
    }
    break;
  case 0x5:
    if (emulator.vregisters[opcode_1 & 0x0F] ==
        emulator.vregisters[opcode_2 >> 4]) {
      emulator.pc += 2;
    }
    break;
  case 0x6:
    emulator.vregisters[opcode_1 & 0x0F] = opcode_2;
    break;
  case 0x7:
    emulator.vregisters[opcode_1 & 0x0F] += opcode_2;
    break;
  case 0x8:
    switch (opcode_2 & 0x0F) {
    case 0x0:
      emulator.vregisters[opcode_1 & 0x0F] = emulator.vregisters[opcode_2 >> 4];
      break;
    case 0x1:
      emulator.vregisters[opcode_1 & 0x0F] |=
          emulator.vregisters[opcode_2 >> 4];
      break;
    case 0x2:
      emulator.vregisters[opcode_1 & 0x0F] &=
          emulator.vregisters[opcode_2 >> 4];
      break;
    case 0x3:
      emulator.vregisters[opcode_1 & 0x0F] ^=
          emulator.vregisters[opcode_2 >> 4];
      break;
    case 0x4:
      uint8_t sum = (emulator.vregisters[opcode_1 & 0x0F] +=
                     emulator.vregisters[opcode_2 >> 4]);
      emulator.vregisters[0xF] = 0;
      /*if (sum > 0xFF) {
        emulator.vregisters[0xF] = 1;
      }*/
      emulator.vregisters[opcode_1 & 0x0F] = sum;
      break;
    case 0x5:
      emulator.vregisters[0xF] = 0;
      if (emulator.vregisters[opcode_1 & 0x0F] >
          emulator.vregisters[opcode_2 >> 4]) {
        emulator.vregisters[0xF] = 1;
      }
      emulator.vregisters[opcode_1 & 0x0F] -=
          emulator.vregisters[opcode_2 >> 4];
      break;
    case 0x6:
      emulator.vregisters[0xF] = (emulator.vregisters[opcode_1 & 0x0F] & 0x1);
      emulator.vregisters[opcode_1 & 0x0F] >>= 1;
      break;
    }
    break;
  case 0x9:
    if (emulator.vregisters[opcode_1 & 0x0F] !=
        emulator.vregisters[opcode_2 >> 4]) {
      emulator.pc += 2;
    }
    break;
  case 0xA:
    emulator.i = chip8_get_combined_opcode(opcode_1, opcode_2) & 0xFFF;
    break;
  case 0xB:
    emulator.pc = (chip8_get_combined_opcode(opcode_1, opcode_2) & 0xFFF) +
                  emulator.vregisters[0];
    break;
  case 0xC:
    int rand_nb = rand() % 256; // number between 0 and 255
    emulator.vregisters[opcode_1 & 0x0F] = rand_nb & (opcode_2);
    break;
  case 0xD:
    uint8_t sprite_width = 8;
    uint8_t sprite_height = (opcode_2 & 0x0F);
    emulator.vregisters[0xF] = 0;
    for (int row = 0; row < sprite_height; row++) {
      uint8_t sprite = emulator.ram[emulator.i + row];
      for (int col = 0; col < sprite_width; col++) {
        if ((sprite & 0x80) > 0) {
          log(LOG_SERIAL, false, "Write to screen pixel\n");
          write_chip8_pixel(emulator.vregisters[opcode_1 & 0x0F] + col,
                            emulator.vregisters[opcode_2 >> 4] + row, true);
          // set emulator.vregisters[0xF] to 1 if the pixel was erased
        }
        sprite <<= 1;
      }
    }
    break;
  case 0xF:
    switch (opcode_2) {
    case 0x07:
      emulator.vregisters[opcode_1 & 0x0F] = emulator.delay_timer;
      break;
    case 0x15:
      emulator.delay_timer = emulator.vregisters[opcode_1 & 0x0F];
      break;
    case 0x1E:
      emulator.i += emulator.vregisters[opcode_1 & 0x0F];
      break;
    case 0x29:
      emulator.i = emulator.vregisters[opcode_1 & 0x0F] * 5;
      break;
    case 0x55:
      for (int i = 0; i <= emulator.vregisters[opcode_1 & 0x0F]; i++) {
        emulator.ram[emulator.i + i] = emulator.vregisters[i];
      }
      break;
    case 0x65:
      for (int i = 0; i <= emulator.vregisters[opcode_1 & 0x0F]; i++) {
        emulator.vregisters[i] = emulator.ram[emulator.i + i];
      }
      break;
    }
    break;
  default:
    log(LOG_SERIAL, false, "Unknown opcode opcode_1 : %d, opcode_2 : %d\n",
        opcode_1, opcode_2);
  }
}

void chip8_update_timers() {
  if (emulator.delay_timer > 0) {
    emulator.delay_timer -= 1;
  }
}

int chip8_cycle() {
  for (int i = 0; i < emulator.speed; i++) {
    if (!emulator.paused) {
      uint8_t opcode_1 = emulator.ram[emulator.pc];
      uint8_t opcode_2 = emulator.ram[emulator.pc + 1];
      chip8_execute_instruction(opcode_1, opcode_2);
    }
  }
  if (!emulator.paused) {
    chip8_update_timers();
  }
  chip8_update_framebuffer();
  return 0;
}

void chip8_mainloop() {
  while (true) {
    if (!chip8_cycle()) {
      break;
    }
  }
}

void chip8_load_program_into_ram(uint8_t* buf, size_t buf_length) {
  for (int i = 0; i < buf_length; i++) {
    emulator.ram[0x200 + i] = buf[i];
  }
}

void chip8_load_sprites_into_ram() {
  for (int i = 0; i < SPRITE_CHARS_LENGTH; i++) {
    emulator.ram[i] = sprite_chars[i];
  }
}

void setup_chip8_emulator(const char* filename) {
  set_chip8_emulator_mode(true);
  emulator.pc = 0x200;
  emulator.speed = 10;
  emulator.paused = false;
  emulator.stack = NULL;
  memset(emulator.display, false, CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT);

  fb = fb_get_info();

  setup_custom_keypress_function(chip8_keyboard_handler);
  // fill_screen(fb, COLOR_BLUE);
  log(LOG_SERIAL, false, "TEST ROM");
  fs_node_t* rom_node = vfs_open(filename, "r");
  uint8_t buffer[rom_node->length];
  log(LOG_SERIAL, false, "rom_node->length : %d\n", rom_node->length);
  if (rom_node == NULL) {
    log(LOG_SERIAL, false, "rom file not found : %s\n", filename);
  }
  read_fs(rom_node, 0, rom_node->length, buffer);
  for (int i = 0; i < rom_node->length; i++) {
    log(LOG_SERIAL, false, "chip instruction (in decimal) : %d\n", buffer[i]);
  }
  chip8_load_program_into_ram(buffer, rom_node->length);
  /*for (pc = 0; pc < rom_node->length; pc+=2){
      chip8_execute_instruction(buffer[pc], buffer[pc+1]);
  }*/
  // fill_chip8_screen();
  // chip8_update_framebuffer();
  while (1) {
    emulator.pc = 0x200; // these lines were added to the chip8_mainloop call
                         // just for debugging
    chip8_mainloop();
    log(LOG_SERIAL, false, "execution ended\n");
  }
}