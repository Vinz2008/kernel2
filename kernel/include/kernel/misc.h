#include <types.h>

#ifndef _MISC_HEADER_
#define _MISC_HEADER_

void reboot();

uint32_t align_to(uint32_t n, uint32_t align);

uint32_t divide_up(uint32_t n, uint32_t d);

#endif