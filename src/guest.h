#include "main.h"

#define MAX_CALL_DEPTH 512
//extern uint32_t return_stack[MAX_CALL_DEPTH];
extern int call_depth;

// virtual fs
typedef enum {
  FS_FILE,
  FS_DIR
} fs_type_t;

typedef struct fs_node {
  char name[32];
  fs_type_t type;

  struct fs_node *parent;
  struct fs_node *children[2048];
  int child_count;

  uint8_t *data;
  uint32_t size;
} fs_node_t;

extern fs_node_t *fs_root;

fs_node_t* fs_lookup(const char *path);

fs_node_t* fs_create_file(fs_node_t *parent, const char *name, uint8_t *data, uint32_t size);

#define MAX_FD 1024

typedef struct {
  fs_node_t *node;
  uint32_t offset;
  int used;
} fd_t;

extern fd_t fd_table[MAX_FD];

int alloc_fd(fs_node_t *node);

void fs_close(int fd);

uint32_t fs_read(int fd, uint8_t *buf, uint32_t count);

void fs_seek(int fd, uint32_t offset);

uint32_t fs_tell(int fd);

void env(uint32_t instr);

#define EI_NIDENT 16
#define PT_LOAD   1

/* ELF structures (minimal subset) */
typedef struct {
  unsigned char e_ident[EI_NIDENT];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
} Elf32_Ehdr;

typedef struct {
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
} Elf32_Phdr;

void syscall_write(uint32_t *x, uint8_t *memory);
void syscall_read(uint32_t *x, uint8_t *memory);
void syscall_open(uint32_t *x, uint8_t *memory);
void syscall_execve(uint32_t *x, uint8_t *memory, uint32_t *programcounter, uint32_t *pc_inst);
