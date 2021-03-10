#include <string.h>

#include <vm.h>
#include "vm.h"

#include "bits.h"
#include "registers.h"
#include "instruction.h"

#define MEM_OP_BYTE			1
#define MEM_OP_WORD			0

/** Standardna operacia pre citanie z pamate virtualneho procesora.
 * Tato operacia sa pouziva v pripade, ze virtualny procesor nema ziadne specialne
 * rozvrhnutie pamate (napr. v pamati mapovane registre, alebo pamatovo mapovane zariadenia
 * @param memory pointer na virtualnu pamat pocitaca
 * @param address adresa z ktorej maju byt data citane
 * @param half ak je 1, nacita iba jeden byte dat z adresy danej parametrom address, inac nacita 2 byty v poradi MSB, LSB z dvoch po sebe nasledujucich buniek danych parametrom address.
 * @return nacitana hodnota
 */
uint16_t vmDefaultMemoryRead(unsigned char * memory, uint16_t address, int half) {
	uint16_t ret = 0;
	ret = memory[address];
	if (half == 0) {
		ret |= (memory[address + 1] << 8);
	}
	return ret;
}

/** Standardna operacia pre zapis do pamate virtualneho procesora.
 * Tato operacia sa pouziva v pripade, ze virtualny procesor nema ziadne specialne
 * rozvrhnutie pamate (napr. v pamati mapovane registre, alebo pamatovo mapovane zariadenia
 * @param memory pointer na virtualnu pamat pocitaca
 * @param address adresa z ktorej maju byt data citane
 * @param half ak je 1, zapise iba jeden byte dat na adresu danej parametrom address, inac zapise 2 byty v poradi MSB, LSB na dve po sebe nasledujuce bunky dane parametrom address.
 */ 
void vmDefaultMemoryWrite(unsigned char * memory, uint16_t address, uint16_t data, int half) {
	memory[address] = data & 0xFF;
	if (half == 0) memory[address + 1] = (data >> 8) & 0xFF;
	return;
}

/** Overi, ci adresa je spravna.
 * Overi, ci dana adresa je v ramci limitov danych nastavenim virtualneho stroja, 
 * najma ci nie je vacsia, ako je velkost pamate a ci sa nejedna o nezarovnany pristup k pamati.
 * @param machine popisovac virtualneho stroja
 * @param address adresa, ktora sa overuje
 * @return stav overovania
 */
static inline uint8_t __checkAddressValid(VIRTUAL_MACHINE * machine, uint16_t address) {
	if (address & 1) return VM_UNALIGNED_MEMORY;
	else if (address >= machine->mem_size) return VM_OUT_OF_MEMORY;
	return VM_OK;
}

/** Vykona instrukcie virtualneho stroja.
 * Tato funkcia je jadrom virtualneho stroja. Vykonava instrukcie virtualneho 
 * stroja, cim virtualizuje (emuluje) jeho cinnost. Tato funkcia vykonava instrukcie
 * bud do vyskytu chyby, volania vonkajsieho prerusenia (instrukcia INT), alebo 
 * kym nie je dosiahnuty pocet instrukcii dany argumentom limit (v pripade, ze ma 
 * nenulovu hodnotu).
 * @note Ak funkcia pri volani nema limit na pocet vykonanych instrukcii, kod vovnutri stroja
 * moze sposobit, ze sa program vovnutri stroja zacykli, nedojde ani k chybe, ani volaniu 
 * externeho prerusenia, co sposobi, ze sa "zacykli" aj program, ktory virtualny stroj zavolal.
 * V takom pripade sa nejedna o chybu v emulatore virtualneho stroja. Tento nie je 
 * pisany s ohladom na multivlaknovost, alebo potrebu necakaneho zasahu do behu stroja.
 * @param machine popisovac virtualneho stroja
 * @param limit limit vykonanych instrukcii, kym dojde k navratu z funkcie. Ak je nastaveny na 0, instrukcie sa vykonavaju bez explicitneho limitu
 * @return dovod, pre ktory bol preruseny beh virtualneho stroja VM_OK znamena, ze bol dosiahnuty limit instrukcii a virtualny stroj normalne moze bezat dalej, VM_SOFTINT znamena ziadost o vonkajsie prerusenie, VM_ILLEGAL_OPCODE znamena chybnu instrukciu
 */
static VM_STATE __execVM(VIRTUAL_MACHINE * machine, uint16_t limit) {
	uint8_t interrupt = (limit != 0);
	uint16_t instr;
	uint8_t vm_state;
	char * dis_opcode;
	machine->ext_interrupt = 0;
	while ((limit > 0 || !interrupt) && !machine->ext_interrupt) {
		if ((vm_state = __checkAddressValid(machine, machine->PC)) != VM_OK) {
			return vm_state;
		}
		instr = machine->read_func(machine->memory, machine->PC, MEM_OP_WORD);
		machine->PC += 2;
		do {
			if (IF_IS_ZERO(instr) && !(machine->flags & ZERO_FLAG)) break;
			if (IF_IS_CARRY(instr) && !(machine->flags & CARRY_FLAG)) break;
			if (IF_IS_SIGN(instr) && !(machine->flags & SIGN_FLAG)) break;
			
			if (IS_BRANCH(instr)) {
				instr = GET_LARGEIMMED(instr);
				if (instr & 1) {
					machine->RL = machine->PC;
				}
				if (instr & BIT11) machine->PC -= instr & ~(BIT11 | BIT0);
				else machine->PC += instr & ~(BIT11 | BIT0);
				//				printf("New PC is %04X\n", machine->PC);
			} else if (IS_STORE(instr) || IS_LOAD(instr)) {
				unsigned char addr_reg = GET_ARG1(instr);
				unsigned char data_reg = GET_ARG2(instr);
				if (IS_PRE_DECREMENT(instr)) {
					machine->registers[addr_reg] -= 2;
				}
				vm_state = __checkAddressValid(machine, machine->registers[addr_reg]);
				if (vm_state != VM_OK) return vm_state;
				if (IS_LOAD(instr)) {
					if (IS_8BIT(instr)) machine->registers[data_reg] = machine->read_func(machine->memory, machine->registers[addr_reg], MEM_OP_BYTE);
					else machine->registers[data_reg] = machine->read_func(machine->memory, machine->registers[addr_reg], MEM_OP_WORD);
				}
				if (IS_STORE(instr)) {
					if (IS_8BIT(instr)) machine->write_func(machine->memory, machine->registers[addr_reg], machine->registers[data_reg], MEM_OP_BYTE);
					else machine->write_func(machine->memory, machine->registers[addr_reg], machine->registers[data_reg], MEM_OP_WORD);
				}
				if (IS_POST_INCREMENT(instr)) {
					machine->registers[addr_reg] += 2;
				}
			} else if (IS_ILOAD(instr)) {
				char reg = (instr >> 8) & 7;
				machine->registers[reg] = (machine->registers[reg] << 8) | GET_IMMEDIATE(instr);
			} else if (IS_ALU2(instr)) {
				unsigned char source_reg = GET_ARG2(instr);
				unsigned char dest_reg = GET_ARG1(instr);
				char flags = 0;
				signed long source = machine->registers[source_reg];
				signed long dest = machine->registers[dest_reg];
				switch (OPCODE(instr, 5)) {
					case OP_ADD:	dest += source; break;
					case OP_SUB:	dest -= source; break;
					case OP_MUL:	dest *= source; break;
					case OP_DIV:	dest /= source; break;
					case OP_MOD:	dest %= source; break;
					case OP_AND:	dest &= source; break;
					case OP_OR:		dest |= source; break;
					case OP_XOR:	dest ^= source; break;
				}
				if (GET_OPFLAG(instr)) {
					if (abs(dest) >= 1 << 16) {
						flags |= OVERFLOW_FLAG;
					}
					if (dest < 0) {
						flags |= SIGN_FLAG;
					}
					if (dest == 0) {
						flags |= ZERO_FLAG;
					}
				}
				machine->flags = flags;
				machine->registers[dest_reg] = dest & 0xFFFF;
			} else if (IS_SHIFT(instr)) {
				unsigned char data_reg = GET_ARG1(instr);
				unsigned char shift_amount = GET_ARG2(instr);
				switch (GET_OPFLAG(instr)) {
					case 0:	machine->registers[data_reg] <<= shift_amount; break;
					case 1:	machine->registers[data_reg] >>= shift_amount; break;
				}
			} else if (IS_NOT(instr)) {
				machine->registers[GET_ARG2(instr)] = ~machine->registers[GET_ARG2(instr)];
			} else if (IS_FLINVERT(instr)) {
				machine->flags = machine->flags ^ ~(GET_ARG2(instr) << 4);
			} else if (IS_ADDC(instr) || IS_SUBC(instr)) {
				char flags = 0;
				if (IS_ADDC(instr)) machine->registers[GET_ARG1(instr)] += GET_ARG2(instr);
				else machine->registers[GET_ARG1(instr)] -= GET_ARG2(instr);
				if (GET_OPFLAG(instr)) {
					if (machine->registers[GET_ARG1(instr)] >= 1 << 16) {
						flags |= OVERFLOW_FLAG;
					}
					if (machine->registers[GET_ARG1(instr)] < 0) {
						flags |= SIGN_FLAG;
					}
					if (machine->registers[GET_ARG1(instr)] == 0) {
						flags |= ZERO_FLAG;
					}
					machine->flags = flags;
				}
			} else if (IS_MOV(instr) || IS_SWAP(instr)) {
				unsigned char dest_reg = GET_ARG1(instr);
				unsigned char source_reg = GET_ARG2(instr);
				if (IS_MOV(instr)) {
					machine->registers[dest_reg] = machine->registers[source_reg];
				} else {
					uint16_t tmp = machine->registers[dest_reg];
					machine->registers[dest_reg] = machine->registers[source_reg];
					machine->registers[source_reg] = tmp;
				}
			} else if (IS_INT(instr)) {
				machine->ext_interrupt = GET_IMMEDIATE(instr);
				return VM_SOFTINT;
			} else return VM_ILLEGAL_OPCODE;
		} while (0);
		if (limit > 0) limit--;
	}
	
	return VM_OK;
}

/** Spusti virtualny stroj bez obmedzenia instrukcii.
 * @param machine popisovac virtualneho stroja
 * @return chybovy kod prerusenia behu stroja
 */
VM_STATE runVirtualMachine(VIRTUAL_MACHINE * machine) {
	return __execVM(machine, 0);
}

/** Spusti virtualny stroj bez s obmedzenim poctu emulovanych instrukcii.
 * @note Tato funkcia je iba verejnym rozhranim k funkcii __execVM, ktora nie je viditelna mimo tuto kompilacnu jednotku
 * @param machine popisovac virtualneho stroja
 * @param instructions pocet instrukcii, ktore budu maximalne emulovane v jednom behu 
 * @return chybovy kod prerusenia behu stroja
 */
VM_STATE traceVirtualMachine(VIRTUAL_MACHINE * machine, uint16_t instructions) {
	return __execVM(machine, instructions);
}
