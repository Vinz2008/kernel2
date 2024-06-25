#include <kernel/vfs.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int printf(const char* restrict format, ...) {
  va_list parameters;
  va_start(parameters, format);
  return vprintf(format, parameters);
}

int vprintf(const char* restrict format, va_list parameters) {
  return vfprintf(VFS_FD_STDOUT, format, parameters);
}
