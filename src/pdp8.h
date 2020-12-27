#if defined (__cplusplus)
extern "C" {
#endif
  
#ifndef PDP8_H
#define PDP8_H
  
#include <stdint.h>
  
  typedef unsigned int uint;
  
  enum PDP8_Constants {
    PDP8_WORD_SIZE   = 12,
    PDP8_WORD_MASK   = 07777,
    PDP8_WORD_SIGN   = 04000,
    PDP8_MEMORY_SIZE = 4096,
  };
  
  struct PDP8 {
    uint memory[PDP8_MEMORY_SIZE]; //  M\Memory[0:4095]<0:11>
    uint ma                : 12;   //  MA\Memory.Address<0:11>
    uint mb                : 12;   //  MB\Memory.Buffer<0:11>
    
    // Internal processor state
    union {
      uint lac             : 13;   //  LAC<0:12>
      struct {
        uint ac            : 12;   //    AC\Accumulator<0:11> := LAC<1:12>
        uint link          :  1;   //    L\Link< >            := LAC<0>
      };
    };
    uint pc                : 12;   //  PC\Program.Counter<0:11>
    uint run               :  1;   //  RUN< >
    uint interrupt_enable  :  1;   //  INTERRUPT.ENABLE< >
    
    // External processor state
    uint interrupt_request :  1;   //  INTERRUPT.REQUEST< >
    uint switches          : 12;   //  SWITCHES<0:11>
    
    uint ir                : 12;   //  i\instruction<0:11>
    uint last_pc           : 12;   //  last.pc<0:11>
    uint restart           :  1;
  };
  
  struct PDP8_Program {
    uint code_length;
    uint code[4096];
  };
  
  extern struct PDP8_Program PDP8_BinaryToProgram(const char *file_name);
  extern void PDP8_ReloadProgram(struct PDP8 *pdp8, struct PDP8_Program *program);
  extern void PDP8_Reset(struct PDP8 *pdp8);
  extern void PDP8_MemoryReset(struct PDP8 *pdp8);
  extern bool PDP8_Step(struct PDP8 *pdp8);
  extern bool PDP8_Run(struct PDP8 *pdp8);
  extern void PDP8_Load(struct PDP8 *pdp8, struct PDP8_Program *program);
  
#endif //PDP8_H
  
#if defined (__cplusplus)
}
#endif