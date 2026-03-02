#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define F3_ADDI 0x0
#define F3_ANDI 0x7
#define F3_ORI 0x6
#define F3_XORI 0x4
#define F3_SLTI 0x2
#define F3_SLTIU 0x3
#define F3_SLLI 0x1
#define F3_SRLI_SRAI 0x5

#define F3_ADD_SUB 0x0
#define F3_AND 0x7
#define F3_OR 0x6
#define F3_XOR 0x4
#define F3_SLT 0x2
#define F3_SLTU 0x3
#define F3_SLL 0x1
#define F3_SRL_SRA 0x5

#define F3_BEQ 0x0
#define F3_BNE 0x1
#define F3_BLT 0x4
#define F3_BGE 0x5
#define F3_BLTU 0x6
#define F3_BGEU 0x7

#define F3_LB 0x0
#define F3_LH 0x1
#define F3_LW 0x2
#define F3_LBU 0x4
#define F3_LHU 0x5

#define F3_SB 0x0
#define F3_SH 0x1
#define F3_SW 0x2

//https://docs.riscv.org/reference/isa/unpriv/_images/svg-6e79131cf8a4d97ce98f508a368da764a7b11e95.svg
uint32_t x[32] = {0};
uint32_t programcounter = 0;
uint32_t pc_inst = 0;

uint8_t memory[1048576] = {0};

static inline uint32_t bits(uint32_t x, int hi, int lo) {
  return (x >> lo) & ((1u << (hi - lo + 1)) - 1);
}

static inline int32_t sext(uint32_t x, int bits) {
  return (int32_t)(x << (32 - bits)) >> (32 - bits);
}

static inline void illegal(uint32_t instr) {
  printf("Illegal instruction=%u at pc=%08x\n", instr, programcounter);
  exit(1);
}

void op_imm(uint32_t instr) {
  uint32_t rd     = bits(instr, 11, 7); //(instr >> 7)  & 0x1F;   // bits 11:7
  uint32_t funct3 = bits(instr, 14, 12); //(instr >> 12) & 0x07;   // bits 14:12
  uint32_t rs1    = bits(instr, 19, 15); //(instr >> 15) & 0x1F;   // bits 19:15
  int32_t imm_i = sext(bits(instr, 31, 20), 12); //(int32_t)instr >> 20;
  uint32_t shamt    = bits(instr, 24, 20); //(instr >> 20) & 0x1F;   // bits 24:20
  switch (funct3) {
    case F3_ADDI: { x[rd] = x[rs1] + imm_i; break; }
    case F3_ANDI: { x[rd] = x[rs1] & imm_i; break; }
    case F3_ORI: { x[rd] = x[rs1] | imm_i; break; }
    case F3_XORI: { x[rd] = x[rs1] ^ imm_i; break; }
    case F3_SLTI: { x[rd] = (int32_t)x[rs1] < imm_i ? 1 : 0; break; }
    case F3_SLTIU: { x[rd] = x[rs1] < (uint32_t)imm_i ? 1 : 0; break; }
    case F3_SLLI: { x[rd] = x[rs1] << shamt; break; }
    case F3_SRLI_SRAI: {
      uint32_t funct7 = (instr >> 25) & 0x7F;   // bits 31:25
      switch(funct7) {
        case 0b0000000: { x[rd] = x[rs1] >> shamt; break; } //SRLI
        case 0b0100000: { x[rd] = (int32_t)x[rs1] >> shamt; break; } //SRAI
        default: { illegal(instr); break; }
      }
      break;
    }
    default: { illegal(instr); break; }
  }
  programcounter += 4;
  x[0] = 0;
}

void op(uint32_t instr) {
  //https://docs.riscv.org/reference/isa/unpriv/_images/svg-6e79131cf8a4d97ce98f508a368da764a7b11e95.svg
  //uint32_t opcode =  instr        & 0x7F;   // bits 6:0
  uint32_t rd     = bits(instr, 11, 7); //(instr >> 7)  & 0x1F;   // bits 11:7
  uint32_t funct3 = bits(instr, 14, 12); //(instr >> 12) & 0x07;   // bits 14:12
  uint32_t rs1    = bits(instr, 19, 15); //(instr >> 15) & 0x1F;   // bits 19:15
  uint32_t rs2    = bits(instr, 24, 20); //(instr >> 20) & 0x1F;   // bits 24:20
  switch (funct3) {
    case F3_ADD_SUB: {
      uint32_t funct7 = (instr >> 25) & 0x7F;   // bits 31:25
      switch(funct7) {
        case 0b0000000: { x[rd] = x[rs1] + x[rs2]; break; } //SRLI
        case 0b0100000: { x[rd] = x[rs1] - x[rs2]; break; } //SRLI
        default: { illegal(instr); break; }
      }
      break;
    }
    case F3_AND: { x[rd] = x[rs1] & x[rs2]; break; }
    case F3_OR: { x[rd] = x[rs1] | x[rs2]; break; }
    case F3_XOR: { x[rd] = x[rs1] ^ x[rs2]; break; }
    case F3_SLT: { x[rd] = (int32_t)x[rs1] < (int32_t)x[rs2] ? 1 : 0; break; } //if (x[rs1] < x[rs2]) x[rd] = 1 else x[rd] = 0
    case F3_SLTU: { x[rd] = x[rs1] < x[rs2] ? 1 : 0; break; }
    case F3_SLL: { x[rd] = x[rs1] << (x[rs2] & 0x1F); break; }
    case F3_SRL_SRA: {
      uint32_t funct7 = (instr >> 25) & 0x7F;   // bits 31:25
      switch(funct7) {
        case 0b0000000: { x[rd] = x[rs1] >> (x[rs2] & 0x1F); break; } //SRLI
        case 0b0100000: { x[rd] = (int32_t)x[rs1] >> (x[rs2] & 0x1F); break; } //SRAI
        default: { illegal(instr); break; }
      }
      break;
    }
    default: { illegal(instr); break; }
  }
  programcounter += 4;
  x[0] = 0;
}


void lui(uint32_t instr) {
  uint32_t rd     = bits(instr, 11, 7); //(instr >> 7)  & 0x1F;   // bits 11:7
  int32_t imm_u = instr & 0xFFFFF000;
  x[rd] = (int32_t)imm_u;
  x[0] = 0;
  programcounter += 4;
}

void auipc(uint32_t instr) {
  uint32_t rd     = bits(instr, 11, 7); //(instr >> 7)  & 0x1F;   // bits 11:7
  int32_t imm_u = instr & 0xFFFFF000;
  x[rd] = pc_inst + (int32_t)imm_u;
  x[0] = 0;
  programcounter += 4;
}

void jal(uint32_t instr) {
  uint32_t rd     = bits(instr, 11, 7); //(instr >> 7)  & 0x1F;   // bits 11:7
  int32_t imm_j = sext((bits(instr, 31, 31) << 20) |
      (bits(instr, 19, 12)<< 12) |
      (bits(instr, 20, 20)<< 11) |
      (bits(instr, 30, 21)<< 1) ,21);
  x[rd] = (int32_t)pc_inst+4;
  programcounter += imm_j;
  x[0] = 0;
}

void jalr(uint32_t instr) {
  uint32_t rd     = bits(instr, 11, 7); //(instr >> 7)  & 0x1F;   // bits 11:7
  uint32_t rs1    = bits(instr, 19, 15); //(instr >> 15) & 0x1F;   // bits 19:15
  int32_t imm_i = sext(bits(instr, 31, 20), 12); //(int32_t)instr >> 20;
                                                 //programcounter = (x[rs1]+sext(imm_i, 12));
  uint32_t next = programcounter + 4;
  programcounter = (x[rs1] + imm_i) & ~1;
  x[rd] = next;
  x[0] = 0;
}

void branch(uint32_t instr) {
  uint32_t rs1    = bits(instr, 19, 15); //(instr >> 15) & 0x1F;   // bits 19:15
  uint32_t rs2    = bits(instr, 24, 20); //(instr >> 20) & 0x1F;   // bits 24:20
  uint32_t funct3 = bits(instr, 14, 12); //(instr >> 12) & 0x07;   // bits 14:12
  uint32_t imm_b = sext((bits(instr, 31, 31) << 12) |
      (bits(instr, 7, 7)   << 11) |
      (bits(instr, 30, 25)<< 5)  |
      (bits(instr, 11, 8) << 1), 13);
  switch(funct3) {
    case F3_BEQ: { programcounter += (x[rs1] == x[rs2]) ? imm_b : 4; break; }
    case F3_BNE: { programcounter += (x[rs1] != x[rs2]) ? imm_b : 4; break; }
    case F3_BLT: { programcounter += (x[rs1] < (int32_t)x[rs2]) ? imm_b : 4; break; }
    case F3_BLTU: { programcounter += (x[rs1] < x[rs2]) ? imm_b : 4; break; }
    case F3_BGE: { programcounter += (x[rs1] >= (int32_t)x[rs2]) ? imm_b : 4; break; }
    case F3_BGEU: { programcounter += (x[rs1] >= x[rs2]) ? imm_b : 4; break; }
    default: { illegal(instr); break; }
  }
}

void load(uint32_t instr) {
  int32_t srd     = bits(instr, 11, 7); //(instr >> 7)  & 0x1F;   // bits 11:7
  uint32_t rs1    = bits(instr, 19, 15); //(instr >> 15) & 0x1F;   // bits 19:15
  uint32_t funct3 = bits(instr, 14, 12); //(instr >> 12) & 0x07;   // bits 14:12
  int32_t imm_i = sext(bits(instr, 31, 20), 12); //(int32_t)instr >> 20;
  uint32_t addr = x[rs1] + imm_i; 
  switch(funct3) {
    case F3_LB: {
      x[srd] = sext(memory[addr], 8);
      break;
    }
    case F3_LH: {
      x[srd] = sext(memory[addr] | (memory[addr+1] << 8), 16);
      break;
    }
    case F3_LW: {
      x[srd] =
        memory[addr] |
        (memory[addr+1] << 8) |
        (memory[addr+2] << 16) |
        (memory[addr+3] << 24);
      break;
    }
    case F3_LBU: { x[srd] = memory[addr]; break; }
    case F3_LHU: {
      x[srd] = memory[addr] | (memory[addr+1] << 8);
      break;
    }
    default: { illegal(instr); break; }
  }
  programcounter += 4;
  x[0] = 0;
}

void store(uint32_t instr) {
  uint32_t rs1    = bits(instr, 19, 15); //(instr >> 15) & 0x1F;   // bits 19:15
  uint32_t rs2    = bits(instr, 24, 20); //(instr >> 20) & 0x1F;   // bits 24:20
  uint32_t funct3 = bits(instr, 14, 12); //(instr >> 12) & 0x07;   // bits 14:12
  int32_t imm_s = sext((bits(instr, 31, 25) << 5) |
      bits(instr, 11, 7),
      12);
  uint32_t addr =  x[rs1] + imm_s;
  switch(funct3) {
    case F3_SB: {
      memory[addr] = x[rs2] & 0xFF;
      break;
    }
    case F3_SH: {
      memory[addr + 0] = (x[rs2] >> 0) & 0xFF;
      memory[addr + 1] = (x[rs2] >> 8) & 0xFF;
      break;
    }
    case F3_SW: {
      memory[addr + 0] = (x[rs2] >> 0)  & 0xFF;
      memory[addr + 1] = (x[rs2] >> 8)  & 0xFF;
      memory[addr + 2] = (x[rs2] >> 16) & 0xFF;
      memory[addr + 3] = (x[rs2] >> 24) & 0xFF;
      break;
    }
    default: { illegal(instr); break; }
  }
  programcounter += 4;
}

//void env(uint32_t instr) {
//  int32_t imm_i = sext(bits(instr, 31, 20), 12); //(int32_t)instr >> 20;
//  switch (imm_i) {
//    case 0x0: { exit(1); break; } //ECALL
//    case 0x1: { exit(1); break; } //EBREAK
//    default: { illegal(instr); break; }
//  }
//}

void env(uint32_t instr) {
  int32_t imm_i = sext(bits(instr, 31, 20), 12);

  if (imm_i != 0) {
    illegal(instr);
    return;
  }

  uint32_t syscall = x[17]; // a7

  switch (syscall) {

    // exit(int code)
    case 93: {
      uint32_t code = x[10]; // a0
      printf("\n[exit %u]\n", code);
      exit(code);
    }

    // write(int fd, const void *buf, size_t count)
    case 64: {
      uint32_t fd    = x[10]; // a0
      uint32_t addr  = x[11]; // a1
      uint32_t len   = x[12]; // a2

      if (fd == 1 || fd == 2) { // stdout / stderr
        for (uint32_t i = 0; i < len; i++)
          putchar(memory[addr + i]);
        fflush(stdout);
        x[10] = len; // return bytes written
      } else {
        x[10] = -1;
      }
      break;
    }

    // read(int fd, void *buf, size_t count)
    case 63: {
      uint32_t fd    = x[10]; // a0
      uint32_t addr  = x[11]; // a1
      uint32_t len   = x[12]; // a2

      if (fd == 0) { // stdin
        uint32_t i;
        for (i = 0; i < len; i++) {
          int c = getchar();
          if (c == EOF) break;
          memory[addr + i] = (uint8_t)c;
        }
        x[10] = i; // bytes read
      } else {
        x[10] = -1;
      }
      break;
    }

    default:
      printf("Unknown syscall %u\n", syscall);
      x[10] = -1;
      break;
  }
  programcounter += 4;
  x[0] = 0;
}

void fence(uint32_t instr) {
  int32_t imm_i = sext(bits(instr, 31, 20), 12); //(int32_t)instr >> 20;
  switch (imm_i) {
    case 0b000000010000: { getchar(); break; } //PAUSE
    case 0b100000110011: { break; } //FENCE.TSO
                                    //case _: { break; } //FENCE
  }
  printf("fence");
  programcounter += 4;
}

typedef void (*OpHandler)(uint32_t instr);

OpHandler opcode_table[256] = {
  [0b0010011] = op_imm,
  [0b0110111] = lui,
  [0b0010111] = auipc,
  [0b0110011] = op,
  [0b1101111] = jal,
  [0b1100111] = jalr,
  [0b1100011] = branch,
  [0b0000011] = load,
  [0b0100011] = store,
  [0b1110011] = env,
  [0b0001111] = fence,
};

void dump_reg() {
  printf("unsigned: ");
  for (int i=0; i<32; i++) {
    printf("%u ", x[i]);
  }
  printf("\n");
  printf("signed: ");
  for (int i=0; i<32; i++) {
    printf("%d ", (int32_t)x[i]);
  }
  printf("\n");
}


void load_binary(const char *path, uint32_t load_addr) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    perror("fopen");
    exit(1);
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (load_addr + size > sizeof(memory)) {
    printf("Binary too large\n");
    exit(1);
  }

  fread(&memory[load_addr], 1, size, f);
  fclose(f);

  programcounter = load_addr;

  printf("Loaded %ld bytes at 0x%08x\n", size, load_addr);
}


int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("usage: %s program.bin\n", argv[0]);
    return 1;
  }

  memset(x, 0, sizeof(x));
  memset(memory, 0, sizeof(memory));

  load_binary(argv[1], 0x0);

  while (1) {
    uint32_t instr =
      memory[programcounter + 0] |
      (memory[programcounter + 1] << 8) |
      (memory[programcounter + 2] << 16) |
      (memory[programcounter + 3] << 24);
    uint32_t opcode = instr & 0x7F;
    printf("%u\n", opcode);
    if (!opcode_table[opcode]) {
      printf("Illegal opcode %02x at pc=%08x\n", opcode, programcounter);
      exit(1);
    }
    pc_inst = programcounter; 
    opcode_table[opcode](instr);
    //if (programcounter == pc)
    if (programcounter & 3) {
      printf("PC misaligned: %08x\n", programcounter);
      exit(1);
    }
  }
  //uint32_t instr = 0b11111111110000001000001010010011;

  //uint32_t opcode =  bits(instr, 6, 0);//instr        & 0x7F; 
  //dump_reg();
  ////i_andi(0xFFC08293);
  //opcode_table[opcode](instr);
  //dump_reg();
  return 0;
}
