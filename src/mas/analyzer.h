#ifndef __SUNBLIND_MAS_ANALYZER_H__
#define __SUNBLIND_MAS_ANALYZER_H__

#include "object.h"

enum tokens { TOKEN_OPCODE, TOKEN_CONDITION, TOKEN_VAR_DEF, TOKEN_OPERATOR, TOKEN_STR_CONST, TOKEN_STR_LITERAL, 
				TOKEN_NUM_CONST, TOKEN_REGISTER, TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_COLON, 
				TOKEN_SEMICOLON, TOKEN_COMMA, TOKEN_DOT, TOKEN_EOL, TOKEN_QUESTION_MARK, TOKEN_MINUSMINUS, TOKEN_PLUSPLUS, TOKEN_EOF, TOKEN_OP_UNKNOWN };

enum state { STATE_BEGIN, STATE_LABELED_BEGIN, STATE_LABEL, STATE_ANCHOR, STATE_DATA, STATE_DATA_VALUE, STATE_OPCODE, STATE_OPCODE_CONDITION, STATE_ARG, STATE_ARG_INDIRECT, STATE_ARG_IMMED, STATE_EOA /* END OF ARGUMENT */, STATE_EOL, STATE_JUNK, STATE_EOF };
				
struct input_token {
	int type;
	char * str_data;
	int int_data;
};

enum arg_type_options { ARG_REG, ARG_INDIRECT, ARG_INDIRECT_MOD, ARG_IMMEDIATE };

struct opcode_properties {
	int argument_count;
	char argument_type[3];
	char argument_size[3];
};

int read_token(int in_file, struct input_token * token);
int analyze_input(int in_file, OBJECT * object);

#endif
