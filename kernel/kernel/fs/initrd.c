#include <kernel/devfs.h>
#include <kernel/initrd.h>
#include <kernel/kmalloc.h>
#include <kernel/serial.h>
#include <kernel/vfs.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <types.h>

initrd_list_header_t list_headers;
bool initrd_readdir_call = false;
initrd_header_t* initrd_header; // The header.
fs_node_t* initrd_root;         // Our root directory node.
fs_node_t* initrd_dev; // We also add a directory node for /dev, so we can mount
                       // devfs later on.
fs_node_t* root_nodes; // List of file nodes.
int nroot_nodes;
initrd_file_header_t* file_headers; // Number of file nodes.

struct dirent dirent;

bool isInitrdInitialized = false;

static uint32_t initrd_adress;

int pos = -1;

extern struct dirent* devfs_readdir(struct fs_node* node, uint32_t index);
extern fs_node_t* devfs_finddir(struct fs_node* node, char* name);

void init_header_list(initrd_list_header_t* list, size_t initalSize) {
  list->headers = kmalloc(initalSize * sizeof(struct tar_header*));
  list->used = 0;
  list->size = initalSize;
}

void add_header_list(initrd_list_header_t* list, struct tar_header* header) {
  if (list->used == list->size) {
    list->size *= 2;
    list->headers =
        krealloc(list->headers, list->size * sizeof(struct tar_header*));
  }
  list->headers[list->used++] = header;
}

void empty_header_list(initrd_list_header_t* list) {
  kfree(list->headers);
  list->headers = NULL;
  list->used = list->size = 0;
}

void set_initrd_address(uint32_t addr) { initrd_adress = addr; }

uint32_t get_initrd_address() { return initrd_adress; }

unsigned int getsize(const char* in) {
  unsigned int size = 0;
  unsigned int j;
  unsigned int count = 1;
  for (j = 11; j > 0; j--, count *= 8) {
    size += ((in[j - 1] - '0') * count);
  }
  return size;
}

void generate_list_files_initrd(unsigned int address) {
  init_header_list(&list_headers, 1);
  unsigned int i;
  for (i = 0;; i++) {
    struct tar_header* header = (struct tar_header*)address;
    if (header->filename[0] == '\0') {
      break;
    }
    write_serialf("filename %i : %s\n", i, header->filename);
    unsigned int size = getsize(header->size);
    add_header_list(&list_headers, header);
    address += ((size / 512) + 1) * 512;
    if (size % 512) {
      address += 512;
    }
  }
}

unsigned int initrd_get_number_files(unsigned int address) {
  unsigned int i;
  for (i = 0;; i++) {
    struct tar_header* header = (struct tar_header*)address;
    if (header->filename[0] == '\0') {
      break;
    }
    write_serialf("filename %i : %s\n", i, header->filename);
    unsigned int size = getsize(header->size);
    address += ((size / 512) + 1) * 512;
    if (size % 512) {
      address += 512;
    }
  }
  return i;
}

void initrd_list_filenames(unsigned int address) {
  unsigned int i = 0;
  while (i <= initrd_get_number_files(address)) {
    struct tar_header* header = (struct tar_header*)address;
    if (header->filename[0] == '\0') {
      break;
    }
    printf("\nfilename [%i] : %s\n", i, header->filename);
    unsigned int size = getsize(header->size);
    address += ((size / 512) + 1) * 512;
    if (size % 512) {
      address += 512;
    }
    i++;
  }
}

static uint32_t initrd_read(fs_node_t* node, uint32_t offset, uint32_t size,
                            uint8_t* buffer) {
  struct tar_header* header = list_headers.headers[node->inode];
  /*if (offset > header.length)
          return 0;
  if (offset + size > header.length)
          size = header.length - offset;*/
  char* content = ((char*)header) + 512 + offset;
  memcpy(buffer, content, size);
  buffer[size] = '\0';
  return size;
}

static struct dirent* initrd_readdir(fs_node_t* dir, uint32_t index) {
  /*if (dir == initrd_root && index == nroot_nodes) {
          strcpy(dirent.name, "dev");
          dirent.name[3] = 0;
          dirent.ino = 0;
          return &dirent;
  }*/
  initrd_readdir_call = true;
  if (index >= nroot_nodes) {
    return NULL;
  }
  uint32_t dir_depth = vfs_get_depth(dir->name);
  for (size_t i = 0; i < nroot_nodes; i++) {
    // log(LOG_SERIAL, false, "TEST filename impl : %s\n", root_nodes[i].name);
    uint32_t child_depth = vfs_get_depth(root_nodes[i].name);
    // log(LOG_SERIAL, false, "dir_depth : %d, child_depth : %d\n ", dir_depth,
    // child_depth);
  }
  // log(LOG_SERIAL, false, "\n");

  return &root_nodes[index];

  // uint32_t dir_depth = vfs_get_depth(dir->name);
  // uint32_t n_child = 1;
  log(LOG_SERIAL, false, "nroot_nodes : %d\n", nroot_nodes);
  for (size_t i = 0; i < nroot_nodes; i++) {
    /*uint32_t child_depth = vfs_get_depth(root_nodes[i].name);
    if (strstr(root_nodes[i].name, dir->name) == NULL){
        log(LOG_SERIAL, false, "strstr(%s, %s)\n", root_nodes[i].name,
    dir->name);
    }
    if(strstr(root_nodes[i].name, dir->name) != NULL || dir ==
    get_initrd_root()) {
     // A file will have the same depth
     if(root_nodes[i].node_type == FT_File && child_depth == dir_depth){
        ++n_child;
     }
     // A sub-directory's name contains one more '/' so it will have +1 depth
    compare to the directory else if(root_nodes[i].node_type == FT_Directory &&
    child_depth == dir_depth + 1){
        ++n_child;
     }
    }
    log(LOG_SERIAL, false, "n_child : %d, index : %d\n ", n_child, index);
    if(n_child == index){
    log(LOG_SERIAL, false, "returned node pointer %p\n", &root_nodes[i]);
    return &root_nodes[i];
    }
    if (strcmp(dir->name, root_nodes[i].name) == 0){
        return &root_nodes[i];
    }*/
  }
  return NULL;
}

static fs_node_t* initrd_finddir(fs_node_t* node, char* name) {
  if (node == initrd_root && !strcmp(name, "dev")) {
    return initrd_dev;
  }

  int i;
  for (i = 0; i < nroot_nodes; i++) {
    if (!strcmp(name, root_nodes[i].name)) {
      return &root_nodes[i];
    }
  }
  return NULL;
}

int initrd_get_file_size(fs_node_t* node) {
  for (int i = 0; i < nroot_nodes; i++) {
    if (strcmp(node->name, root_nodes[i].name) == 0) {
      return getsize(list_headers.headers[i]->size);
    }
  }
  return -1;
}

fs_node_t* initialise_initrd(uint32_t location) {
  // initrd_header = (initrd_header_t*) location;
  // file_headers = (initrd_file_header_t*) (location +
  // sizeof(initrd_header_t));
  generate_list_files_initrd(location);
  initrd_header->nfiles = initrd_get_number_files(location);
  log(LOG_SERIAL, false, "nb_files: %d\n", initrd_header->nfiles);
  initrd_root = (fs_node_t*)kmalloc(sizeof(fs_node_t));
  strcpy(initrd_root->name, "/");
  initrd_root->mask = initrd_root->uid = initrd_root->gid = initrd_root->inode =
      initrd_root->length = 0;
  initrd_root->flags = FS_DIRECTORY;
  initrd_root->node_type = FT_Directory;
  initrd_root->read = &initrd_read;
  ;
  initrd_root->write = 0;
  initrd_root->open = 0;
  initrd_root->close = 0;
  initrd_root->readdir = &initrd_readdir;
  initrd_root->finddir = &initrd_finddir;
  initrd_root->get_file_size = &initrd_get_file_size;
  /*initrd_dev = (fs_node_t*)kmalloc(sizeof(fs_node_t));
  strcpy(initrd_dev->name, "dev/");
  initrd_dev->mask = initrd_dev->uid = initrd_dev->gid = initrd_dev->inode =
  initrd_dev->length = 0; initrd_dev->flags = FS_DIRECTORY;
  initrd_dev->node_type = FT_Directory;
  initrd_dev->read = 0;
  initrd_dev->write = 0;
  initrd_dev->open = 0;
  initrd_dev->close = 0;
  initrd_dev->readdir = &devfs_readdir;
  initrd_dev->finddir = &devfs_finddir;
  initrd_dev->mount_point = 0;
  initrd_dev->impl = 0;*/
  root_nodes =
      (fs_node_t*)kmalloc(sizeof(fs_node_t) * (initrd_header->nfiles + 1));
  nroot_nodes = initrd_header->nfiles;
  int i;
  for (i = 0; i < initrd_header->nfiles; i++) {
    // file_headers[i].offset += location;
    strcpy(root_nodes[i].name, strdup(list_headers.headers[i]->filename));
    log(LOG_SERIAL, false, "root_nodes[i].name : %s\n",
        list_headers.headers[i]->filename);
    root_nodes[i].mask = root_nodes[i].uid = root_nodes[i].gid = 0;
    root_nodes[i].length = getsize(list_headers.headers[i]->size);
    root_nodes[i].inode = i;
    root_nodes[i].flags = FS_FILE;
    root_nodes[i].read = &initrd_read;
    root_nodes[i].write = 0;
    root_nodes[i].readdir = &initrd_readdir;
    root_nodes[i].finddir = 0;
    root_nodes[i].open = 0;
    root_nodes[i].close = 0;
    root_nodes[i].impl = 0;
    root_nodes[i].get_file_size = &initrd_get_file_size;
    switch (list_headers.headers[i]->typeflag[0]) {
    case FILE_TAR_TYPE:
      root_nodes[i].node_type = FT_File;
      break;
    case DIRECTORY_TAR_TYPE:
      root_nodes[i].node_type = FT_Directory;
      break;
    }
  }
  pos = i;
  isInitrdInitialized = true;
  log(LOG_SERIAL, false, "initrd initialized\n");
  // vfs_mount(initrd_root, "/");
  return initrd_root;
}

void set_dev_node_initrd() {
  fs_node_t* dev = get_root_devfs();
  root_nodes[pos] = *dev;
  nroot_nodes++;
}

fs_node_t* get_initrd_root() { return initrd_root; }