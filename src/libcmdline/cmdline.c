#include <stdlib.h>
#include <unistd.h>

#include <cmdline.h>

/* Externy kod dodany Eduardom Drusom z jeho internych archivov zdrojovych kodov */

int write_arg(void* data_target, char* argument, enum arg_type type, char* prog_name) {
	char * atoi_check = NULL;
	switch (type) {
		case ARG_STR:
			*((char **) data_target) = argument;
			break;
			
		case ARG_NUM:
			
			if ((strncmp(argument, "0x", 2) * strncmp(argument, "0X", 2)) == 0) {
				*((long *) data_target) = strtol(argument, &atoi_check, 16);
			} else {
				*((long *) data_target) = strtol(argument, &atoi_check, 10);
			}
			if (atoi_check == argument) {
				fprintf(stderr, "%s: \"%s\" is not valid number\n", prog_name, argument);
				return -2;
			}
			break;
			
		case ARG_BOOL:
			if ((strcmp(argument, "yes") * strcmp(argument, "true")) == 0) {
				*((long *) data_target) = 1;
			} else {
				*((long *) data_target) = 0;
			}
			break;
			
		case ARG_BOOL_INVERSE:
			if ((strcmp(argument, "yes") * strcmp(argument, "true")) == 0) {
				*((long *) data_target) = 0;
			} else {
				*((long *) data_target) = 1;
			}
	}
	return 0;
}

int process_commandline(int argc, char ** argv, struct cmdline_args * options) {
	int q, w;
	unsigned char option_parsed = false;
	int write_retval;
	unsigned positional_current = 0;
	for (q = 0; q < argc; q++) {
		option_parsed = false;
		for (w = 0; w < options->count; w++) {
			if (options->options[w].short_opt != NULL && options->options[w].long_opt != NULL) {
				if ((strcmp(argv[q], options->options[w].short_opt) * strcmp(argv[q], options->options[w].long_opt)) == 0) {
					if (options->options[w].arg_name != NULL) {
						if (argc <= q + 1) {
							fprintf(stderr, "%s: Option %s need an argument but none given!\n", argv[0], argv[q]);
							return -1;
						}
						write_retval = write_arg((void *) options->options[w].arg_tgt, argv[q + 1], options->options[w].type, argv[0]);
						if (write_retval != 0) return write_retval;
						option_parsed = true;
						q += 1;
						options->options[w].matched++;
						break;
					} else {
						if (options->options[w].type == ARG_BOOL) {
							*((long *) options->options[w].arg_tgt) = 1;
							option_parsed = true;
						} else if (options->options[w].type == ARG_BOOL_INVERSE) {
							*((long *) options->options[w].arg_tgt) = 0;
							option_parsed = true;
						}
						options->options[w].matched++;
						break;
					}
				}
			}
		}
		if (!option_parsed) {
			for (w = 0; w < options->count; w++) {
				if (options->options[w].short_opt == NULL && options->options[w].long_opt == NULL) {
					if (positional_current == options->options[w].position) {
						write_retval = write_arg((void *) options->options[w].arg_tgt, argv[q], options->options[w].type, argv[0]);
						options->options[w].matched++;
						if (write_retval != 0) return write_retval;
						break;
					}
					if (options->options[w].position == NON_POSITIONAL && q > 0) {
						char ** tmp_table = (char **) realloc(*((void **) options->options[w].arg_tgt), (options->options[w].matched + 1) * sizeof(void *));
						if (tmp_table != NULL) {
							* ((char ***) options->options[w].arg_tgt) = tmp_table;
							tmp_table[options->options[w].matched] = (char *) options->options[w].matched; //tmp_data;
							write_retval = write_arg(&tmp_table[options->options[w].matched], argv[q], options->options[w].type, argv[0]);
							if (write_retval != 0) return write_retval;
							options->options[w].matched++;
						} else {
							fprintf(stderr, "%s: Out of memory!\n", argv[0]);
							_exit(1);
						}
					}
				}
			}
			positional_current++;
		}
	}
	for (w = 0; w < options->count; w++) {
		if (options->options[w].optional == MANDATORY && options->options[w].matched == 0) {
			printf("Options %d is mandatory and has not been matched\n", w);
			return -3;
		}
	}
	return 0;
}

void print_help(struct cmdline_args * options, char * prog_name) {
	int q, w;
	printf("%s ", prog_name);
	int maxlen = 0;
	int curlen = 0;
	for (q = 0; q < options->count; q++) {
		curlen = 0;
		if (options->options[q].optional) {
			printf("[ ");
		}
		if (options->options[q].short_opt != NULL) {
			printf("(%s | %s) ", options->options[q].short_opt, options->options[q].long_opt);
			curlen += 7 + strlen(options->options[q].short_opt) + strlen(options->options[q].long_opt);
		}
		if (options->options[q].arg_name != NULL) {
			printf("%s ", options->options[q].arg_name);
			curlen += 2 + strlen(options->options[q].arg_name);
		}
		if (options->options[q].optional) printf("] ");
		else printf(" ");
		if (maxlen < curlen) maxlen = curlen;
	}
	printf("\n");
	for (q = 0; q < options->count; q++) {
		curlen = 0;
		if (options->options[q].optional) {
			printf("  %s | %s  ", options->options[q].short_opt, options->options[q].long_opt);
			curlen += strlen(options->options[q].short_opt) + strlen(options->options[q].long_opt) + 7;
		}
		if (options->options[q].arg_name != NULL) {
			printf("%s  ", options->options[q].arg_name);
			curlen += strlen(options->options[q].arg_name) + 2;
		}
		
		for (w = curlen; w < maxlen; w++) printf(" ");
		
		printf("%s\n", options->options[q].description);
	}
	
}