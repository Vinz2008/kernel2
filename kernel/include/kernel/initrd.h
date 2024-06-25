#include <kernel/vfs.h>
#include <types.h>

#define FILE_TAR_TYPE '0'
#define HARD_LINK_TAR_TYPE '1'
#define SYM_LINK_TAR_TYPE '2'
#define CHAR_DEVICE_TAR_TYPE '3'
#define BLOCK_DEVICE_TAR_TYPE '4'
#define DIRECTORY_TAR_TYPE '5'
#define NAMED_PIPE_TAR_TYPE '6'

struct tar_header {
  char filename[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char typeflag[1];
};

typedef struct {
  uint32_t nfiles; // The number of files in the ramdisk.
} initrd_header_t;

typedef struct {
  uint8_t magic;   // Magic number, for error checking.
  int8_t name[64]; // Filename.
  uint32_t offset; // Offset in the initrd that the file starts.
  uint32_t length; // Length of the file.
} initrd_file_header_t;

typedef struct {
  struct tar_header** headers;
  size_t used;
  size_t size;
} initrd_list_header_t;

unsigned int getsize(const char* in);
unsigned int initrd_get_number_files(unsigned int address);
void initrd_list_filenames(unsigned int address);

void set_initrd_address(uint32_t addr);
uint32_t get_initrd_address();

fs_node_t* initialise_initrd(uint32_t location);
fs_node_t* get_initrd_root();
void set_dev_node_initrd();