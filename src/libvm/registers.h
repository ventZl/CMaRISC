#ifndef __SUNBLIND_REGISTERS_H__
#define __SUNBLIND_REGISTERS_H__

#define ZERO_FLAG		(1 << 7)
#define SIGN_FLAG		(1 << 6)
#define CARRY_FLAG		(1 << 5)
#define OVERFLOW_FLAG	(1 << 4)

#define PC				registers[15]
#define RL				registers[14]
#define SP				registers[13]

#endif
