#include <kernel/descriptors_tables.h>
#include <kernel/tss.h>
#include <string.h>
#include <types.h>

tss_entry_t tss_entry;

void write_tss(/*struct gdt_entry* g*/ int32_t num, uint16_t ss0,
               uint32_t esp0) {

  uint32_t base = (uint32_t)&tss_entry;
  uint32_t limit = base + sizeof(tss_entry);
  set_gdt_entry(num, base, limit, 0xE9, 0x00);
  memset(&tss_entry, 0, sizeof(tss_entry));
  tss_entry.ss0 = ss0;
  tss_entry.esp0 = esp0;
  tss_entry.cs = 0x0b;
  tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs =
      0x13;
  tss_entry.iomap_base = sizeof(tss_entry);
}

void set_kernel_stack(uint32_t stack) { // Used when an interrupt occurs
  tss_entry.esp0 = stack;
}

void install_tss() {
  tss_entry.ss0 = 0x10;
  tss_entry.iomap_base = (uint16_t)sizeof(tss_entry_t);
}
