#include <kernel/speaker.h>
#include <types.h>
#include <stdio.h>
#include <kernel/io.h>
#include <kernel/pit.h>
#include <kernel/x86.h>
 
void pit_phase_c2(uint32_t hz) {
  uint32_t divisor = 1193180 / hz;
  outb(0x43, 0xB6);
  outb(0x42, (uint8_t)divisor & 0xFF);
  outb(0x42, (uint8_t)(divisor >> 8) & 0xFF);
}

void pcspkr_tone_on(uint32_t hz) {
  pit_phase_c2(hz);
  uint8_t tmp = inb(0x61);
  if (tmp != (tmp | 3))
    outb(0x61, tmp | 3);
}

void pcspkr_tone_off() {
  uint8_t tmp = inb(0x61) & 0xFC;
  outb(0x61, tmp);
  pit_phase_c2(1);
}
 
 //make it shutup
static void nosound() {
 	uint8_t tmp = inb(0x61) & 0xFC;
 	outb(0x61, tmp);
}
 
 //Make a beep
void pcspkr_beep(uint32_t mst, uint16_t hz) {
	pcspkr_tone_on(hz);
	log(LOG_SERIAL, false, "before sleep\n");
 	//pit_sleep(mst);
	pit_mdelay(mst);
	log(LOG_SERIAL, false, "after sleep\n");
	pcspkr_tone_off();
}

void init_speaker(){
	outb(0x61, inb(0x61) | 0x1);
}