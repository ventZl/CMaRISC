#ifndef __SUNBLIND_INSTRUCTION_H__
#define __SUNBLIND_INSTRUCTION_H__

#include "bits.h"

#define COND_MASK		(BIT15 | BIT14)

#define COND_ALWAYS		0
#define COND_ZERO		(BIT14)
#define COND_CARRY		(BIT15)
#define COND_SIGN		COND_MASK

#define OP_BRANCH			3

#define OP_LOAD				0
#define OP_STORE			1
#define OP_ILOAD			1

#define OP_REGMOVE			0x16
#define OP_MOV				0x2C
#define OP_SWAP				0x2D

#define OP_ADD				8
#define OP_SUB				9
#define OP_MUL				10
#define OP_DIV				11
#define OP_MOD				12
#define OP_AND				13
#define OP_OR				14
#define OP_XOR				15

#define OP_ADDC				16
#define OP_SUBC				17

#define OP_SHIFT			20
#define OP_NOT				0x150
#define OP_FLINVERT			0x151
#define OP_INT				0x2E

#define IF_EXEC_ALWAYS(_i)	((_i & COND_MASK) == COND_ALWAYS)
#define IF_IS_ZERO(_i)		((_i & COND_MASK) == COND_ZERO)
#define IF_IS_CARRY(_i)		((_i & COND_MASK) == COND_CARRY)
#define IF_IS_SIGN(_i)		((_i & COND_MASK) == COND_SIGN)

#define OPCODE(_i, _bits)	((_i >> (14 - _bits)) & ((1 << _bits) - 1))
#define LOADSTORE_INDIRECTION(_i)		((_i >> 8) & 3)

#define IS_INDIRECT(_i)			(LOADSTORE_INDIRECTION(_i) == 0)
#define IS_PRE_DECREMENT(_i)	(LOADSTORE_INDIRECTION(_i) == 1)
#define IS_POST_INCREMENT(_i)	(LOADSTORE_INDIRECTION(_i) == 2)
#define IS_8BIT(_i)				(LOADSTORE_INDIRECTION(_i) == 3)

#define IS_BRANCH(_i)		(OPCODE(_i, 2) == OP_BRANCH)

#define IS_LOAD(_i) 		(OPCODE(_i, 4) == OP_LOAD)
#define IS_STORE(_i) 		(OPCODE(_i, 4) == OP_STORE)

#define IS_ILOAD(_i) 		(OPCODE(_i, 3) == OP_ILOAD)

#define IS_REGMOVE(_i)		(OPCODE(_i, 5) == OP_REGMOVE)

#define IS_MOV(_i)			(OPCODE(_i, 6) == OP_MOV)
#define IS_SWAP(_i)			(OPCODE(_i, 6) == OP_SWAP)

#define IS_ALU2(_i)			(OPCODE(_i, 2) == 1)

#define IS_ADD(_i)			(OPCODE(_i, 5) == OP_ADD)
#define IS_SUB(_i)			(OPCODE(_i, 5) == OP_SUB)
#define IS_MUL(_i)			(OPCODE(_i, 5) == OP_MUL)
#define IS_DIV(_i)			(OPCODE(_i, 5) == OP_DIV)
#define IS_MOD(_i)			(OPCODE(_i, 5) == OP_MOD)
#define IS_AND(_i)			(OPCODE(_i, 5) == OP_AND)
#define IS_OR(_i)			(OPCODE(_i, 5) == OP_OR)
#define IS_XOR(_i)			(OPCODE(_i, 5) == OP_XOR)

#define IS_ADDC(_i)			(OPCODE(_i, 5) == OP_ADDC)
#define IS_SUBC(_i)			(OPCODE(_i, 5) == OP_SUBC)

#define IS_SHIFT(_i)		(OPCODE(_i, 5) == OP_SHIFT)

#define IS_NOT(_i)			(OPCODE(_i, 9) == OP_NOT)
#define IS_FLINVERT(_i)		(OPCODE(_i, 9) == OP_FLINVERT)

#define IS_INT(_i)			(OPCODE(_i, 6) == OP_INT)

#define GET_ARG1(_i)		((_i >> 4) & 0xF)
#define GET_ARG2(_i)		(_i & 0xF)
#define GET_IMMEDIATE(_i)	(_i & 0xFF)
#define GET_LARGEIMMED(_i)	(_i & 0x0FFF)
#define GET_OPFLAG(_i)		((_i >> 8) & 1)

#define SET_ARG1(_i)		((_i & 0xF) << 4)
#define SET_ARG2(_i)		(_i & 0xF)
#define SET_IMMEDIATE(_i)	(_i & 0xFF)
#define SET_LARGEIMMED(_i)	(_i & 0x0FFF)
#define SET_OPFLAG(_i)		((_i & 1) << 8)

#define TOOPCODE(_i, _bits)	((_i & ((1 << _bits) - 1)) << (14 - _bits))

#define MK_BRANCH			TOOPCODE(OP_BRANCH, 2)

#define MK_LOAD		 		TOOPCODE(OP_LOAD, 4)
#define MK_STORE	 		TOOPCODE(OP_STORE, 4)

#define MK_ILOAD	 		TOOPCODE(OP_ILOAD, 3)

#define MK_REGMOVE			TOOPCODE(OP_REGMOVE, 5)

#define MK_MOV				TOOPCODE(OP_MOV, 6)
#define MK_SWAP				TOOPCODE(OP_SWAP, 6)

#define MK_ALU2				TOOPCODE(1, 2)

#define MK_ADD				TOOPCODE(OP_ADD, 5)
#define MK_SUB				TOOPCODE(OP_SUB, 5)
#define MK_MUL				TOOPCODE(OP_MUL, 5)
#define MK_DIV				TOOPCODE(OP_DIV, 5)
#define MK_MOD				TOOPCODE(OP_MOD, 5)
#define MK_AND				TOOPCODE(OP_AND, 5)
#define MK_OR				TOOPCODE(OP_OR, 5)
#define MK_XOR				TOOPCODE(OP_XOR, 5)

#define MK_ADDC				TOOPCODE(OP_ADDC, 5)
#define MK_SUBC				TOOPCODE(OP_SUBC, 5)

#define MK_SHIFT			TOOPCODE(OP_SHIFT, 5)

#define MK_NOT				TOOPCODE(OP_NOT, 9)
#define MK_FLINVERT			TOOPCODE(OP_FLINVERT, 9)

#define MK_INT				TOOPCODE(OP_INT, 6)

#endif
