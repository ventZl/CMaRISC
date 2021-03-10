#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <instruction.h>
#include "assembler.h"
#include "mas.h"

static int warn_unsafe = 0;

/** Zapne varovanie pri pouziti potencialne nebezpecneho assembleroveho kodu.
 * Jedna sa hlavne o instrukcie, ktore otvorene manipuluju s obsahom registrov
 * viazanych ABI na nejaky konkretny ucel.
 */
void do_warn_unsafe_assembly() {
	warn_unsafe = 1;
}

/** Nasledujuce funkcie generuju assemblerovy kod jednotlivych instrukcii. 
 * Vzhladom k tomu, ze je ich ucel funkcia a argumenty +- rovnaka, uvadzane su pre vsetky funkcie spolocne.
 * Funkcia vzdy vygeneruje bytecode instrukcie, ktora je v nazve funkcie (napr. instrukcia createMOV generuje
 * bytecode instrukcie MOV.
 * Argumenty funkcii maju nasledovny vyznam:
 * @param cond znamena podmienku pri ktorej sa ma instrukcia vykonat, inac funguje ako instrukcia NOP
 * @param source cislo zdrojoveho registra (0-15) - z tohto registra sa berie zdrojovy argument operacie
 * @param destination cislo cieloveho registra - do tohto registra sa uklada vysledok operacie, pripadne aj zdrojovy argument operacie, ak sa jedna o binarnu operaciu (napr. scitanie)
 * @param vector (iba instrukcia INT) urcuje ID externeho prerusenia, ktore je vyvolane
 * @param flag (iba pre aritmeticko logicke instrukcie) ak je nastavene na 1, instrukcia zmeni hodnoty priznakov v registri flags, inac zostanu nedotknute
 * @param relative_address (iba instrukcia BRANCH) znamienkove cislo +- 128 bytov, ktore bude pricitane k hodnote PC registra.
 * @param link (iba instrukcia BRANCH ) ak je nastaveny na 1, vygeneruje sa instrukcia, ktora aktualnu hodnotu registra PC ulozi do link registra
 * @param reg cislo registra (pre unarne operacie, ktore pracuju iba s jednym registrom)
 * @param type typ nepriameho pouzitia hodnoty
 */

INSTRUCTION createMOV(CONDITION cond, REGISTER source, REGISTER destination) {
	if (destination == 15 && source != 14 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: MOV Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_MOV | SET_ARG1(source) | SET_ARG2(destination);
	
}

INSTRUCTION createSWAP(CONDITION cond, REGISTER source, REGISTER destination) {
	if (destination == 15 && source != 14 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: MOV Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_SWAP | SET_ARG1(source) | SET_ARG2(destination);
	
}

INSTRUCTION createINT(CONDITION cond, unsigned char vector) {
	return (cond << 14) | MK_INT | SET_IMMEDIATE(vector);
	
}

INSTRUCTION createADD(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination) {
	if (destination == 15 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: ADD Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_ADD | (flag != 0 ? 1 << 8 : 0) | SET_ARG1(source) | SET_ARG2(destination);
}

INSTRUCTION createSUB(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination) {
	if (destination == 15 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: SUB Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_SUB | (flag != 0 ? 1 << 8 : 0) | SET_ARG1(source) | SET_ARG2(destination);
}

INSTRUCTION createADDC(CONDITION cond, unsigned char flag, REGISTER source, char immediate) {
	if (source == 15 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: ADD Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_ADDC | (flag != 0 ? 1 << 8 : 0) | SET_ARG1(source) | SET_ARG2(immediate);
}

INSTRUCTION createSUBC(CONDITION cond, unsigned char flag, REGISTER source, char immediate) {
	if (source == 15 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: SUB Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_SUBC | (flag != 0 ? 1 << 8 : 0) | SET_ARG1(source) | SET_ARG2(immediate);
}

INSTRUCTION createMUL(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination) {
	if (destination == 15 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: MUL Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_MUL | (flag != 0 ? 1 << 8 : 0) | SET_ARG1(source) | SET_ARG2(destination);
}

INSTRUCTION createDIV(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination) {
	if (destination == 15 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: DIV Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_DIV | (flag != 0 ? 1 << 8 : 0) | SET_ARG1(source) | SET_ARG2(destination);
}

INSTRUCTION createMOD(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination) {
	if (destination == 15 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: MOD Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_MOD | (flag != 0 ? 1 << 8 : 0) | SET_ARG1(source) | SET_ARG2(destination);
}

INSTRUCTION createAND(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination) {
	if (destination == 15 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: AND Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_AND | (flag != 0 ? 1 << 8 : 0) | SET_ARG1(source) | SET_ARG2(destination);
}

INSTRUCTION createOR(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination) {
	if (destination == 15 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: OR Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_OR | (flag != 0 ? 1 << 8 : 0) | SET_ARG1(source) | SET_ARG2(destination);
}

INSTRUCTION createXOR(CONDITION cond, unsigned char flag, REGISTER source, REGISTER destination) {
	if (destination == 15 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: XOR Reg%d, PC\n", source);
	}
	return (cond << 14) | MK_XOR | (flag != 0 ? 1 << 8 : 0) | SET_ARG1(source) | SET_ARG2(destination);
}

INSTRUCTION createNOT(CONDITION cond, unsigned char flag, REGISTER reg) {
	if (reg == 15 && warn_unsafe) {
		fprintf(stderr, "WARNING: Potentially unsafe instruction: NOT PC\n");
	}
	return (cond << 14) | MK_NOT | (flag != 0 ? 1 << 4 : 0) | SET_ARG2(reg);
}

INSTRUCTION createBRANCH(CONDITION cond, short relative_address, char link) {
	if ((link & ~1) != 0) {
		fprintf(stderr, "ERROR: Invalid branch link specification!\n");
		exit(1);
	}
	if (abs(relative_address) > 0x7FF) {
		fprintf(stderr, "ERROR: Branch target outside of relative jump area!\n");
		exit(1);
	}
	if ((relative_address & 1) != 0) {
		fprintf(stderr, "ERROR: Odd target address. Cannot jump to unaligned address!\n");
		exit(1);
	}
	return (cond << 14) | MK_BRANCH | (abs(relative_address) & 0x7FF) | (abs(relative_address) != relative_address ? 1 << 11 : 0) | (link & 1);
}

INSTRUCTION createLOAD(CONDITION cond, INDIRECTION type, REGISTER ref, REGISTER reg) {
	if (type > 3) {
		fprintf(stderr, "ERROR: Invalid indirection behavior specified (%d)!\n", type);
		exit(1);
	} else {
		fprintf(stderr, "LOAD with indirection = %d\n", type);
	}
	return (cond << 14) | (((type) & 0x3) << 8) | SET_ARG1(ref) | SET_ARG2(reg);
}

INSTRUCTION createILOAD(CONDITION cond, REGISTER dest, unsigned char immediate) {
	if (dest > 7) {
		fprintf(stderr, "ERROR: You can't load immediate into reigster higher than R7!\n");
		exit(1);
	}
	return (cond << 14) | (1 << 11) | (dest & 0x3) << 8 | SET_IMMEDIATE(immediate);
}

INSTRUCTION createSTORE(CONDITION cond, INDIRECTION type, REGISTER ref, REGISTER reg) {
	if (type > 3) {
		fprintf(stderr, "ERROR: Invalid indirection behavior specified (%d)!\n", type);
		exit(1);
	} else {
		fprintf(stderr, "STORE with indirection = %d\n", type);
	}
	return (cond << 14) | MK_STORE | (((type) & 0x3) << 10) | SET_ARG1(ref) | SET_ARG2(reg);
}

INSTRUCTION createSHIFT(CONDITION cond, REGISTER reg, signed amount) {
	if (abs(amount) > 15) {
		fprintf(stderr, "ERROR: Unable to shift by more than 15 bits!\n");
		exit(1);
	}
	return (cond << 14) | MK_SHIFT | (abs(amount) != amount ? 1 << 8 : 0) | SET_ARG1(reg) | SET_ARG2(abs(amount));
}
