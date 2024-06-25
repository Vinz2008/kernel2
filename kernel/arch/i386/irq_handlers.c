#include <kernel/chip8_emulator.h>
#include <kernel/config.h>
#include <kernel/io.h>
#include <kernel/irq_handlers.h>
#include <kernel/keyboard.h>
#include <kernel/misc.h>
#include <kernel/pic.h>
#include <kernel/pit.h>
#include <kernel/ps2.h>
#include <kernel/serial.h>
#include <kernel/tty_framebuffer.h>
#include <kernel/x86.h>
#include <stddef.h>
#include <stdio.h>
#include <types.h>

#if GUI_MODE
#include <kernel/gui.h>
#endif

extern uint32_t simple_sc_to_kc[];
static uint32_t device;
static kbd_context_t context;
static void* irq_routines[16] = {0};
extern char keyboard_us[];
extern char keyboard_us_shift[];
bool key_pressed[128];

void irq_register_handler(int irq, void (*handler)(registers_t*)) {

  if ((NULL == handler) || (irq < 0) || (irq > 15)) {
    return;
  }
  irq_routines[irq] = handler;
}

void irq_unregister_handler(int irq) { irq_routines[irq] = 0; }

void handle_platform_irq(registers_t* frame) {

  void (*handler)(registers_t* frame);
  uint32_t irq = frame->vector - 32;

  handler = irq_routines[irq];

  if (handler) {
    handler(frame);
  }

  if (irq == IRQ_PIT) {
    return;
  }

  pic_send_EOI(irq);
}

// TODO : remove this ?
/*void sys_sleep(int seconds){
    log(LOG_SERIAL, false, "seconds to wait : %d\n", seconds);
    uint64_t sleep_ticks = ticks + (seconds * SYSTEM_TICKS_PER_SEC);
    log(LOG_SERIAL, false, "start tick : %d\n", ticks);
    log(LOG_SERIAL, false, "end tick : %d\n", sleep_ticks);
    while(ticks < sleep_ticks){
        log(LOG_SERIAL, false, "tick : %d\n", ticks);
    }
}*/
void sys_mouse_handler(registers_t* frame) {
  log(LOG_SERIAL, false, "mouse moved\n");
  return;
}

void (*custom_keypress_function)(int) = NULL;

void setup_custom_keypress_function(void (*f)(int)) {
  custom_keypress_function = f;
}

void sys_key_handler(registers_t* frame) {
  (void)frame;
  // scan code https://wiki.osdev.org/PS/2_Keyboard
  // https://github.com/29jm/SnowflakeOS/blob/master/kernel/src/devices/kbd.c
  uint8_t scan_code = inb(0x60);
  if (scan_code == 0xE0) {
    return; // shift pressed
  }
  // uint8_t scan_code = ps2_read(PS2_DATA);
  // log(LOG_SERIAL, false, "scan code : %d\n", scan_code);
  bool pressed = 1;
  if (scan_code >= 128) {
    pressed = false;
    scan_code -= 128;
  }
  key_pressed[scan_code] = pressed;
  if (!pressed) {
    return;
  }
  if (scan_code == ESC_KEY) { // ESC - pressed
    reboot();
  }
  if (custom_keypress_function != NULL) {
    custom_keypress_function(scan_code);
    return;
  }
#if GUI_MODE
  log(LOG_SERIAL, false, "key pressed : %d\n", scan_code);
  if (scan_code & 0x80) {
    log(LOG_SERIAL, false, "modifer key/special key pressed : %d\n", scan_code);
  } else if (scan_code == 0x4B) {
    log(LOG_SERIAL, false, "left\n");
    move_focused_window_wm(LEFT);
  } else if (scan_code == 0x48) {
    log(LOG_SERIAL, false, "up\n");
    move_focused_window_wm(UP);
  } else if (scan_code == 0x4D) {
    log(LOG_SERIAL, false, "right\n");
    move_focused_window_wm(RIGHT);
  } else if (scan_code == 0x50) {
    log(LOG_SERIAL, false, "down\n");
    move_focused_window_wm(DOWN);
  } /*else if (scan_code == ESC_KEY){
      close_window(get_focused_window());
  }*/
  else if (scan_code < 0x81) {
    char c = keyboard_us[scan_code];
    if (c == 'n') {
      window_t* win = open_window("test window", 300, 125, 0);
      draw_string(win->fb, "Lorem Ipsum", 45, 55, 0x00AA1100);
      render_window(win);
    } else if (c == 'q') {
      close_window(get_focused_window());
    } else {
      gui_keypress(c);
    }
  }

#else
  if (scan_code == ENTER_KEY) { // ENTER - pressed
    launch_command_framebuffer();
    empty_line_cli_framebuffer();
    if (!is_chip8_emulator_mode()) {
      printf("\n> ");
    }
  } else if (scan_code == DELETE_KEY ||
             scan_code == BACKSPACE_KEY) { // DELETE - pressed
    write_serialf("delete pressed\n");
    terminal_framebuffer_delete_character();
    // bug with delete character (wrong keycode)
  } else if (scan_code == CURSOR_LEFT_KEY) {
    // TODO : implement cursor
  } else if (scan_code == CURSOR_UP_KEY) {
    // TODO : print last command
  } else if (scan_code < 0x81) {
    char c = keyboard_us[scan_code];
    terminal_framebuffer_keypress(c);
  }
#endif
}