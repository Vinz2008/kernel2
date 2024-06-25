#include <stdio.h>

#if defined(__is_libk)
#include <kernel/config.h>
#include <kernel/gui.h>
#include <kernel/tty_framebuffer.h>
#endif

int putchar(int ic) {
#if defined(__is_libk)
  char c = (char)ic;
#if GUI_MODE
  // draw_string(&c, 100, 250, 0xFFFFFF);
  // putchar_gui(c);
#else
  terminal_framebuffer_putc(c);
  // terminal_framebuffer_putc_back(c);
#endif
#else
  // TODO: Implement stdio and the write system call.
#endif
  return ic;
}
