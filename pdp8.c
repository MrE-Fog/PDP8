#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define UNUSED(x) (void)(x)

#define  u8  uint8_t
#define u12 uint16_t

struct PDP8 {
  u12  memory[4096];
  //u12  memory_address;
  //u12  memory_buffer;
  
  // Internal processor state
  u12  accumulator;
  bool link;
  u12  program_counter;
  bool run;
  bool interrupt_enable;
  
  // External processor state
  u12  switch_register;
  bool interrupt_request;
  
  u12  instruction_register;
  u12  last_pc;
  bool restart;
};

void initialize(struct PDP8 *pdp8)
{
  pdp8->link = false;
  pdp8->accumulator = 0;
  pdp8->instruction_register = 0;
  pdp8->program_counter = 0;
  pdp8->switch_register = 0;
  pdp8->interrupt_request = false;
  pdp8->interrupt_enable = false;
  
  pdp8->last_pc = 0;
  pdp8->restart = false;
}

u12
effective_address(struct PDP8 *pdp8)
{
  u12 instr   = pdp8->instruction_register;
  u12 last_pc = pdp8->last_pc;
  
  // page bit * page address + page offset
  u12 eadd = ((instr & 0200) >> 7) * (last_pc & 07600) + (instr & 0177);
  
  if (((instr & 0400) >> 8) == 0) { // indirect bit
    return eadd;
  }
  
  // auto index
  u12 ceadd = pdp8->memory[eadd];
  if (eadd > 010 && eadd < 017) {
    pdp8->memory[eadd] = (ceadd + 1) & 07777;
  }
  
  return ceadd;
}

void
execute(struct PDP8 *pdp8)
{
  u12 ir = pdp8->instruction_register;
  int opcode = ir & 0xe00;
  switch (opcode) {
    case 000: { // AND
      u12 eadd  = effective_address(pdp8);
      u12 ceadd = pdp8->memory[eadd];
      
      pdp8->accumulator = pdp8->accumulator & ceadd;
    } break;
    case 001: { // TAD
      u12 eadd  = effective_address(pdp8);
      u12 ceadd = pdp8->memory[eadd];
      
      u12 ac = pdp8->accumulator + ceadd;
      if (ac > 07777) {
        pdp8->accumulator = pdp8->accumulator & 07777;
        pdp8->link        = !pdp8->link;
      }
    } break;
    case 002: { // ISZ
      u12 eadd  = effective_address(pdp8);
      u12 ceadd = pdp8->memory[eadd];
      
      pdp8->memory[eadd] = (ceadd + 1) & 07777;
      if (pdp8->memory[eadd] == 0) {
        pdp8->program_counter++;
      }
    } break;
    case 003: { // DCA
      u12 eadd  = effective_address(pdp8);
      
      pdp8->memory[eadd] = pdp8->accumulator;
      pdp8->accumulator = 0;
    } break;
    case 004: { // JMS
      u12 eadd  = effective_address(pdp8);
      
      pdp8->memory[eadd] = pdp8->program_counter;
      pdp8->program_counter = (eadd + 1) & 07777;
    } break;
    case 005: { // JMP
      u12 eadd  = effective_address(pdp8);
      
      pdp8->program_counter = eadd;
    } break;
    case 006: { // IOT
      switch (ir & 0777) {
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
      
    } break;
  }
}

bool
step(struct PDP8 *pdp8)
{
  pdp8->instruction_register = pdp8->memory[pdp8->program_counter];
  pdp8->last_pc = pdp8->program_counter;
  
  pdp8->program_counter++;
  execute(pdp8);
  
  if (pdp8->restart) { return true; }
  
  if (pdp8->interrupt_enable && pdp8->interrupt_request) {
    pdp8->memory[0] = pdp8->program_counter;
    pdp8->program_counter = 1;
  }
  
  return true;
}

bool
interpret(struct PDP8 *pdp8)
{
  bool run = true;
  while (run) {
    run = step(pdp8);
  }
  return run;
}

int
main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);
  
  struct PDP8 pdp8;
  initialize(&pdp8);
  
  return 0;
}