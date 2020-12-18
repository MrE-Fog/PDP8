#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "pdp8.h"

//#define BIT_TEST(var, pos) ((var) & (1 << (pos)))
//#define DECODE(n, l, h) (((n) >> (11 - (h))) & ((1 << ((h) - (l) + 1)) - 1))

void initialize(struct PDP8 *pdp8) {
    pdp8->l = false;
    pdp8->ac = 0;
    pdp8->pc = 0;
    pdp8->run = false;
    pdp8->interrupt_enable = false;
    
    pdp8->interrupt_request = false;
    pdp8->switches = 0;
    
    pdp8->instruction = 0;
    pdp8->last_pc = 0;
    pdp8->restart = false;
}

#define decodeb(n, b) decode(n, b, b)

inline u12 decode(u12 n, int l, int h) {
    return ((n >> (11 - h)) & ((1 << (h - l + 1)) - 1));
}

u12 effective_address(struct PDP8 *pdp8) {
    u12 ir      = pdp8->instruction;
    u12 last_pc = pdp8->last_pc;
    
    // page bit * page address + page offset
    u12 eadd = ((bool)(ir & PAGE_BIT)) * (last_pc & CURRENT_PAGE) + (ir & PAGE_ADDRESS); 
    
    if ((ir & INDIRECT_BIT) == 0) {
        return eadd;
    }
    
    // auto index
    u12 ceadd = pdp8->memory[eadd];
    if (eadd > 010 && eadd < 017) {
        pdp8->memory[eadd] = (ceadd + 1) & WORD_MASK;
    }
    
    return ceadd;
}

void skip(struct PDP8 *pdp8) {
    uint ir = pdp8->instruction;
    uint ac = pdp8->ac;
    uint  l = pdp8->l;
    
    bool skip = false;
    if (ir & OPR_IS) { // invert skip condition
        if ((ir & OPR_SZL & OPR_SNA & OPR_SPA) == 0) skip = true;
        if ((ir & OPR_SZL) && (l == 0))              skip = true;
        if ((ir & OPR_SNA) && (ac > 0))              skip = true;
        if ((ir & OPR_SPA) && (ac < WORD_SIGN))      skip = true;
    } else {
        if ((ir & OPR_SNL) && (l == 1))              skip = true;
        if ((ir & OPR_SZA) && (ac == 0))             skip = true;
        if ((ir & OPR_SMA) && (ac >= WORD_SIGN))     skip = true;
    }
    
    if (skip) {
        pdp8->pc = (pdp8->pc + 1) & WORD_MASK;
    }
}

void operate(struct PDP8 *pdp8) {
    u12 ir = pdp8->instruction;
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
            pdp8->l = false;
        }
        if (ir & OPR_CMA) { // cma
            pdp8->ac = ~(pdp8->ac) & WORD_MASK;
        }
        if (ir & OPR_CML) { // cml
            pdp8->l = !pdp8->l;
        }
        if (ir & OPR_IAC) { // iac
            pdp8->lac += 1;
        }
        if (ir & OPR_RT) { // rotate twice
            if (ir & OPR_RAL) { // ral
                pdp8->lac = (pdp8->lac << 1) + pdp8->l;
                pdp8->lac = (pdp8->lac << 1) + pdp8->l;
            }
            if (ir & OPR_RAR) { // rar
                pdp8->lac = (pdp8->lac >> 1) + (pdp8->ac << 12);
                pdp8->lac = (pdp8->lac >> 1) + (pdp8->ac << 12);
            }
        } else { // rotate once
            if (ir & OPR_RAL) { // ral
                pdp8->lac = (pdp8->lac << 1) + pdp8->l;
            }
            if (ir & OPR_RAR) { // rar
                pdp8->lac = (pdp8->lac >> 1) + (pdp8->ac << 12);
            }
        }
    }
}

void execute(struct PDP8 *pdp8) {
    u12 ir = pdp8->instruction;
    //int opcode = ir & 0xe00;
    switch ((ir & OPCODE) >> 9) {
        case 000: { // AND
            u12 eadd  = effective_address(pdp8);
            u12 ceadd = pdp8->memory[eadd];
            
            pdp8->ac = pdp8->ac & ceadd;
        } break;
        case 001: { // TAD
            u12 eadd  = effective_address(pdp8);
            u12 ceadd = pdp8->memory[eadd];
            
            pdp8->lac = pdp8->lac + ceadd;
        } break;
        case 002: { // ISZ
            u12 eadd  = effective_address(pdp8);
            u12 ceadd = pdp8->memory[eadd];
            
            pdp8->memory[eadd] = (ceadd + 1) & WORD_MASK;
            if (pdp8->memory[eadd] == 0) {
                pdp8->pc++;
            }
        } break;
        case 003: { // DCA
            u12 eadd  = effective_address(pdp8);
            
            pdp8->memory[eadd] = pdp8->ac;
            pdp8->ac = 0;
        } break;
        case 004: { // JMS
            u12 eadd  = effective_address(pdp8);
            
            pdp8->memory[eadd] = pdp8->pc;
            pdp8->pc = (eadd + 1) & WORD_MASK;
        } break;
        case 005: { // JMP
            u12 eadd  = effective_address(pdp8);
            
            pdp8->pc = eadd;
        } break;
        case 006: { // IOT
            switch (decode(ir, 3, 11)) {
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

bool step(struct PDP8 *pdp8) {
    pdp8->instruction = pdp8->memory[pdp8->pc];
    pdp8->last_pc = pdp8->pc;
    
    pdp8->pc++;
    execute(pdp8);
    
    if (pdp8->restart) { return true; }
    
    if (pdp8->interrupt_enable && pdp8->interrupt_request) {
        pdp8->memory[0] = pdp8->pc;
        pdp8->pc = 1;
    }
    
    return true;
}

bool interpret(struct PDP8 *pdp8) {
    bool run = true;
    while (run) {
        run = step(pdp8);
    }
    return run;
}

int main(int argc, char *argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    
    struct PDP8 pdp8;
    initialize(&pdp8);
    
    pdp8.lac = 4095;
    printf("lac: %d, ac: %d, l: %d\n", pdp8.lac, pdp8.ac, pdp8.l);
    pdp8.lac = (pdp8.lac << 1) + pdp8.l;
    printf("lac: %d, ac: %d, l: %d\n", pdp8.lac, pdp8.ac, pdp8.l);
    
    printf("opcode: %d\n", OPCODE);
    printf("pa: %d\n", PAGE_ADDRESS);
    
    return 0;
}

