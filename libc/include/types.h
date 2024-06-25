#include <stdbool.h>
#include <stdint.h>

/*typedef unsigned char int8;
typedef int8 byte;
typedef unsigned short int16;
typedef int16 byte2;
// typedef unsigned long int32;
typedef int16 byte4;
typedef unsigned long long int64;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
// typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;
// typedef unsigned long uintptr_t;*/

typedef void (*void_function)(void);

#define low_16(address) (uint16_t)((address) & 0xFFFF)
#define high_16(address) (uint16_t)(((address) >> 16) & 0xFFFF)