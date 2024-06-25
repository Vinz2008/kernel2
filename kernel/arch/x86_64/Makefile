ARCHDIR=$(pwd)
INCLUDEDIR?=$(PREFIX)/include
PREFIX?=/usr/local

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@


install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps ../../include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: kernel.efi
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp kernel.efi $(DESTDIR)$(BOOTDIR)

include make.config

ARCH = x86_64
SRCS = $(wildcard *.c)
CFLAGS = -Wall --ansi -O2 -ffreestanding
LIBS=-nostdlib
TARGET = kernel.efi

USE_GCC=1 

include uefi/Makefile