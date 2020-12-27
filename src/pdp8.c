#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "pdp8.h"

#define UNUSED(x) (void)(x)

#define PDP8_BMASK(b) (1 << (11 - b))
#define PDP8_MASK(l, h) (((1 << ((h) - (l) + 1)) - 1) << (11 - (h)))

enum INSTRUCTION_FORMAT_MASK {
  OPCODE       = PDP8_MASK(0, 2),  // op\operation.code<0:2> := i<0:2>
  INDIRECT_BIT = PDP8_BMASK(3),    // ib\indirect.bit< >     := i<3>
  PAGE_BIT     = PDP8_BMASK(4),    // pb\page.0.bit  < >     := i<4>
  PAGE_ADDRESS = PDP8_MASK(5, 11), // pa\page.address<0:6>   := i<5:11>
  CURRENT_PAGE = PDP8_MASK(0, 4),
};

enum IO_MASK {
  IO_SELECT    = PDP8_MASK(3, 8),  // IO.SELECT<0:5>   := i<3:8>   !device select
  IO_CONTROL   = PDP8_MASK(9, 11), // io.control<0:2>  := i<9:11>  !device operation
  IO_PULSE_P1  = PDP8_BMASK(9),    //   IO.PULSE.P1< > := io.control<0>
  IO_PULSE_P2  = PDP8_BMASK(10),   //   IO.PULSE.P2< > := io.control<1>
  IO_PULSE_P4  = PDP8_BMASK(11),   //   IO.PULSE.P4< > := io.control<2>
  IO_MICROOP   = PDP8_MASK(3, 11),
};

enum OPR_MASK {
  OPR_GROUP = PDP8_BMASK(3),  // group< > := i<3>   !microinstruction group
  OPR_SMA   = PDP8_BMASK(5),  // sma< >   := i<5>   !skip on minus AC
  OPR_SPA   = PDP8_BMASK(5),  // spa< >   := i<5>   !skip on positive AC
  OPR_SZA   = PDP8_BMASK(6),  // sza< >   := i<6>   !skip on zero AC
  OPR_SNA   = PDP8_BMASK(6),  // sna< >   := i<6>   !skip on AC not zero
  OPR_SZL   = PDP8_BMASK(7),  // szl< >   := i<7>   !skip on zero L
  OPR_SNL   = PDP8_BMASK(7),  // snl< >   := i<7>   !skip on L not zero
  OPR_IS    = PDP8_BMASK(8),  // is< >    := i<8>   !invert skip sense
  
  OPR_CLA   = PDP8_BMASK(4),  // cla< >   := i<4>   !clear AC
  OPR_CLL   = PDP8_BMASK(5),  // cll< >   := i<5>   !clear L
  OPR_CMA   = PDP8_BMASK(6),  // cma< >   := i<6>   !complement AC
  OPR_CML   = PDP8_BMASK(7),  // cml< >   := i<7>   !complement L
  OPR_RAR   = PDP8_BMASK(8),  // rar< >   := i<8>   !rotate right
  OPR_RAL   = PDP8_BMASK(9),  // ral< >   := i<9>   !rotate left
  OPR_RT    = PDP8_BMASK(10), // rt< >    := i<10>  !rotate twice
  OPR_IAC   = PDP8_BMASK(11), // iac< >   := i<11>  !increment AC
  
  OPR_OSR   = PDP8_BMASK(9),  // osr< >   := i<9>   !logical or AC with SWITCHES
  
  OPR_HLT   = PDP8_BMASK(10), // hlt< >   := i<10>  !halt the processor
};

inline void memory_read(struct PDP8 *pdp8) {
  pdp8->mb = pdp8->memory[pdp8->ma];
}

inline uint PDP8_MemoryRead(struct PDP8 *pdp8, uint address) {
  pdp8->ma = address;
  memory_read(pdp8);
  return pdp8->mb;
}

inline void memory_write(struct PDP8 *pdp8) {
  pdp8->memory[pdp8->ma] = pdp8->mb;
}

inline void PDP8_MemoryWrite(struct PDP8 *pdp8, uint address, uint value) {
  pdp8->ma = address;
  pdp8->mb = value;
  memory_write(pdp8);
}

void effective_address(struct PDP8 *pdp8) {
  uint ir      = pdp8->ir;
  uint last_pc = pdp8->last_pc;
  
  // page bit * page address + page offset
  uint eadd = ((bool)(ir & PAGE_BIT)) * (last_pc & CURRENT_PAGE) + (ir & PAGE_ADDRESS);
  
  if ((ir & INDIRECT_BIT) == 0) {
    pdp8->ma = eadd;
    return;
  }
  
  // auto index
  uint ceadd = PDP8_MemoryRead(pdp8, eadd);
  if (eadd > 010 && eadd < 017) {
    ceadd = (ceadd + 1) & PDP8_WORD_MASK;
    PDP8_MemoryWrite(pdp8, eadd, ceadd);
  }
  
  pdp8->ma = ceadd;
}

inline uint PDP8_EffectiveAddress(struct PDP8 *pdp8) {
  effective_address(pdp8);
  return pdp8->ma;
}

void skip(struct PDP8 *pdp8) {
  uint ir   = pdp8->ir;
  uint ac   = pdp8->ac;
  uint link = pdp8->link;
  
  bool skip = false;
  if (ir & OPR_IS) { // invert skip condition
    if ((ir & OPR_SZL & OPR_SNA & OPR_SPA) == 0)  skip = true;
    if ((ir & OPR_SZL) && (link == 0))            skip = true;
    if ((ir & OPR_SNA) && (ac > 0))               skip = true;
    if ((ir & OPR_SPA) && (ac < PDP8_WORD_SIGN))  skip = true;
  } else {
    if ((ir & OPR_SNL) && (link == 1))            skip = true;
    if ((ir & OPR_SZA) && (ac == 0))              skip = true;
    if ((ir & OPR_SMA) && (ac >= PDP8_WORD_SIGN)) skip = true;
  }
  
  if (skip) {
    pdp8->pc = (pdp8->pc + 1) & PDP8_WORD_MASK;
  }
}

void operate(struct PDP8 *pdp8) {
  uint ir = pdp8->ir;
  if (ir & OPR_GROUP) { // group 2 and 3
    if (ir & 1) { // group 3
      if (ir & OPR_CLA) {
        pdp8->ac = 0;
      }
    } else {
      skip(pdp8);
      if (ir & OPR_CLA) {
        pdp8->ac = 0;
      }
      if (ir & OPR_OSR) { // osr
        pdp8->ac |= pdp8->switches;
      }
      if (ir & OPR_HLT) { // hlt
        pdp8->run = false;
      }
    }
  } else { // group 1
    if (ir & OPR_CLA) { // cla
      pdp8->ac = 0;
    }
    if (ir & OPR_CLL) { // cll
      pdp8->link = false;
    }
    if (ir & OPR_CMA) { // cma
      pdp8->ac = ~(pdp8->ac) & PDP8_WORD_MASK;
    }
    if (ir & OPR_CML) { // cml
      pdp8->link = !pdp8->link;
    }
    if (ir & OPR_IAC) { // iac
      pdp8->lac += 1;
    }
    if (ir & OPR_RT) { // rotate twice
      if (ir & OPR_RAL) { // ral
        pdp8->lac = (pdp8->lac << 1) + pdp8->link;
        pdp8->lac = (pdp8->lac << 1) + pdp8->link;
      }
      if (ir & OPR_RAR) { // rar
        pdp8->lac = (pdp8->lac >> 1) + (pdp8->ac << 12);
        pdp8->lac = (pdp8->lac >> 1) + (pdp8->ac << 12);
      }
    } else { // rotate once
      if (ir & OPR_RAL) { // ral
        pdp8->lac = (pdp8->lac << 1) + pdp8->link;
      }
      if (ir & OPR_RAR) { // rar
        pdp8->lac = (pdp8->lac >> 1) + (pdp8->ac << 12);
      }
    }
  }
}

void execute(struct PDP8 *pdp8) {
  uint ir = pdp8->ir;
  
  switch ((ir & OPCODE) >> 9) {
    case 000: { // AND
      effective_address(pdp8);
      memory_read(pdp8);
      
      pdp8->ac = pdp8->ac & pdp8->mb;
    } break;
    case 001: { // TAD
      effective_address(pdp8);
      memory_read(pdp8);
      
      pdp8->lac = pdp8->lac + pdp8->mb;
    } break;
    case 002: { // ISZ
      effective_address(pdp8);
      memory_read(pdp8);
      
      pdp8->mb = (pdp8->mb + 1) & PDP8_WORD_MASK;
      memory_write(pdp8);
      
      if (pdp8->mb == 0) {
        pdp8->pc++;
      }
    } break;
    case 003: { // DCA
      effective_address(pdp8);
      
      pdp8->mb = pdp8->ac;
      memory_write(pdp8);
      
      pdp8->ac = 0;
    } break;
    case 004: { // JMS
      effective_address(pdp8);
      
      pdp8->mb = pdp8->pc;
      memory_write(pdp8);
      
      pdp8->pc = (pdp8->ma + 1) & PDP8_WORD_MASK;
    } break;
    case 005: { // JMP
      effective_address(pdp8);
      
      pdp8->pc = pdp8->ma;
    } break;
    case 006: { // IOT
      switch (ir & IO_MICROOP) {
        case 001: { // ION - Interrupt System On
          pdp8->interrupt_enable = true;
          pdp8->restart = true;
        } break;
        case 002: { // IOF - Interrupt System Off
          pdp8->interrupt_enable = false;
        } break;
      }
    } break;
    case 007: { // OPR
      operate(pdp8);
    } break;
  }
}

void PDP8_MemoryReset(struct PDP8 *pdp8) {
  for (int i = 0; i < PDP8_MEMORY_SIZE; ++i) {
    pdp8->memory[i] = 0;
  }
}

void PDP8_Reset(struct PDP8 *pdp8) {
  pdp8->ma = 0;
  pdp8->mb = 0;
  
  pdp8->link = false;
  pdp8->ac = 0;
  pdp8->pc = 0;
  pdp8->run = false;
  pdp8->interrupt_enable = false;
  
  pdp8->interrupt_request = false;
  pdp8->switches = 0;
  
  pdp8->ir = 0;
  pdp8->last_pc = 0;
  pdp8->restart = false;
}

void PDP8_Load(struct PDP8 *pdp8, struct PDP8_Program *program) {
  for (int addr = 0; addr < program->code_length; ++addr) {
    pdp8->memory[addr] = program->code[addr];
  }
}

#define MAX_CODE_LENGTH 4096

struct PDP8_Program PDP8_BinaryToProgram(const char *file_name) {
  FILE *file;
  
  file = fopen(file_name, "rb");
  if (!file) {
    fprintf(stderr, "Error opening file: %s", file_name);
    exit(1);
  }
  
  struct PDP8_Program program;
  unsigned char buffer[2];
  program.code_length = 0;
  while (fread(buffer, 2, 1, file) == 1) {
    program.code[program.code_length] = ((buffer[0] << 6) + buffer[1]) & PDP8_WORD_MASK;
    program.code_length++;
  }
  
  fclose(file);
  
  return program;
}

void PDP8_ReloadProgram(struct PDP8 *pdp8, struct PDP8_Program *program) {
  // compare.lst
  PDP8_Load(pdp8, program);
  
  pdp8->memory[00070] = 32;
  pdp8->memory[00100] = 30;
  
  pdp8->pc = 00170;
}

bool PDP8_Step(struct PDP8 *pdp8) {
  pdp8->ir = PDP8_MemoryRead(pdp8, pdp8->pc);
  pdp8->last_pc = pdp8->pc;
  
  pdp8->pc++;
  execute(pdp8);
  
  if (pdp8->restart) return true;
  
  if (pdp8->interrupt_enable && pdp8->interrupt_request) {
    PDP8_MemoryWrite(pdp8, 0, pdp8->pc);
    pdp8->pc = 1;
  }
  
  return true;
}

bool PDP8_Run(struct PDP8 *pdp8) {
  bool run = true;
  while (run) {
    run = PDP8_Step(pdp8);
  }
  return run;
}

