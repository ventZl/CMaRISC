#include <string.h>

#include <vm.h>
#include "vm.h"

#include "bits.h"
#include "registers.h"
#include "instruction.h"

#define MEM_OP_BYTE			1
#define MEM_OP_WORD			0

/** Vytvori popisovac virtualneho stroja.
 * @param memory adresa pamate vyhradenej ako virtualna pamat virtualneho stroja
 * @param memory_size velkost virtualnej pamate
 * @param pc startovacia adresa behu virtualneho stroja
 * @return popisovac virtualneho stroja
 */
VIRTUAL_MACHINE * createVirtualMachine(char * memory, uint16_t memory_size, uint16_t pc) {
	VIRTUAL_MACHINE * mach = malloc(sizeof(VIRTUAL_MACHINE));
	memset(mach, 0, sizeof(VIRTUAL_MACHINE));
	mach->memory = memory;
	mach->mem_size = memory_size;
	memset(mach->registers, 0, sizeof(mach->registers));
	mach->read_func = vmDefaultMemoryRead;
	mach->write_func = vmDefaultMemoryWrite;
	mach->registers[15] = pc;
	mach->flags = 0;
	return mach;
}

/** Vypise obsah registrov virtualneho stroja v ludsky citatelnej forme.
 * @param machine popisovac virtualneho stroja
 */
void dumpRegistersVirtualMachine(VIRTUAL_MACHINE * machine) {
    int q;
    for (q = 0; q < 8; q++) {
        printf("R%02d: %04X\tR%02d: %04X\n", q, machine->registers[q], q + 8, machine->registers[q+8]);
    }
    return;
}
