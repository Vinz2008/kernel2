#include <kernel/devfs.h>
#include <kernel/device.h>
#include <kernel/kmalloc.h>
#include <kernel/tty_framebuffer.h>
#include <stdio.h>
#include <string.h>

void null_open(fs_node_t* node) { (void)node; }

Device* create_device(char* name, read_type_t read, write_type_t write,
                      open_type_t open, close_type_t close) {
  if (strlen(name) > 16) {
    return NULL;
  }
  Device* device = kmalloc(sizeof(Device));
  strcpy(device->name, name);
  device->read = read;
  device->write = write;
  device->open = open;
  device->close = close;
  return device;
}

static uint32_t stdin_read(fs_node_t* node, uint32_t offset, uint32_t size,
                           uint8_t* buffer) {
  char* written = get_line_cli();
  memcpy(buffer, written, size);
  return size;
}

static uint32_t stdout_write(fs_node_t* node, uint32_t offset, uint32_t size,
                             uint8_t* buffer) {
  char* str = kmalloc(sizeof(char) * size);
  memcpy(str, buffer, size);
  return (uint32_t)printf("%s", str);
}

void register_devices_necessary() {
  // Device* stdin_dev = (Device){.name="stdin", .close=0, .read=0, .write=0,};
  Device* stdin_dev = create_device("stdin", stdin_read, 0, 0, 0);
  // memset((uint8_t*)stdin_dev, 0, sizeof(Device));
  /*Device stdin_dev;
  memset((uint8_t*)&stdin_dev, 0, sizeof(Device));
  strcpy(stdin_dev.name, "stdin");*/
  devfs_register_device(stdin_dev);
  Device stdout_dev = (Device){
      .name = "stdout",
      .close = 0,
      .read = 0,
      .write = stdout_write,
  };
  devfs_register_device(&stdout_dev);
  Device tty_dev = (Device){
      .name = "tty",
      .close = 0,
      .read = 0,
      .write = 0,
  };
  devfs_register_device(&tty_dev);
  Device null_dev = (Device){
      .name = "null",
      .close = 0,
      .read = 0,
      .write = 0,
  };
  /*Device null_dev;
  memset((uint8_t*)&null_dev, 0, sizeof(Device));
  strcpy(null_dev.name, "null");
  null_dev.open = null_open;*/
  devfs_register_device(&null_dev);
}