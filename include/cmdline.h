#ifndef __CMDLINE_H__
#define __CMDLINE_H__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NON_POSITIONAL			-1
#define ANY_COUNT				-1

#define true			1
#define false			0

#define OPTIONAL		true
#define MANDATORY		false

#ifdef __cplusplus
extern "C" {
#endif

enum arg_type { ARG_STR, ARG_NUM, ARG_BOOL, ARG_BOOL_INVERSE };

struct cmdline_opts {
	const char * short_opt;
	const char * long_opt;
	const char * arg_name;
	const char * description;
	void * arg_tgt;
	enum arg_type type;
	unsigned char optional;
	int matched;
	int position;
};

struct cmdline_args {
	struct cmdline_opts * options;
	unsigned count;
};

int write_arg(void * data_target, char * argument, enum arg_type type, char * prog_name);
int process_commandline(int argc, char ** argv, struct cmdline_args * options);
void print_help(struct cmdline_args * options, char * prog_name);

#ifdef __cplusplus
}
#endif
	
#endif