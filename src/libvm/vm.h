#ifndef __SUNBLINDCTL_VM_VM_H__
#define __SUNBLINDCTL_VM_VM_H__

#include <stdint.h>
#include <vm.h>

uint16_t vmDefaultMemoryRead(unsigned char * memory, uint16_t address, int half);
void vmDefaultMemoryWrite(unsigned char * memory, uint16_t address, uint16_t data, int half);

#endif
