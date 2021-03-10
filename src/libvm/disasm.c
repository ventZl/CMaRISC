#include <instruction.h>
#include <string.h>
#include <stdio.h>

/** Disassembluje podmienku pri ktorej sa instrukcia vykona
 * @param opcode operacny kod instrukcie
 * @return retazec reprezentujuci podmienku
 */
static char * test_cond(unsigned short opcode) {
	if (IF_IS_ZERO(opcode)) return "CZ ";
	if (IF_IS_CARRY(opcode)) return "CO ";
	if (IF_IS_SIGN(opcode)) return "CS ";
	return "";
}

/** Disassembluje instrukciu.
 * Instrukcie virtualneho stroja maju pevnu dlzku 16 bitov. Tato funkcia vrati stringovu
 * reprezentaciu instrukcie. Uvolnenie pamate alokovanej touto funkciou je na zodpovednosti volajuceho kodu.
 * @param opcode operacny kod instrukcie
 * @return textova reprezentacia instrukcie v jazyku Assembler
 */
char * disassemble(unsigned short opcode) {
	char disbuf[64];
	char numbuf[32];
	
	memset(disbuf, 0, sizeof(disbuf));
	memset(numbuf, 0, sizeof(numbuf));
	
	if (IS_BRANCH(opcode)) {
		unsigned short reladdr = GET_LARGEIMMED(opcode);
		if (opcode & 1 != 0) strcat(disbuf, "BRANCHL "); else strcat(disbuf, "BRANCH ");
		strcat(disbuf, test_cond(opcode));
		if (reladdr & BIT11) strcat(disbuf, "-");
		sprintf(numbuf, "%u", reladdr & (~(1 | (1 << 11))));
		strcat(disbuf, numbuf);
	} else if (IS_ADD(opcode) || IS_SUB(opcode) || IS_MUL(opcode) || IS_DIV(opcode) || IS_MOD(opcode) || IS_AND(opcode) || IS_OR(opcode) || IS_XOR(opcode)) {
		if (IS_ADD(opcode)) {
			strcat(disbuf, "ADD");
		} else if (IS_SUB(opcode)) {
			strcat(disbuf, "SUB");
		} else if (IS_MUL(opcode)) {
			strcat(disbuf, "MUL");
		} else if (IS_DIV(opcode)) {
			strcat(disbuf, "DIV");
		} else if (IS_MOD(opcode)) {
			strcat(disbuf, "MOD");
		} else if (IS_AND(opcode)) {
			strcat(disbuf, "AND");
		} else if (IS_OR(opcode)) {
			strcat(disbuf, "OR");
		} else if (IS_XOR(opcode)) {
			strcat(disbuf, "XOR");
		}
		if ((opcode >> 8) & 1) {
			strcat(disbuf, "S ");
		} else strcat(disbuf, " ");
		strcat(disbuf, test_cond(opcode));
		sprintf(numbuf, "R%d, R%d", GET_ARG1(opcode), GET_ARG2(opcode));
		strcat(disbuf, numbuf);
	} else if (IS_ADDC(opcode) || IS_SUBC(opcode)) {
		if (IS_ADDC(opcode)) {
			strcat(disbuf, "ADDC");
		} else {
			strcat(disbuf, "SUBC");
		}
		if ((opcode >> 8) & 1) {
			strcat(disbuf, "S ");
		} else strcat(disbuf, " ");
		strcat(disbuf, test_cond(opcode));
		sprintf(numbuf, "R%d, %d", GET_ARG1(opcode), GET_ARG2(opcode));
		strcat(disbuf, numbuf);
	} else if (IS_MOV(opcode) || IS_SWAP(opcode)) {
		if (IS_MOV(opcode)) {
			strcat(disbuf, "MOV ");
		} else {
			strcat(disbuf, "SWAP ");
		}
		strcat(disbuf, test_cond(opcode));
		sprintf(numbuf, "R%d, R%d", GET_ARG1(opcode), GET_ARG2(opcode));
		strcat(disbuf, numbuf);
	} else if (IS_LOAD(opcode) || IS_STORE(opcode)) {
		char * arg_tpl;
		if (IS_LOAD(opcode)) {
			strcat(disbuf, "LOAD");
		} else {
			strcat(disbuf, "STORE");
		}
		if (IS_PRE_DECREMENT(opcode)) {
			arg_tpl = "[--R%d], R%d";
			strcat(disbuf, " ");
		} else if (IS_POST_INCREMENT(opcode)) {
			arg_tpl = "[R%d++], R%d";
			strcat(disbuf, " ");
		} else if (IS_8BIT(opcode)) {
			strcat(disbuf, "8 ");
			arg_tpl = "[R%d], R%d";
		} else {
			strcat(disbuf, " ");
			arg_tpl = "[R%d], R%d";
		}
		strcat(disbuf, test_cond(opcode));
		sprintf(numbuf, arg_tpl, GET_ARG1(opcode), GET_ARG2(opcode));
		strcat(disbuf, numbuf);
	} else if (IS_ILOAD(opcode)) {
		strcat(disbuf, "ILOAD ");
		strcat(disbuf, test_cond(opcode));
		sprintf(numbuf, "R%d, %u", (opcode >> 8) & 7, GET_IMMEDIATE(opcode));
		strcat(disbuf, numbuf);
	} else if (IS_NOT(opcode)) {
		strcat(disbuf, "NOT");
		if ((opcode >> 8) & 1) {
			strcat(disbuf, "S ");
		} else strcat(disbuf, " ");
		strcat(disbuf, test_cond(opcode));
		sprintf(numbuf, "R%d", GET_ARG2(opcode));
		strcat(disbuf, numbuf);
	} else if (IS_SHIFT(opcode)) {
		if (GET_OPFLAG(opcode) == 1) {
			strcat(disbuf, "SHIFTR ");
		} else {
			strcat(disbuf, "SHIFTL ");
		}
		strcat(disbuf, test_cond(opcode));
		sprintf(numbuf, "R%d, %d", GET_ARG1(opcode), GET_ARG2(opcode));
		strcat(disbuf, numbuf);
	} else if (IS_FLINVERT(opcode)) {
		strcat(disbuf, "FLINVERT ");
		strcat(disbuf, test_cond(opcode));
		sprintf(numbuf, "%d", GET_ARG2(opcode));
		strcat(disbuf, numbuf);
	} else if (IS_INT(opcode)) {
		strcat(disbuf, "INT ");
		strcat(disbuf, test_cond(opcode));
		sprintf(numbuf, "%d", GET_IMMEDIATE(opcode));
		strcat(disbuf, numbuf);
	} else strcat(disbuf, "(unknown opcode)");
	return strdup(disbuf);
}