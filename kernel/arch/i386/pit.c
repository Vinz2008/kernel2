
#include <kernel/config.h>
#include <kernel/io.h>
#include <kernel/irq_handlers.h>
#include <kernel/pic.h>
#include <kernel/pit.h>
#include <kernel/x86.h>
#include <stdio.h>
#include <types.h>

extern bool tick_animation_enabled;
static uint64_t timer_delta_time;
static uint16_t divisor;
static volatile uint64_t ticks = 0;

/* PIT (i8253) registers */
#define I8253_CONTROL_REG 0x43
#define I8253_DATA_REG 0x40

#define INTERNAL_FREQ 1193182ULL
#define INTERNAL_FREQ_3X 3579546ULL

void increment_tick() { ticks++; }

void pit_init(uint32_t frequency) {
  uint32_t count, remainder;
  /* figure out the correct divisor for the desired frequency */
  if (frequency <= 18) {
    count = 0xffff;
  } else if (frequency >= INTERNAL_FREQ) {
    count = 1;
  } else {
    count = INTERNAL_FREQ_3X / frequency;
    remainder = INTERNAL_FREQ_3X % frequency;
    if (remainder >= INTERNAL_FREQ_3X / 2) {
      count += 1;
    }
    count /= 3;
    remainder = count % 3;

    if (remainder >= 1) {
      count += 1;
    }
  }
  divisor = count & 0xffff;
  timer_delta_time = (3685982306ULL * count) >> 10;
  outb(I8253_CONTROL_REG, 0x34);
  outb(I8253_DATA_REG, divisor & 0xff); // LSB
  outb(I8253_DATA_REG, divisor >> 8);   // MSB
}

void pit_sleep(uint64_t t) {
  uint64_t rest_ticks = ticks + t;
  log(LOG_SERIAL, false, "ticks : %d\n", ticks);
  log(LOG_SERIAL, false, "rest_ticks : %d\n", rest_ticks);
  while (ticks < rest_ticks) {
    log(LOG_SERIAL, false, "ticks in loop : %d\n", ticks);
  }
}

void sys_tick_handler(registers_t* frame) {
  (void)frame;
  ++ticks;
  // log(LOG_SERIAL, false, "ticks in handler: %d\n", ticks);
  pic_send_EOI(IRQ_PIT);
#if GUI_MODE
  if (ticks % 1000 == 0) {
    render_date_to_top_bar();
  }
#endif
}

static inline uint8_t get_port(uint8_t channel) {
  switch (channel) {
  case 0x0:
    return 0x40;
  case 0x1:
    return 0x41;
  case 0x2:
    return 0x42;
  default:
    return -1;
  }
}

void pit_set_counter(uint8_t channel, uint16_t count) {
  if (!(channel < 3)) {
    return;
  }
  uint16_t port = get_port(channel);
  outb(port, count & 0xFF);
  outb(port, (count >> 8) & 0xFF);
}

void pit_mdelay(uint64_t ms) {
  uint8_t val = ((0) | ((0x0) << 1) | ((0x3) << 4) | ((0x0) << 6));
  outb(I8253_CONTROL_REG, val);
  while (ms > 0) {
    pit_set_counter(0x2, INTERNAL_FREQ / 1000);
    while ((inb(0x61) & (1 << 5)) == 0) {
      __asm volatile("pause" ::: "memory");
    }
    ms--;
  }
}