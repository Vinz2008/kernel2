#include <kernel/io.h>
#include <kernel/misc.h>
#include <kernel/x86.h>

void reboot() {
  uint8_t good = 0x02;
  while (good & 0x02) {
    good = inb(0x64);
  }
  outb(0x64, 0xFE);
  x86_halt();
}

uint32_t divide_up(uint32_t n, uint32_t d) {
  if (n % d == 0) {
    return n / d;
  }

  return 1 + n / d;
}

uint32_t align_to(uint32_t n, uint32_t align) {
  if (n % align == 0) {
    return n;
  }

  return n + (align - n % align);
}