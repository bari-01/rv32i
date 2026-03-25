#include "guest.h"

//uint32_t return_stack[MAX_CALL_DEPTH];
int call_depth = 0;

fs_node_t *fs_root;

fs_node_t* fs_lookup(const char *path) {
  if (path[0] != '/') return NULL;
  fs_node_t *cur = fs_root;
  char temp[256];
  strcpy(temp, path);
  char *token = strtok(temp, "/");
  while (token) {
    int found = 0;
    for (int i = 0; i < cur->child_count; i++) {
      if (strcmp(cur->children[i]->name, token) == 0) {
        cur = cur->children[i];
        found = 1;
        break;
      }
    }
    if (!found) return NULL;
    token = strtok(NULL, "/");
  }
  return cur;
}

fs_node_t* fs_create_file(fs_node_t *parent, const char *name, uint8_t *data, uint32_t size) {
  fs_node_t *n = (fs_node_t*)malloc(sizeof(fs_node_t));
  memset(n, 0, sizeof(*n));

  strcpy(n->name, name);
  n->type = FS_FILE;
  n->data = data;
  n->size = size;
  n->parent = parent;

  parent->children[parent->child_count++] = n;
  return n;
}

fd_t fd_table[MAX_FD];

int alloc_fd(fs_node_t *node) {
  for (int i = 0; i < MAX_FD; i++) {
    if (!fd_table[i].used) {
      fd_table[i].used = 1;
      fd_table[i].node = node;
      fd_table[i].offset = 0;
      return i;
    }
  }
  return -1; // no free fd
}

void fs_close(int fd) {
  if (fd < 0 || fd >= MAX_FD) return;
  fd_table[fd].used = 0;
}

uint32_t fs_read(int fd, uint8_t *buf, uint32_t count) {
  if (fd < 0 || fd >= MAX_FD || !fd_table[fd].used)
    return 0;

  fd_t *f = &fd_table[fd];
  fs_node_t *node = f->node;

  uint32_t remaining = node->size - f->offset;
  if (count > remaining)
    count = remaining;

  memcpy(buf, node->data + f->offset, count);
  f->offset += count;

  return count;
}

void fs_seek(int fd, uint32_t offset) {
  if (fd < 0 || fd >= MAX_FD || !fd_table[fd].used)
    return;

  if (offset > fd_table[fd].node->size)
    offset = fd_table[fd].node->size;

  fd_table[fd].offset = offset;
}

uint32_t fs_tell(int fd) {
  return fd_table[fd].offset;
}

uint32_t setup_stack(char **argv, uint8_t *memory, uint32_t mem_size, uint32_t *x) {
    uint32_t sp = mem_size;

    uint32_t argv_ptrs[16];
    int argc = 0;

    // copy strings
    for (; argv[argc]; argc++) {
        size_t len = strlen(argv[argc]) + 1;
        sp -= len;
        memcpy(&memory[sp], argv[argc], len);
        argv_ptrs[argc] = sp;
    }

    // align
    sp &= ~0xF;

    // NULL
    sp -= 4;
    *(uint32_t*)&memory[sp] = 0;

    // argv[]
    for (int i = argc - 1; i >= 0; i--) {
        sp -= 4;
        *(uint32_t*)&memory[sp] = argv_ptrs[i];
    }

    uint32_t argv_addr = sp;

    // argc
    sp -= 4;
    *(uint32_t*)&memory[sp] = argc;

    // registers
    x[2]  = sp;        // sp
    x[10] = argc;      // a0
    x[11] = argv_addr; // a1

    return sp;
}

void load_elf(const char *path, uint32_t base_addr, uint8_t *memory, uint32_t *programcounter, uint32_t *pc_inst) {
  fs_node_t *node = fs_lookup(path);
  if (!node || node->type != FS_FILE) {
    printf("ELF not found: %s\n", path);
    exit(1);
  }

  int fd = alloc_fd(node);
  if (fd < 0) {
    printf("No free file descriptors\n");
    exit(1);
  }

  Elf32_Ehdr eh;
  fs_read(fd, (uint8_t*)&eh, sizeof(eh));

  // check ELF magic
  if (eh.e_ident[0] != 0x7f ||
      eh.e_ident[1] != 'E' ||
      eh.e_ident[2] != 'L' ||
      eh.e_ident[3] != 'F') {
    printf("Invalid ELF\n");
    exit(1);
  }

  // go to program headers
  fs_seek(fd, eh.e_phoff);

  for (int i = 0; i < eh.e_phnum; i++) {
    Elf32_Phdr ph;
    fs_read(fd, (uint8_t*)&ph, sizeof(ph));

    if (ph.p_type != PT_LOAD)
      continue;

    // load segment
    fs_seek(fd, ph.p_offset);
    fs_read(fd,
            &memory[base_addr + ph.p_vaddr],
            ph.p_filesz);

    // zero BSS
    if (ph.p_memsz > ph.p_filesz) {
      memset(&memory[base_addr + ph.p_vaddr + ph.p_filesz],
             0,
             ph.p_memsz - ph.p_filesz);
    }

    // restore ph table position
    fs_seek(fd, eh.e_phoff + (i + 1) * sizeof(Elf32_Phdr));
  }

  fs_close(fd);

  // push return address (stack version!)
  //return_stack[call_depth++] = *programcounter;

  *programcounter = base_addr + eh.e_entry;
  *pc_inst = *programcounter;

  printf("ELF loaded: %s\n", path);
}

void syscall_write(uint32_t *x, uint8_t *memory) {
  uint32_t fd   = x[10];
  uint32_t addr = x[11];
  uint32_t len  = x[12];

  uint8_t *buf = &memory[addr];

  if (fd == 1 || fd == 2) {
    for (uint32_t i = 0; i < len; i++)
      putchar(buf[i]);
    fflush(stdout);
    x[10] = len;
  } else if (fd >= 0 && fd < MAX_FD && fd_table[fd].used) {
    fd_t *f = &fd_table[fd];
    fs_node_t *node = f->node;

    // grow file if needed
    if (f->offset + len > node->size) {
      node->data = (uint8_t*)realloc(node->data, f->offset + len);
      node->size = f->offset + len;
    }

    memcpy(node->data + f->offset, buf, len);
    f->offset += len;

    x[10] = len;
  } else {
    x[10] = -1;
  }
}

void syscall_read(uint32_t *x, uint8_t *memory) {
  uint32_t fd   = x[10];
  uint32_t addr = x[11];
  uint32_t len  = x[12];

  uint8_t *buf = &memory[addr];

  if (fd == 0) {
    uint32_t i;
    for (i = 0; i < len; i++) {
      int c = getchar();
      if (c == EOF) break;
      buf[i] = (uint8_t)c;
    }
    x[10] = i;
  } else if (fd >= 0 && fd < MAX_FD && fd_table[fd].used) {
    x[10] = fs_read(fd, buf, len);
  } else {
    x[10] = -1;
  }
}

void syscall_open(uint32_t *x, uint8_t *memory) {
  char *path = (char*)&memory[x[10]];
  fs_node_t *node = fs_lookup(path);

  if (!node) {
    x[10] = -1;
  } else {
    x[10] = alloc_fd(node);
  }
}

uint32_t heap = 0x10000;

void syscall_execve(uint32_t *x, uint8_t *memory, uint32_t *programcounter, uint32_t *pc_inst) {
  char *path = (char*)&memory[x[10]];
  char *argv[16];
  int argc = 0;

  uint32_t argv_addr = x[11];

  while (1) {
    uint32_t ptr = *(uint32_t*)&memory[argv_addr];
    if (ptr == 0) break;

    argv[argc++] = (char*)&memory[ptr];
    argv_addr += 4;
  }

  argv[argc] = NULL;
  char *envp = (char*)&memory[x[12]];
  memset(memory, 0, MEM_SIZE);
  heap = 0x10000; // reset heap
  load_elf(path, 0, memory, programcounter, pc_inst); 
  setup_stack(argv, memory, MEM_SIZE, x);
}


void syscall_brk(uint32_t *x, uint8_t *memory) {
  uint32_t new_brk = x[10];

  if (new_brk == 0) {
    x[10] = heap; // return current break
    return;
  }

  if (new_brk >= x[2]) {
    x[10] = -1;
    return;
  }

  heap = new_brk;
  x[10] = heap;
}
