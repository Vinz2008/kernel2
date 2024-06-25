#include <stdio.h>

char* strcpy(char* dst, const char* src) {
  if (dst == NULL) {
    return NULL;
  }
  char* s = dst;
  if (src && dst && src != dst) {
    while (*src) {
      *(dst++) = *(src++);
    }
    *dst = 0;
  }
  return s;
}

char* strncpy(char* dst, const char* src, size_t num) {
  if (dst == NULL) {
    return NULL;
  }
  char* s = dst;
  if (src && dst && src != dst) {
    while (*src && num--) {
      *(dst++) = *(src++);
    }
    *dst = 0;
  }
  return s;
}