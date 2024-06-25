CC=i686-medusaos-gcc
LD=i686-medusaos-ld
CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

CFLAGS:=$(CFLAGS) -Wall
CPPFLAGS:=$(CPPFLAGS)
LDFLAGS:=$(LDFLAGS)
CRTI_OBJ=crti.o


BIN=\
init.c \

SCRS = $(wildcard *.c)



all: $(SCRS:.c=)


%: %.c
	$(CC) -std=gnu11 -o $@ $^ $(CPPFLAGS) $(CFLAGS)

clean:
	rm -f $(SCRS:.c=)
	rm -rf out

install:  $(SCRS:.c=)