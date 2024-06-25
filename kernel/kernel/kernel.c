#include <assert.h>
#include <kernel/config.h>
#include <kernel/cpuid.h>
#include <kernel/descriptors_tables.h>
#include <kernel/devfs.h>
#include <kernel/device.h>
#include <kernel/fb.h>
#include <kernel/fpu.h>
#include <kernel/graphics.h>
#include <kernel/gui.h>
#include <kernel/hashtable.h>
#include <kernel/initrd.h>
#include <kernel/irq_handlers.h>
#include <kernel/keyboard.h>
#include <kernel/kmalloc.h>
#include <kernel/mouse.h>
#include <kernel/multiboot2_internal.h>
#include <kernel/paging.h>
#include <kernel/pci.h>
#include <kernel/pic.h>
#include <kernel/pit.h>
#include <kernel/pmm.h>
#include <kernel/proc.h>
#include <kernel/ps2.h>
#include <kernel/rtc.h>
#include <kernel/serial.h>
#include <kernel/syscall.h>
#include <kernel/tty_framebuffer.h>
#include <kernel/vfs.h>
#include <math.h>
#include <multiboot2.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define _KERNEL_
#define CHECK_FLAG(flags, bit) ((flags) & (1 << (bit)))

extern uint32_t KERNEL_BEGIN_PHYS;
extern uint32_t KERNEL_END_PHYS;
extern uint32_t KERNEL_SIZE;

extern uint32_t KERNEL_END;
extern uint32_t placement_address;

extern int32_t syscall3(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);
extern uint32_t syscall(uint32_t id);
extern int syscall_c(int function, int param1, int param2, int param3);

extern void sse_init();
// multiboot_info_t *mb_info;
// multiboot_module_t *mod;
struct multiboot_tag* tag;

void kernel_main(uint32_t addr, uint32_t magic) {
  init_serial();
  write_serial("LOG START\n");
  fpu_init();
  log(LOG_SERIAL, false, "FPU initialized\n");
  init_pmm(addr);
  init_paging(addr);
  log(LOG_SERIAL, false, "Paging enabled\n");
  vfs_init();
  log(LOG_SERIAL, false, "MedusaOS\n");
  log(LOG_SERIAL, false, "kernel is %d KiB large\n",
      ((uint32_t)&KERNEL_SIZE) >> 0);
  struct multiboot_tag* tag;
  unsigned size;
  if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
    log(LOG_SERIAL, false, "Invalid magic number %x\n", magic);
  }
  if (addr & 7) {
    log(LOG_SERIAL, false, "Unaligned mbi: %p\n", addr);
    return;
  }
  size = *(unsigned*)addr;
  log(LOG_SERIAL, false, "Annonced mbi size: %x\n", size);
  // TODO : move this in a handle_multiboot function ?
  for (tag = (struct multiboot_tag*)(addr + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag*)((multiboot_uint8_t*)tag +
                                     ((tag->size + 7) & ~7))) {
    log(LOG_SERIAL, false, "Tag %x, Size %x\n", tag->type, tag->size);
    switch (tag->type) {
    case MULTIBOOT_TAG_TYPE_CMDLINE:
      log(LOG_SERIAL, false, "Command line = %s\n",
          ((struct multiboot_tag_string*)tag)->string);
      break;
    case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
      log(LOG_SERIAL, false, "Boot loader name = %s\n",
          ((struct multiboot_tag_string*)tag)->string);
      break;
    case MULTIBOOT_TAG_TYPE_MODULE:
      struct multiboot_tag_module* mod = (struct multiboot_tag_module*)tag;
      log(LOG_SERIAL, false, "Module at %x-%x. Command line %s\n",
          mod->mod_start, mod->mod_end, mod->cmdline);
      uint32_t size = mod->mod_end - mod->mod_start;
      uint8_t* data = (uint8_t*)kmalloc(size);
      memcpy(data, (void*)mod->mod_start, size);
      if (strcmp(mod->cmdline, "initrd") == 0) {
        set_initrd_address((uint32_t)data);
        fs_root = initialise_initrd((uint32_t)data);
        vfs_mount("/", fs_root);
        struct dirent* dir = NULL;
        int i = 0;
        while ((dir = readdir_fs(fs_root, i)) != NULL) {
          log(LOG_SERIAL, false, "found file %s\n", dir->name);
          fs_node_t* node = finddir_fs(fs_root, dir->name);
          if (node->node_type != FT_Directory) {
            uint8_t buffer[node->length];
            read_fs(node, 0, node->length, buffer);
            log(LOG_SERIAL, false, "file %s content %s\n", node->name, buffer);
          }
          i++;
        }
        devfs_initialize();
        set_dev_node_initrd();
        register_devices_necessary();
      } else {
        // is not an initrd module (is an elf ?)
      }
      break;
    case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
      log(LOG_SERIAL, false, "mem_lower = %dKB, mem_upper = %dKB\n",
          ((struct multiboot_tag_basic_meminfo*)tag)->mem_lower,
          ((struct multiboot_tag_basic_meminfo*)tag)->mem_upper);
      break;
    case MULTIBOOT_TAG_TYPE_BOOTDEV:
      log(LOG_SERIAL, false, "Boot device %x,%d,%d\n",
          ((struct multiboot_tag_bootdev*)tag)->biosdev,
          ((struct multiboot_tag_bootdev*)tag)->slice,
          ((struct multiboot_tag_bootdev*)tag)->part);
      break;
    case MULTIBOOT_TAG_TYPE_MMAP: {
      multiboot_memory_map_t* mmap;

      log(LOG_SERIAL, false, "mmap\n");

      for (mmap = ((struct multiboot_tag_mmap*)tag)->entries;
           (multiboot_uint8_t*)mmap < (multiboot_uint8_t*)tag + tag->size;
           mmap = (multiboot_memory_map_t*)((unsigned long)mmap +
                                            ((struct multiboot_tag_mmap*)tag)
                                                ->entry_size)) {
        log(LOG_SERIAL, false,
            " base_addr = %x%x,"
            " length = %x%x, type = %x\n",
            (unsigned)(mmap->addr >> 32), (unsigned)(mmap->addr & 0xffffffff),
            (unsigned)(mmap->len >> 32), (unsigned)(mmap->len & 0xffffffff),
            (unsigned)mmap->type);
      }
    } break;
    case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
      // multiboot_uint32_t color;
      //  unsigned i;

      struct multiboot_tag_framebuffer* tagfb =
          (struct multiboot_tag_framebuffer*)tag;
      // void* fb = (void*)(unsigned long)tagfb->common.framebuffer_addr;
      log(LOG_SERIAL, false, "framebuffer type : %d\n",
          tagfb->common.framebuffer_type);
      init_fb(tagfb);

      /*switch (tagfb->common.framebuffer_type)
        {
        case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED:
          {
            unsigned best_distance, distance;
            struct multiboot_color *palette;

            palette = tagfb->framebuffer_palette;

            color = 0;
            best_distance = 4*256*256;

            for (i = 0; i < tagfb->framebuffer_palette_num_colors; i++)
              {
                distance = (0xff - palette[i].blue)
                  * (0xff - palette[i].blue)
                  + palette[i].red * palette[i].red
                  + palette[i].green * palette[i].green;
                if (distance < best_distance)
                  {
                    color = i;
                    best_distance = distance;
                  }
              }
          }
          break;
                    case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
          color = ((1 << tagfb->framebuffer_blue_mask_size) - 1)
            << tagfb->framebuffer_blue_field_position;
          break;

        case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
          color = '\\' | 0x0100;
          break;

        default:
          color = 0xffffffff;
          break;
        }
                  for (i = 0; i < tagfb->common.framebuffer_width
             && i < tagfb->common.framebuffer_height; i++)
        {
          switch (tagfb->common.framebuffer_bpp)
            {
            case 8:
              {
                multiboot_uint8_t *pixel = fb
                  + tagfb->common.framebuffer_pitch * i + i;
                *pixel = color;
              }
              break;
            case 15:
            case 16:
              {
                multiboot_uint16_t *pixel
                  = fb + tagfb->common.framebuffer_pitch * i + 2 * i;
                *pixel = color;
              }
              break;
            case 24:
              {
                multiboot_uint32_t *pixel
                  = fb + tagfb->common.framebuffer_pitch * i + 3 * i;
                *pixel = (color & 0xffffff) | (*pixel & 0xff000000);
              }
              break;

            case 32:
              {
                multiboot_uint32_t *pixel
                  = fb + tagfb->common.framebuffer_pitch * i + 4 * i;
                *pixel = color;
              }
              break;
            }
        }*/
      break;
    }
    }
  }
  tag =
      (struct multiboot_tag*)((multiboot_uint8_t*)tag + ((tag->size + 7) & ~7));
  log(LOG_SERIAL, false, "Total mbi size 0x%x\n", (unsigned)tag - addr);

  uint32_t model_name[12];
  if (get_model_name(model_name) == 0) {
    log(LOG_SERIAL, false, "model name : %s\n", model_name);
  }

#if GUI_MODE
  init_gui();
  // draw_line_vertical(100, 10, 250, 0xFFFFFF);
  fb_t fb = fb_get_info();
  window_t* bg_window = get_background_window();
  window_t* top_bar_window = get_top_bar_window();
  draw_string(bg_window->fb, "MEDUSAOS", 100, 250, 0xFFFFFF);
  draw_string_size(bg_window->fb, 2, "MEDUSAOS", 100, 250, 0xFFFFFF);
  draw_line_horizontal(bg_window->fb, 125, 360, 330, 0xFFFFFF);
  render_window(bg_window);
  render_window(top_bar_window);

  window_t* win = open_window("test window", 300, 125, 0);
  // draw_string(fb, "Lorem Ipsum", 45, 55, 0x00AA1100);
  // draw_border(fb, 40, 50, strlen("Lorem Ipsum")*8+10, 26, 0xFFFFFF);
  draw_string(win->fb, "Lorem Ipsum", 45, 55, 0x00AA1100);
  // draw_border(win->fb, 40, 50, strlen("Lorem Ipsum")*8+10, 26, 0x00);

  // draw_window(win);
  render_window(win);
  // render_window(win);
#else
  terminal_framebuffer_initialize();
#endif
  descriptor_tables_initialize();
#if GUI_MODE
#else
  log(LOG_ALL, true, "gdt initialized\n");
  log(LOG_ALL, true, "idt initialized\n");
#endif
  pit_init(SYSTEM_TICKS_PER_SEC);
  log(LOG_ALL, true, "i8253 (PIT) initialized @%i hz\n", SYSTEM_TICKS_PER_SEC);
  pic_init();
  log(LOG_ALL, true, "i8259 (PIC) initialized\n");
  irq_register_handler(0, sys_tick_handler);
  log(LOG_ALL, true, "IRQ handler set: sys_tick_handler\n");
  // init_ps2();
  // ps2_initialize();
  keyboard_install();
#if GUI_MODE
#else
  char timer_str[] = "System timer is ticking\n";
  puts(timer_str);
#endif
  x86_enable_int();
  sse_init();
  log(LOG_SERIAL, false, "SSE initialized\n");
  init_syscalls();
  log(LOG_ALL, true, "Syscalls enabled\n");
  /*if (mb_info->mods_count > 0){
          mod = (multiboot_module_t *) mb_info->mods_addr;
          uint32_t initrd_location = *((uint32_t*)mb_info->mods_addr);
          uint32_t initrd_end = *(uint32_t*)(mb_info->mods_addr+4);
          placement_address = initrd_end;
          write_serialf("module loaded\n");
          write_serialf("disk image: Module starts at %p and ends at
  %p\n",initrd_location, initrd_end);
          //write_serialf("number of files : %d\n",
  initrd_get_number_files(mod->mod_start));
  }
  write_serialf("modules %d\n", mb_info->mods_count);
  unsigned int* modules = (unsigned int*)mb_info->mods_addr;
  log(LOG_SERIAL, false, "available memory from bios :
  %d\n",mb_info->mem_lower);*/
  int* test = kmalloc(sizeof(int));
  *test = 2;
  log(LOG_SERIAL, false, "pointer returned : %p\n", test);
  int* test2 = kmalloc(sizeof(int));
  int* test3 = kmalloc(sizeof(int));
  *test2 = 4;
  *test3 = 6;
  log(LOG_SERIAL, false, "pointer returned : %p\n", test2);
  log(LOG_SERIAL, false, "pointer returned : %p\n", test3);
  log(LOG_SERIAL, false, "end of the kernel : %p\n", &KERNEL_END);
  kfree(test);
  kfree(test2);
  kfree(test3);

  int i = 1233;
#if GUI_MODE
#else
  printf("Welcome to kernel %i\n", i);
  printf("This kernel is made using the osdev wiki\n");
  printf("Press ESC to reboot\n");
  alert("error : %s\n", "string");
  fprintf(VFS_FD_STDOUT, "hello from fprintf stdout\n");
#endif
  fprintf(VFS_FD_SERIAL, "hello from fprintf serial\n");
  char* date = read_rtc_date();
  printf("%s\n", date);
  /*if (CHECK_FLAG (mb_info->flags, 2)){
printf ("cmdline = %s\n", (char *) mb_info->cmdline);
  }*/
  printf("\x1b[1;31m Hello World in red with ansi escape sequences \x1b[0m \n");
#if GUI_MODE
#else
  printf("> ");
#endif
  while (1) {};
}
