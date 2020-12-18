#ifndef PDP8_H
#define PDP8_H

#include <stdlib.h>

#define UNUSED(x) (void)(x)

#define WORD_MASK 07777
#define WORD_SIGN 04000

typedef uint8_t u8;
typedef uint16_t u12;
typedef unsigned int uint;

struct PDP8 {
  u12  memory[4096];      //  M\Memory[0:4095]<0:11>
  //u12  memory_address;
  //u12  memory_buffer;

  // Internal processor state
  union {
    uint lac    : 13;     //  LAC<0:12>
    struct {
      uint ac   : 12;     //    AC\Accumulator<0:11> := LAC<1:12>
      uint link :  1;     //    L\Link< >            := LAC<0>
    };
  };
  u12  pc;                //  PC\Program.Counter<0:11>
  bool run;               //  RUN< >
  bool interrupt_enable;  //  INTERRUPT.ENABLE< >

  // External processor state
  bool interrupt_request; //  INTERRUPT.REQUEST< >
  u12  switches;          //  SWITCHES<0:11>

  u12  instruction;       //  i\instruction<0:11>
  u12  last_pc;           //  last.pc<0:11>
  bool restart;
};

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

#define decodeb(n, b) decode(n, b, b)

inline u12 decode(u12 n, int l, int h) {
  return ((n >> (11 - h)) & ((1 << (h - l + 1)) - 1));
}

#endif //PDP8_H

