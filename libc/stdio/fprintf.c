#include <kernel/kmalloc.h>
#include <kernel/serial.h>
#include <kernel/vfs.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool fprint(file_descriptor_t file, const char* data, size_t length) {
  const unsigned char* bytes = (const unsigned char*)data;
  for (size_t i = 0; i < length; i++) {
    if (fputchar(bytes[i], file) == EOF) {
      return false;
    }
  }
  return true;
}

int fprintf(file_descriptor_t file, const char* format, ...) {
  va_list parameters;
  va_start(parameters, format);
  return vfprintf(file, format, parameters);
}

int vfprintf(file_descriptor_t file, const char* format, va_list parameters) {
  int written = 0;
  while (*format != '\0') {
    size_t maxrem = INT_MAX - written;

    if (format[0] != '%' || format[1] == '%') {
      if (format[0] == '%') {
        format++;
      }
      size_t amount = 1;
      while (format[amount] && format[amount] != '%') {
        amount++;
      }
      if (maxrem < amount) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!fprint(file, format, amount)) {
        return -1;
      }
      format += amount;
      written += amount;
      continue;
    }

    const char* format_begun_at = format++;

    if (*format == 'c') {
      format++;
      char c = (char)va_arg(parameters, int /* char promotes to int */);
      if (!maxrem) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!fprint(file, &c, sizeof(c))) {
        return -1;
      }
      written++;
    } else if (*format == 's') {
      format++;
      const char* str = va_arg(parameters, const char*);
      size_t len = strlen(str);
      if (maxrem < len) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }

      if (!fprint(file, str, len)) {
        return -1;
      }
      written += len;

    } else if (*format == 'd') {
      format++;
      char i2[10];
      memset(i2, 0, sizeof(i2));
      int i = va_arg(parameters, int);
      int_to_ascii(i, i2);
      int length = strlen(i2);
      if (!maxrem) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!fprint(file, i2, sizeof(i2) - (10 - length))) {
        return -1;
      }
      written++;
    } else if (*format == 'i') {
      format++;
      char i2[10];
      memset(i2, 0, sizeof(i2));
      int i = va_arg(parameters, int);
      int_to_ascii(i, i2);
      int length = strlen(i2);
      if (!maxrem) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!fprint(file, i2, sizeof(i2) - (10 - length))) {
        return -1;
      }
      written++;
    } else if (*format == 'x') {
      format++;
      char i2[10];
      memset(i2, 0, 10);
      // char* i2;
      int i = va_arg(parameters, int);
      int_to_ascii_base(i, i2, 16);
      int length = strlen(i2);
      if (!maxrem) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!fprint(file, i2, sizeof(i2) - (10 - length))) {
        return -1;
      }
      written++;
    } else if (*format == 'p') {
      format++;
      char i2[10];
      memset(i2, 0, 10);
      void* i = va_arg(parameters, void*);
      hex_to_ascii((int)i, i2);
      int length = strlen(i2);
      if (!maxrem) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      fprint(file, i2, sizeof(i2) - (10 - length));
      written++;
    } else {
      format = format_begun_at;
      size_t len = strlen(format);
      if (maxrem < len) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!fprint(file, format, len)) {
        return -1;
      }
      written += len;
      format += len;
    }
  }

  va_end(parameters);
  return written;
}
