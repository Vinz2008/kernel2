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


LDFLAGS:=$(LDFLAGS) -Tmod.ld


BIN=\
test_c.mod \
test_int.mod \

.PHONY: all clean install
.SUFFIXES: .o .mod .c .S

all: $(BIN)

LINK_LIST=\
$(LDFLAGS) \
$(LIBS) \

%.mod: %.c
	$(CC) -std=gnu11 -o $@ $^ $(CPPFLAGS) $(CFLAGS)


test_module.bin: $(OBJS)
# mv test_module.o test_module.bin
	echo $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@
# objcopy -O binary -j .text $(OBJS) $@


.S.o: 
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)
# objcopy -O binary -j .text test.o binfile

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f $(BIN)
	rm -f *.mod
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d


install: $(BIN)

#-include $(OBJS:.o=.d)