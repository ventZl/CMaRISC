#ifndef __SUNBLIND_VM_H__
#define __SUNBLIND_VM_H__

#include <stdint.h>

typedef uint16_t (* vmMemoryRead)(unsigned char * memory, uint16_t address, int half);
typedef void (* vmMemoryWrite)(unsigned char * memory, uint16_t address, uint16_t data, int half);

enum VM_State { VM_OK = 0, VM_ILLEGAL_OPCODE, VM_SOFTINT, VM_OUT_OF_MEMORY, VM_DIVIDE_BY_ZERO, VM_UNALIGNED_MEMORY };

typedef uint8_t VM_STATE;

struct VirtualMachine {
	uint16_t registers[16];
	uint8_t flags;
	unsigned char * memory;
	uint16_t mem_size;
	vmMemoryRead read_func;
	vmMemoryWrite write_func;
	uint8_t ext_interrupt;
};

typedef struct VirtualMachine VIRTUAL_MACHINE;

void dumpRegistersVirtualMachine(VIRTUAL_MACHINE * machine);

VIRTUAL_MACHINE * createVirtualMachine(char * memory, uint16_t mem_size, uint16_t pc);
VM_STATE runVirtualMachine(VIRTUAL_MACHINE * machine);
VM_STATE traceVirtualMachine(VIRTUAL_MACHINE * machine, uint16_t instructions);

#endif
