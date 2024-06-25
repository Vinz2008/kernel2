DEFAULT_HOST!=../default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc

ARCHDIR=arch/$(HOSTARCH)

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
kernel/kernel.o \
kernel/data_structures/list.o \
kernel/data_structures/pair.o \
kernel/data_structures/hashtable.o \
kernel/data_structures/linked_list.o \
kernel/data_structures/stack.o \
kernel/fs/vfs.o \
kernel/fs/devfs.o \
kernel/fs/pipe.o \
kernel/fs/initrd.o \
kernel/fs/ext2.o \
kernel/mm/pmm.o \
kernel/mm/kmalloc.o \
kernel/mm/paging.o \
kernel/tools/ansi.o \
kernel/tools/base64.o \
kernel/proc/syscall.o \
kernel/proc/process.o \
kernel/devices/device.o \
kernel/chip8/emulator.o \
kernel/fb.o \
kernel/font.o \
kernel/multiboot2.o \


OBJS=\
$(KERNEL_OBJS) \
# $(ARCHDIR)/crti.o \
# $(ARCHDIR)/crtbegin.o \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

LINK_LIST=\
$(LDFLAGS) \
$(KERNEL_OBJS) \
$(LIBS) \
# $(ARCHDIR)/crtend.o \
# $(ARCHDIR)/crtn.o \
# $(ARCHDIR)/crti.o \
# $(ARCHDIR)/crtbegin.o \

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S

all: kernel.kernel

kernel.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot2 kernel.kernel

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f kernel.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: kernel.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp kernel.kernel $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
