#include <string.h>

int strcmp(const char* s1, const char* s2) {
  while (*s1 != 0 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return (*s1 - *s2);
}
int strncmp(const char* s1, const char* s2, size_t n) {
  while (n-- > 0) {
    if (*s1 == 0 || *s2 != *s1) {
      return (*s1) - (*s2);
    }
    s1++;
    s2++;
  }
  return 0;
}