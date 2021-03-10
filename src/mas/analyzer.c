#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "analyzer.h"
#include <malloc.h>
#include <stdlib.h>
#include "assembler.h"
#include "object.h"

/// Operacne kody instrukcii
enum OPCODES { ADD = 0, ADDS, SUB, SUBS, MUL, MULS, DIV, DIVS, MOD, MODS, AND, ANDS, OR, ORS, XOR, XORS, MOV, SWAP, SHIFTL, SHIFTR, FLINVERT, NOT, NOTS, INT, LOAD, STORE, ILOAD, BRANCH, BRANCHL, SUBC, SUBCS, ADDC, ADDCS, PUSH, POP, RET, LOAD8, STORE8 };

/// Stringove nazvy operacnych kodov instrukcii
char * opcodes[] = { "ADD", "ADDS", "SUB", "SUBS", "MUL", "MULS", "DIV", "DIVS", "MOD", "MODS", "AND", "ANDS", "OR", "ORS", "XOR", "XORS", "MOV", "SWAP", "SHIFTL", "SHIFTR", "FLINVERT", "NOT", "NOTS", "INT", "LOAD", "STORE", "ILOAD", "BRANCH", "BRANCHL", "SUBC", "SUBCS", "ADDC", "ADDCS", "PUSH", "POP", "RET", "LOAD8", "STORE8" };

/// Stringove nazvy podmienok vykonania instrukcie
char * conditions[] = { "CZ", "CS", "CO" };

/// Stringovy nazov velkosti spracovanych dat
char * data[] = { "WORD" /*, "DWORD" */ };

/// Stringove operatory
char * operators[] = { "[", "]", ":", ";", ",", ".", "\n", "?", "--", "++" };

/// Stringove nazvy registrov
char * registers[] = { "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15", "SP", "LR", "PC" };

/// Stringove nazvy internych stavov analyzatora assembleroveho vstupu
char * states[] = {"STATE_BEGIN", "STATE_LABELED_BEGIN", "STATE_LABEL", "STATE_ANCHOR", "STATE_DATA", "STATE_DATA_VALUE", "STATE_OPCODE", "STATE_OPCODE_CONDITION", "STATE_ARG", "STATE_ARG_INDIRECT", "STATE_ARG_IMMED", "STATE_EOA", "STATE_EOL", "STATE_JUNK", "STATE_EOF" };

/// Pocitadlo riadkov vstupneho suboru
static int line_counter = 1;

/// Buffer vstupneho riadka, pouziva sa najma pre ucely vypisu chyby
static char parse_line[160];

/** Definicia vlastnosti instrukcii instrukcnej sady
 * Pole obsahuje jeden zaznam pre kazdu znamu instrukciu. Zaznamy su radene v poradi zodpovedajucemu
 * operacnym kodom instrukcii (ich stringove nazvy v komentari).
 * Zaznam pre kazdu instrukciu obsahuje pocet argumentov, tomuto poctu argumentov 
 * zodpovedajuci pocet zaznamov v poli typov argumentov (ci argument je odkaz na 
 * register, alebo nejaka okamzita hodnota).
 */
static struct opcode_properties ISA[] = {
	/* ADD */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* ADDS */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* SUB */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* SUBS */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* MUL */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* MULS */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* DIV */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* DIVS */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* MOD */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* MODS */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* AND */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* ANDS */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* OR */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* ORS */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* XOR */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* XORS */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* MOV */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* SWAP */ { 2, { ARG_REG, ARG_REG }, { 0, 0 } },
	/* SHIFTL */ { 2, { ARG_REG, ARG_IMMEDIATE }, { 0, 4 } },
	/* SHIFTR */ { 2, { ARG_REG, ARG_IMMEDIATE }, { 0, 4 } },
	/* FLINVERT */ { 2, { ARG_IMMEDIATE}, { 4 } },
	/* NOT */ { 1, { ARG_REG, 0, 0 }, { 0 } },
	/* NOTS */ { 1, { ARG_REG, 0, 0 }, { 0 } },
	/* INT */ { 1, { ARG_IMMEDIATE, 0, 0 }, { 8 } },
	/* LOAD */ { 2, { ARG_INDIRECT_MOD, ARG_REG } },
	/* STORE */ { 2, { ARG_INDIRECT_MOD, ARG_REG } },
	/* ILOAD */ { 2, { ARG_REG, ARG_IMMEDIATE ,0}, { 0, 8, 0 } },
	/* BRANCH */ { 1, { ARG_IMMEDIATE }, { 12 } },
	/* BRANCHL */ { 1, { ARG_IMMEDIATE }, { 12 } },
	/* SUBC */ { 2, { ARG_REG, ARG_IMMEDIATE }, { 0, 4 } },
	/* SUBCS */ { 2, { ARG_REG, ARG_IMMEDIATE }, { 0, 4 } },
	/* ADDC */ { 2, { ARG_REG, ARG_IMMEDIATE }, { 0, 4 } },
	/* ADDCS */ { 2, { ARG_REG, ARG_IMMEDIATE }, { 0, 4 } },
	// PSEUDO INSTRUCTIONS
	/* PUSH */ { 1, { ARG_REG }, { 0 } },
	/* POP */ { 1, { ARG_REG }, { 0 } },
	/* RET */ { 0 },
	/* LOAD8 */ { 2, { ARG_INDIRECT, ARG_REG } },
	/* STORE8 */ { 2, { ARG_INDIRECT, ARG_REG } }
};

/** Vypise semanticku chybu a ukonci beh prekladaca
 * @param code navratovy kod s ktorym prekladac skonci
 */
static void semantic_error(int code) {
	fprintf(stderr, "Line %d: %s\n", line_counter, parse_line);
	exit(code);
	return;
}

/** Vypise chybu parsovania vstupu a ukonci beh prekladaca.
 * @param state vnutorny stav, resp. posledny akceptovany token po ktoreho spracovani chyba nastala
 * @param token token, ktory vyvolal chybu
 * @param message dovod vzniku chyby v ludsky citatelnej podobe
 */
static void parse_error(int state, struct input_token * token, char * message) {
	fprintf(stderr, "[%s] Parse error: ", states[state]);
	switch (token->type) {
		case TOKEN_OPCODE: fprintf(stderr, "unexpected OPCODE"); break;
		case TOKEN_CONDITION:  fprintf(stderr, "unexpected CONDITION"); break;
		case TOKEN_VAR_DEF:  fprintf(stderr, "unexpected VAR_DEF"); break;
		case TOKEN_OPERATOR:  fprintf(stderr, "unexpected OPERATOR"); break;
		case TOKEN_STR_CONST:  fprintf(stderr, "unexpected STR_CONST"); break;
		case TOKEN_NUM_CONST:  fprintf(stderr, "unexpected NUM_CONST"); break;
		case TOKEN_REGISTER:  fprintf(stderr, "unexpected REGISTER"); break;
		case TOKEN_LBRACE:  fprintf(stderr, "unexpected LBRACE"); break;
		case TOKEN_RBRACE:  fprintf(stderr, "unexpected RBRACE"); break;
		case TOKEN_COLON:  fprintf(stderr, "unexpected COLON"); break;
		case TOKEN_SEMICOLON:  fprintf(stderr, "unexpected SEMICOLON"); break;
		case TOKEN_COMMA:  fprintf(stderr, "unexpected COMMA"); break;
		case TOKEN_DOT:  fprintf(stderr, "unexpected DOT"); break;
		case TOKEN_EOL:  fprintf(stderr, "unexpected EOL"); break;
		case TOKEN_QUESTION_MARK:  fprintf(stderr, "unexpected QUESTION_MARK"); break;
		case TOKEN_MINUSMINUS:  fprintf(stderr, "unexpected MINUSMINUS"); break;
		case TOKEN_PLUSPLUS:  fprintf(stderr, "unexpected PLUSPLUS"); break;
		case TOKEN_EOF: fprintf(stderr, "unexpected EOF"); break;
		case TOKEN_STR_LITERAL: fprintf(stderr, "unexpected string literal"); break;
		case TOKEN_OP_UNKNOWN: fprintf(stderr, "unexpected unknown operator '%s'", token->str_data); break;
		default:
			fprintf(stderr, "unknown token"); break;
	}
	fprintf(stderr, " %s\n", message);
	semantic_error(1);
}

/** Zisti, ci sa token (retazec znakov) nachadza v poli retazcov znakov.
 * @param token hladany retazec
 * @param list pole pripustnych retazcov
 * @param list_size velkost pola
 */
int match_token(char * token, char ** list, unsigned list_size) {
	int q = 0;
	for (q = 0; q < list_size; q++) {
		if (strcmp(token, list[q]) == 0) {
			return q;
		}
	}
	return -1;
}

/** Nacita zo vstupneho suboru prave jeden token.
 * @param in_file vstupny subor
 * @param token pointer na strukturu, kam bude popis nacitaneho tokena ulozeny
 * @return chybovy kod operacie, 1 znamena ziadnu chybu
 */
int read_token(int in_file, struct input_token * token) {
	char c;
	
	static char latch_c = 0;
	static int line_offset = -1;
	int rs;
	char opbuffer[4];
	char buffer[64];
	unsigned cursor = 0;
	unsigned opcursor = 0;
	unsigned in_quotes = 0;
	
	if (line_offset == -1) {
		memset(parse_line, 0, sizeof(parse_line));
		line_offset = 0;
		line_counter++;
	}
	
	memset(buffer, 0, sizeof(buffer));
	memset(opbuffer, 0, sizeof(opbuffer));
	do {
		if (latch_c != 0) {
// 			printf("reading '%c' (latched)\n", latch_c);
			c = latch_c;
			latch_c = 0;
			rs = 1;
		} else {
			rs = read(in_file, &c, 1);
			parse_line[line_offset++] = c;
// 			printf("reading '%c'\n", c);
		}		
		if (rs == 1) {
			if (in_quotes) {
				if (c != '"') {
					buffer[cursor++] = c;
				} else {
					token->type = TOKEN_STR_CONST;
					token->str_data = strdup(buffer);
				}
			} else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '.')) {		// bodka je tiez pre ucely assemblera alfanumericky znak
// 				printf("gotcha alphanumeric char\n");
				if (opcursor > 0) {
// 					printf("operand buffer non empty and i've got alphanumeric char. something is wrong!\n");
					// ak ideme nieco zapisovat do buffera tokenu, ale buffer operatora je neprazdny, tak sa operator nenasiel
					token->type = TOKEN_OP_UNKNOWN;
					token->str_data = strdup(opbuffer);
					return 1;
				}
				buffer[cursor++] = c;
			} else if (c != ' ' && c != '\t') {
//  				printf("gotcha operator char\n");
				if (cursor != 0) {
					latch_c = c;
// 					printf("got something in token buffer, processing\n");
					break;					// nacitali sme znak, ktory by mohol byt operatorom, ale v bufferi pre token nieco je, takze to spracujeme
				}
				
				opbuffer[opcursor++] = c;
				
//  				printf("Testing '%s' if it's operator...", opbuffer);
				rs = match_token(opbuffer, operators, sizeof(operators)/sizeof(char *));
				if (rs != -1) {
//  					printf("OKAY\n");
					token->type = TOKEN_LBRACE + rs;
					if (token->type == TOKEN_EOL) line_offset = -1;
					return 1;
				} else {
//  					printf("NOPE\n");
				}
			} else {
//  				printf("gotcha whitespace char\n");
				if (cursor == 0 && opcursor == 0) {
//  					printf("nothing in buffers, ignoring\n");
					continue;
				} else if (cursor > 0 && opcursor == 0) {
					break;
				} else if (opcursor > 0 && cursor == 0) {
					token->type = TOKEN_OP_UNKNOWN;
					token->str_data = strdup(opbuffer);
					return 1;
				}
			}
		} else {
			if (opcursor > 0) {
				token->type = TOKEN_OP_UNKNOWN;
				token->str_data = strdup(opbuffer);
				return 1;
			} else if (cursor > 0) {
				break;
			} else {
				token->type = TOKEN_EOF;
				return 1;
			}
		}
	} while (1);
	
//  	printf("processing '%s'...\n", buffer);
	
	rs = match_token(buffer, opcodes, sizeof(opcodes)/sizeof(char *));
	if (rs != -1) {
		token->type = TOKEN_OPCODE;
		token->int_data = rs;
		return 1;
	}
	rs = match_token(buffer, conditions, sizeof(conditions)/sizeof(char *));
	if (rs != -1) {
		token->type = TOKEN_CONDITION;
		token->int_data = rs;
		return 1;
	}
	rs = match_token(buffer, data, sizeof(data)/sizeof(char *));
	if (rs != -1) {
		token->type = TOKEN_VAR_DEF;
		token->int_data = rs;
		return 1;
	}
	rs = match_token(buffer, registers, sizeof(registers)/sizeof(char *));
	if (rs != -1) {
//  		printf("its register!\n");
		token->type = TOKEN_REGISTER;
		if (rs > 15) rs = rs - (sizeof(registers)/sizeof(char *) - 16);			// cokolvek, co precnieva za 16 definicii registrov su aliasy na posledne registre (momentalne R13, R14, R15)
		token->int_data = rs;
		return 1;
	}
	char * endchr;
	token->type = TOKEN_NUM_CONST;
	token->int_data = strtol(buffer, &endchr, 0);
	if (endchr != buffer + strlen(buffer)) {
// 		printf("lets say that it's string constant\n");
		token->str_data = strdup(buffer);
		token->type = TOKEN_STR_LITERAL;
		return 1;
	}
	return 1;
}

/** Vygeneruje globalne unikatny label.
 * Pouziva sa na unifikovanie lokalnych labelov (zacinajucich bodkou). Pred bodku 
 * vlozi nazov najblizsieho skorsieho globalneho labelu (nezacinajuceho bodkou).
 * @param token token z ktoreho ma byt label vytvoreny
 * @return unikatny label
 */
char * compose_label(const char * token) {
	static char * global_label = NULL;
	char * label;
	
	if (token[0] != '.') {
		if (global_label != NULL) {
			free(global_label);
		}
		label = strdup(token);
		global_label = strdup(label);
		printf("Label is '%s'\tGlobal label is '%s'\n", label, global_label);
	} else {
		if (global_label == NULL) {
			fprintf(stderr, "warning: local label '%s' with no preceding global label!\n", token);
			label = strdup(token);
		} else {
			label = malloc(strlen(global_label) + strlen(token) + 1);
			snprintf(label, strlen(global_label) + strlen(token) + 1, "%s%s", global_label, token);
		}
		printf("Label is '%s'\tGlobal label is '%s'\n", label, global_label);
	}
	return label;
}

/** Analyzuje vstupny subor zdrojveho kodu v jazyku assembler.
 * Funkcia cita vstup, na zaklade ktoreho generuje strojovy kod do objektoveho suboru.
 * @param in_file popisovac vstupneho suboru
 * @param object vysledny objektovy subor vzniknuty prekladom zdrojoveho kodu do strojoveho kodu
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
int analyze_input(int in_file, OBJECT * object) {
	int state = STATE_BEGIN;
	struct input_token token;
	int instruction;
	char * label = NULL;
	char * global_label = NULL;
	int cond;
	int current_label;
	int arg[2], argno;
	char reevaluate = 0;
	int indirection = NONE;
	
	SECTION * data_section = section_create(".data");
	SECTION * text_section = section_create(".text");
	
	token.str_data = NULL;
	
	do {
		if (!reevaluate) {
			if (!read_token(in_file, &token)) {
				fprintf(stderr, "read_token() returned nothing!\n");
				exit(2);
			}
		}
		reevaluate = 0;
		printf("[%s] => ", states[state]);
		switch (state) {
			case STATE_ANCHOR:
			case STATE_BEGIN:
				if (token.type == TOKEN_OPCODE) {
					instruction = token.int_data;
					cond = 0;
					argno = 0;
					state = STATE_OPCODE;
				} else if (token.type == TOKEN_STR_LITERAL) {
					if (state == STATE_ANCHOR) {
						parse_error(state, &token, "unexpected label! Expected opcode!");
						exit(1);
					}
					if (label != NULL) free(label);
					label = compose_label(token.str_data);
					state = STATE_LABEL;
				} else if (token.type == TOKEN_VAR_DEF) {
					state = STATE_DATA;
				} else if (token.type == TOKEN_SEMICOLON) {
					state = STATE_JUNK;
				} else if (token.type == TOKEN_EOF) {
					state = STATE_EOF;
				} else if (token.type == TOKEN_EOL) {
					break;
				}else parse_error(state, &token, "! Expected opcode, label, variable, beggining of comment, line break or end of file!");
				break;
				
			case STATE_LABEL:
				if (token.type == TOKEN_COLON) {
					symbol_set(text_section, label, section_get_next_address(text_section), 0);
					state = STATE_ANCHOR;
				} else if (token.type == TOKEN_VAR_DEF) {
					symbol_set(data_section, label, section_get_next_address(data_section), 0);
					state = STATE_DATA;
				} else parse_error(state, &token, "! Expected : or data definition!");
				break;
				
			case STATE_DATA:
				if (token.type == TOKEN_NUM_CONST) {
					unsigned char data[2];
					data[0] = (token.int_data >> 8) & 0xFF;
					data[1] = token.int_data & 0xFF;
					section_append_data(data_section, data, 2);
					// write data to .data section
					state = STATE_DATA_VALUE;
				} else if (token.type == TOKEN_STR_CONST) {
					// write string data to .data section
					fprintf(stderr, "using of string literals not implemented yet\n");
					semantic_error(3);
					state = STATE_DATA_VALUE;
				} else if (token.type == TOKEN_QUESTION_MARK) {
					uint16_t foo;
					state = STATE_DATA_VALUE;
					// write 0x0000 to .data section
					section_append_data(data_section, (unsigned char *) &foo, sizeof(foo));
				} else parse_error(state, &token, "Expected numeric constant, string constant or '?'!");
				break;
				
			case STATE_DATA_VALUE:
				if (token.type == TOKEN_COMMA) {
					state = STATE_DATA;
				} else if (token.type == TOKEN_EOL) {
					state = STATE_BEGIN;
				} else if (token.type == TOKEN_SEMICOLON) {
					state = STATE_JUNK;
				} else parse_error(state, &token, "Expected `,', end of line or semicolon!");
				break;
				
			case STATE_OPCODE:
				if (token.type == TOKEN_CONDITION) {
					cond = token.int_data;
					argno = 0;
					state = STATE_ARG;
				} else if (token.type == TOKEN_REGISTER || token.type == TOKEN_REGISTER || token.type == TOKEN_LBRACE || token.type == TOKEN_NUM_CONST || token.type == TOKEN_STR_CONST || token.type == TOKEN_STR_LITERAL) {
					argno = 0;
					state = STATE_ARG;
					reevaluate = 1;
					continue;
				} else if (token.type == TOKEN_SEMICOLON || token.type == TOKEN_EOL || token.type == TOKEN_EOF) {
					argno = 0;
					state = STATE_EOA;
					reevaluate = 1;
				} else parse_error(state, &token, "Expected condition, register, [, numeric constant or label reference!");
				break;
				
			case STATE_ARG:
				if (token.type == TOKEN_REGISTER) {
					if (ISA[instruction].argument_type[argno] == ARG_REG) {
						arg[argno++] = token.int_data;
						state = STATE_EOA;
					} else {
						fprintf(stderr, "Instruction %s does not support direct register as it's %s operand!\n", opcodes[instruction], (argno == 0 ? "first" : "second"));
						exit(1);
					}
				} else if (token.type == TOKEN_LBRACE) {
					if ((ISA[instruction].argument_type[argno] == ARG_INDIRECT) || (ISA[instruction].argument_type[argno] == ARG_INDIRECT_MOD)) {
						state = STATE_ARG_INDIRECT;
						arg[argno] = 0xFF;					// happy debugging
						indirection = NORMAL;
					} else {
						fprintf(stderr, "Instruction %s does not support indirection for argument %d!\n", opcodes[instruction], argno+1);
						exit(1);
					}
				} else if (token.type == TOKEN_NUM_CONST) {
					if (ISA[instruction].argument_type[argno] == ARG_IMMEDIATE) {
						arg[argno++] = token.int_data;
						state = STATE_EOA;
					} else {
						printf("Instruction %s does not support immediate for %d%s operand!\n", opcodes[instruction], argno, (argno == 1 ? "st" : "nd"));
						exit(1);
					}
				} else if (token.type == TOKEN_STR_CONST) {
					if (strlen(token.str_data) > 2) {
						fprintf(stderr, "String constant too long. You can use only 16bit wide string constants!\n");
						semantic_error(1);
					}
					arg[argno] = 0;
					// can be made better way but i'm lazy
					if (strlen(token.str_data) > 0) {
						arg[argno] = token.str_data[0];
						if (strlen(token.str_data) > 1) {
							arg[argno] = arg[argno] << 8 | token.str_data[1];
						}
					}
					argno++;
					state = STATE_EOA;
				} else if (token.type == TOKEN_STR_LITERAL) {
					if (ISA[instruction].argument_type[argno] == ARG_IMMEDIATE) {
						ADDRESS current_address = section_get_next_address(text_section);
						char * token_symbol = compose_label(token.str_data);
						symbol_add_relocation(text_section, token_symbol, current_address, current_address + 2, 8, 3, 3);
						symbol_add_relocation(text_section, token_symbol, current_address + 1, current_address + 2, 0, 8, 0xFF);
						free(token_symbol);
						state = STATE_EOA;
						argno++;
					}
				}else parse_error(state, &token, "! Expected register definition, [, numeric constant or label reference!");
				break;
				
			case STATE_ARG_INDIRECT:
				if (token.type == TOKEN_MINUSMINUS) {
					if (arg[argno] == 0xFF) {
						if (ISA[instruction].argument_type[argno] == ARG_INDIRECT_MOD) {
							indirection = PRE_DECREMENT;
						} else {
							fprintf(stderr, "Instruction %s does not support indirect manipulation (pre decrement or post increment)\n", opcodes[instruction]);
							semantic_error(1);
						}
					} else {
						fprintf(stderr, "Post decrement not supported while indirect register usage!\n");
						semantic_error(1);
					}
				} else if (token.type == TOKEN_PLUSPLUS) {
					if (arg[argno] != 0xFF && indirection == NORMAL) {
						if (ISA[instruction].argument_type[argno] == ARG_INDIRECT_MOD) {
							printf("*** POST INCREMENTING ****\n");
							indirection = POST_INCREMENT;
						} else {
							fprintf(stderr, "Instruction %s does not support indirect manipulation (pre decrement or post increment)\n", opcodes[instruction]);
							semantic_error(1);
						}
					} else if (indirection == PRE_DECREMENT) {
						fprintf(stderr, "Cannot combine pre decrement and post increment in one indirect register usage!\n");
						semantic_error(1);
					} else {
						fprintf(stderr, "Pre increment not supported while indirect register usage!\n");
						semantic_error(1);
					}
				} else if (token.type == TOKEN_REGISTER) {
					if (arg[argno] == 0xFF) {
						arg[argno] = token.int_data;
					} else {
						fprintf(stderr, "You can reference only one register at a time!\n");
						semantic_error(1);
					}
				} else if (token.type == TOKEN_RBRACE) {
					if (arg[argno] != 0xFF) {
						state = STATE_EOA;
						argno++;
					} else {
						fprintf(stderr, "You have to reference at least one register in indirect operation!\n");
					}
				} else parse_error(state, &token, "! Expected --, ++, register specification or ]!");
				break;
				
			case STATE_EOA:
				if (token.type == TOKEN_COMMA) {
					if (ISA[instruction].argument_count == argno) {
						fprintf(stderr, "Too many operands! Instruction %s takes only %d operand%s.\n", opcodes[instruction], ISA[instruction].argument_count, (ISA[instruction].argument_count == 1 ? "" : "s"));
						semantic_error(1);
					}
					state = STATE_ARG;
				} else if (token.type == TOKEN_SEMICOLON || token.type == TOKEN_EOL || token.type == TOKEN_EOF) {
					if (argno != ISA[instruction].argument_count) {
						fprintf(stderr, "Instruction %s takes %d operand%s. %d given!\n", opcodes[instruction], ISA[instruction].argument_count, (ISA[instruction].argument_count == 1 ? "" : "s"), argno);
						semantic_error(1);
					} else {
						switch (token.type) {
							case TOKEN_SEMICOLON:	state = STATE_JUNK; break;
							case TOKEN_EOL: state = STATE_BEGIN; break;
							case TOKEN_EOF: state = STATE_EOF; break;
						}
						INSTRUCTION i;
						unsigned char bytecode[2];
						// generovanie prelozenej instrukcie
						switch (instruction) {
							case ADD:
							case ADDS:
								i = createADD(cond, (instruction == ADDS ? 1 : 0), arg[0], arg[1]);
								break;
								
							case SUB:
							case SUBS:
								i = createSUB(cond, (instruction == SUBS ? 1 : 0), arg[0], arg[1]);
								break;
								
							case MUL:
							case MULS:
								i = createMUL(cond, (instruction == MULS ? 1 : 0), arg[0], arg[1]);
								break;
								
							case DIV:
							case DIVS:
								i = createDIV(cond, (instruction == DIVS ? 1 : 0), arg[0], arg[1]);
								break;
								
							case MOD:
							case MODS:
								i = createMOD(cond, (instruction == MODS ? 1 : 0), arg[0], arg[1]);
								break;
								
							case AND:
							case ANDS:
								i = createAND(cond, (instruction == ANDS ? 1 : 0), arg[0], arg[1]);
								break;
								
							case OR:
							case ORS:
								i = createOR(cond, (instruction == ORS ? 1 : 0), arg[0], arg[1]);
								break;
								
							case XOR:
							case XORS:
								i = createXOR(cond, (instruction == XORS ? 1 : 0), arg[0], arg[1]);
								break;
								
							case NOT:
							case NOTS:
								i = createXOR(cond, (instruction == NOTS ? 1 : 0), arg[0], arg[1]);
								break;
								
							case ADDC:
							case ADDCS:
								i = createADDC(cond, (instruction == ADDCS ? 1 : 0), arg[0], arg[1]);
								break;
								
							case SUBC:
							case SUBCS:
								i = createSUBC(cond, (instruction == SUBCS ? 1 : 0), arg[0], arg[1]);
								break;
								
							case SHIFTL:
								i = createSHIFT(cond, arg[0], arg[1]);
								break;
								
							case SHIFTR:
								i = createSHIFT(cond, arg[0], -arg[1]);
								break;
								
							case MOV:
								i = createMOV(cond, arg[0], arg[1]);
								break;
								
							case SWAP:
								i = createSWAP(cond, arg[0], arg[1]);
								break;
								
							case INT:
								i = createINT(cond, arg[0]);
								break;
								
							case LOAD:
								i = createLOAD(cond, indirection, arg[0], arg[1]);
								break;
								
							case STORE:
								i = createSTORE(cond, indirection, arg[0], arg[1]);
								break;
								
							case ILOAD:
								i = createILOAD(cond, arg[0], arg[1]);
								break;
								
							case BRANCH:
								i = createBRANCH(cond, 0, 0);
								break;
								
							case BRANCHL:
								i = createBRANCH(cond, 0, 1);
								break;
								
/* PSEUDO INSTRUCTIONS
 * They translate into regular instructions from above with contrained semantics.
 * You can reach their effect with instructions above. These are here just for 
 * convenience.
 */
							case PUSH:
								i = createSTORE(cond, PRE_DECREMENT, 13, arg[0]);
								break;
								
							case POP:
								i = createLOAD(cond, POST_INCREMENT, 13, arg[0]);
								break;
							
							case RET:
								i = createMOV(cond, 14, 15);
								break;
							
							case LOAD8:
								i = createLOAD(cond, REF_8_BIT, arg[0], arg[1]);
								break;
								
							case STORE8:
								i = createSTORE(cond, REF_8_BIT, arg[0], arg[1]);
								break;
								
							default:
								fprintf(stderr, "internal ERROR: Unhandled instruction %s!\n", opcodes[instruction]);
								semantic_error(1);
						}
						bytecode[0] = (i >> 8) & 0xFF;
						bytecode[1] = i & 0xFF;
						printf("(appending instruction) => ");
						section_append_data(text_section, bytecode, 2);
					}
				} else parse_error(state, &token, "Expected ',' or ';' or end of line or end of file!");
				break;
				
			case STATE_EOL:
				if (token.type == TOKEN_SEMICOLON) {
					state = STATE_JUNK;
				} else if (token.type == TOKEN_EOL) {
					state = STATE_BEGIN;
				} else parse_error(state, &token, "Expected ';' or end of line!");
				break;
				
			case STATE_JUNK:
				if (token.type == TOKEN_EOL) {
					state = STATE_BEGIN;
				}
				break;
		}
		printf("[%s]\n", states[state]);
		if (token.str_data != NULL) {
			free(token.str_data);
			token.str_data = NULL;
		}
	} while (token.type != TOKEN_EOF);
	object_add_section(object, data_section);
	object_add_section(object, text_section);
	return 0;
}
