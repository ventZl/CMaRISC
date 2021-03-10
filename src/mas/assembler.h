#ifndef __SUNBLIND_ASSEMBLER_H__
#define __SUNBLIND_ASSEMBLER_H__

//#include "object.h"

typedef unsigned short INSTRUCTION;
typedef unsigned char REGISTER;
typedef unsigned char CONDITION;
typedef unsigned char INDIRECTION;

enum instr_condition { ALWAYS = 0, ZERO = 1, CARRY = 2, SIGN = 3 };
enum indirect_behavior { NORMAL = 0, PRE_DECREMENT = 1, POST_INCREMENT = 2, REF_8_BIT = 3 , NONE = 4 };

void do_warn_unsafe_assembly();
INSTRUCTION createMOV(CONDITION cond, REGISTER source, REGISTER destination);
INSTRUCTION createSWAP(CONDITION cond, REGISTER source, REGISTER destination);
INSTRUCTION createINT(CONDITION cond, unsigned char vector);
INSTRUCTION createADD(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination);
INSTRUCTION createSUB(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination);
INSTRUCTION createMUL(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination);
INSTRUCTION createDIV(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination);
INSTRUCTION createMOD(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination);
INSTRUCTION createAND(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination);
INSTRUCTION createOR(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination);
INSTRUCTION createXOR(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination);
INSTRUCTION createADDC(CONDITION cond, unsigned char flag, REGISTER source, char immediate);
INSTRUCTION createSUBC(CONDITION cond, unsigned char flag, REGISTER source, char immediate);
INSTRUCTION createNOT(CONDITION cond, unsigned char flag, REGISTER reg);
INSTRUCTION createBRANCH(CONDITION cond, short relative_address, char link);
INSTRUCTION createLOAD(CONDITION cond, INDIRECTION type, REGISTER ref, REGISTER reg);
INSTRUCTION createILOAD(CONDITION cond, REGISTER dest, unsigned char IMMED);
INSTRUCTION createSTORE(CONDITION cond, INDIRECTION type, REGISTER ref, REGISTER reg);
INSTRUCTION createSHIFT(CONDITION cond, REGISTER reg, signed amount);

#endif
