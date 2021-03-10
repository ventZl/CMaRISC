/* Minimalistic Debugger w/ virtual machine or remote connection
 * For C Minimalistic RISC machine
 */

#include <cmdline.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <vm.h>
#include <object.h>
#include <disasm.h>
#include <signal.h>

char * cmdline_remote_id = NULL;
char * cmdline_infile = NULL;
long cmdline_help = 0;
long cmdline_memsize = 65535;
char * cmdline_dump_text_file = NULL;
char * cmdline_dump_data_file = NULL;

VIRTUAL_MACHINE * mach = NULL;

struct cmdline_opts options[] = {
	{ "-r", "--remote", "id", "Remotely connect to controller with name.", (void *) &cmdline_remote_id, ARG_STR, OPTIONAL, 0, NON_POSITIONAL},
	{ "-m", "--memsize", "size", "Set size of device memory (0-65535) [default 65535].", (void *) &cmdline_memsize, ARG_NUM, OPTIONAL, 0, NON_POSITIONAL},
	{ "-h", "--help", NULL, "Show this help", (void *) &cmdline_help, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL},
	{ NULL, NULL, "bin_file", "Virtual memory image file.", &cmdline_infile, ARG_STR, MANDATORY, 0, 1},
};

struct cmdline_args commandline = { options, 4 };

enum p_type { T_NONE, T_NUM, T_STR };

struct dbg_command {
	char * command;
	enum p_type par_type[10];
};

enum cmd_ids { CMD_RUN, CMD_STEP, CMD_QUIT, CMD_DUMP_REGS, CMD_DISASSEBMLE, CMD_SET_REG, CMD_SET_MEM, CMD_DUMP_MEM, CMD_HELP, CMD_COMPUTER, CMD_HUMAN, CMD_AUTOSTAT };

struct dbg_command commands[] = {
	{ "run", { T_NONE }},
	{ "step", { T_NUM }},
	{ "quit", { T_NONE }},
	{ "dump-regs", { T_NONE }},
	{ "disassemble", { T_NUM, T_NUM }},
	{ "set-reg", { T_NUM, T_NUM }},
	{ "set-mem", { T_NUM, T_NUM }},
	{ "dump-mem", { T_NUM, T_NUM }},
	{ "help", { T_STR }},
	{ "computer", { T_NONE }},
	{ "human", { T_NONE }},
	{ "auto-stat", { T_NONE }}
};

#define DBG_CMD_COUNT (sizeof(commands) / sizeof(struct dbg_command))

union cmd_arg {
	int number;
	char * string;
};

struct dbg_runtime_command {
	int command;
	union cmd_arg cmd_argument[10];
};

int parse_command(struct dbg_runtime_command * command, char * input) {
	char * token, * token_save;
	int q;
	token = strtok_r(input, " \n", &token_save);
	
	command->command = -1;
	
	for (q = 0; q < 10; q++) {
		command->cmd_argument[q].number = 0;
	}
	
	for (q = 0; q < DBG_CMD_COUNT; q++) {
		if (strcmp(token, commands[q].command) == 0) {
			command->command = q;
			break;
		}
		if (strncmp(token, commands[q].command, strlen(token)) == 0) {
			if (command->command != -1) return -2;
			command->command = q;
		}
	}
	if (command->command == -1) return -1;
	
	for (q = 0; q < 10; q++) {
		token = strtok_r(NULL, " \n", &token_save);
		if (token == NULL) break;
		switch(commands[command->command].par_type[q]) {
			case T_NUM:
				command->cmd_argument[q].number = strtol(token, NULL, 0);
				break;
				
			case T_STR:
				command->cmd_argument[q].string = strdup(token);
				break;
		}
	}
	return q;
}

void free_command(struct dbg_runtime_command * command) {
	int q;
	for (q = 0; q < 10; q++) {
		if (commands[command->command].par_type[q] == T_STR) free(command->cmd_argument[q].string);
	}
	return;
}

void sigint_handler(int signo) {
	mach->ext_interrupt = 1;
}

int main(int argc, char ** argv) {
	char * memory = NULL; 
	char running = 1;
	ADDRESS entrypoint = 0;
	char comp_out = 0;
	int auto_stat = 0;
	int rc;
	int cmdline_retval = process_commandline(argc, argv, &commandline);
	if (cmdline_help) { print_help(&commandline, argv[0]); return 0; }
	if (cmdline_retval != 0) return cmdline_retval;

	if (cmdline_memsize < 0 || cmdline_memsize > 65535) {
		fprintf(stderr, "error: Invalid memory size %d\n", cmdline_memsize);
		exit(1);
	}
	
	memory = malloc(cmdline_memsize);
	memset(memory, 0, cmdline_memsize);
	
	rc = binary_read(cmdline_infile, memory, &entrypoint, cmdline_memsize);
	
	if (rc == -1) {
		OBJECT * binary_object = object_load(cmdline_infile);
		if (binary_object == NULL) {
			fprintf(stderr, "Unable to load virtual memory image nor as plain binary nor as debuggable binary.\n");
			exit(1);
		}
		SECTION * binary_section = object_get_section_by_name(binary_object, ".binary");
		if (binary_section == NULL) {
			fprintf(stderr, "Invalid virtual memory image. Cannot find image section.\n");
			exit(1);
		}
		if (section_data_copy(binary_section, memory, cmdline_memsize) != 0) {
			fprintf(stderr, "Virtual memory image is too big to fit into debugger memory.");
			exit(1);
		}
		
		entrypoint = symbol_get_address(binary_section, "@@entrypoint");
		if (entrypoint == 0xFFFF) {
			fprintf(stderr, "Unable to find image entrypoint!\n");
			exit(1);
		}
	} else if (rc != 0) {
		exit(1);
	}
	
	mach = createVirtualMachine(memory, cmdline_memsize, entrypoint);

	signal(SIGINT, sigint_handler);
	
	char command[80];
	int arg_count = -1, q;
	
	VM_STATE mach_state;
	struct dbg_runtime_command cmd;
	
	while (running) {
		if (!comp_out) {
			printf("dbg> ");
			fflush(stdout);
		}
		fgets(command, 80, stdin);
		if (strlen(command) > 1) {
			free_command(&cmd);
			arg_count = parse_command(&cmd, command);
		}
		if (arg_count >= 0) {
			switch (cmd.command) {
				case CMD_RUN:
					mach_state = runVirtualMachine(mach);
					if (comp_out) {
						switch (mach_state) {
							case VM_OK: printf("OK\n"); break;
							case VM_ILLEGAL_OPCODE: printf("ILL_OPCODE\n"); break;
							case VM_DIVIDE_BY_ZERO: printf("DIV_BY_ZERO\n"); break;
							case VM_OUT_OF_MEMORY: printf("OUT_OF_MEM\n"); break;
							case VM_SOFTINT: printf("SOFTINT\n"); break;
						}
					}
					break;
					
				case CMD_STEP:
					if (arg_count == 0) {
						mach_state = traceVirtualMachine(mach, 1);
						if (auto_stat) {
							uint16_t instr;
							ADDRESS d_addr = mach->registers[15];
							char * d_str;
							instr = mach->read_func(mach->memory, d_addr, 0);
							d_str = disassemble(instr);
							printf("0x%04X:\t%s\n", d_addr, d_str);
							dumpRegistersVirtualMachine(mach);
							free(d_str);
						}
					} else {
						if (cmd.cmd_argument > 0) mach_state = traceVirtualMachine(mach, cmd.cmd_argument[0].number);
						else {
							if (!comp_out) fprintf(stderr, "invalid step count\n"); else printf("BAD_STEP\n");
							break;
						}
					}
					if (comp_out) {
						switch (mach_state) {
							case VM_OK: printf("OK\n"); break;
							case VM_ILLEGAL_OPCODE: printf("ILL_OPCODE\n"); break;
							case VM_DIVIDE_BY_ZERO: printf("DIV_BY_ZERO\n"); break;
							case VM_OUT_OF_MEMORY: printf("OUT_OF_MEM\n"); break;
							case VM_SOFTINT: printf("SOFTINT\n"); break;
						}
					}
					break;
					
				case CMD_QUIT: 
					running = 0;
					break;
					
				case CMD_DUMP_REGS: 
				{
					if (!comp_out) dumpRegistersVirtualMachine(mach);
					else {
						for (q = 0; q < 16; q++) {
							printf("%d\n", mach->registers[q]);
						}
						printf("OK\n");
					}
					break;
				}
					
				case CMD_DUMP_MEM:
				{
					int dump_count;
					int cursor;
					int addr;
					if (!comp_out) {
						if (arg_count < 1) {
							fprintf(stderr, "not enough arguments\n");
						} else {
							if (arg_count == 1) dump_count = 16;
							else dump_count = cmd.cmd_argument[1].number;
							addr = cmd.cmd_argument[0].number;
							for (cursor = 0; cursor < dump_count; cursor++) {
								if (cursor % 8 == 0) printf("\n0x%04X:", addr + cursor);
								if (cursor % 4 == 0) printf(" ");
								printf("%02X", mach->memory[addr + cursor]);
							}
						}
					} else {
						if (arg_count < 2) {
							printf("BAD_COUNT\n");
						} else {
							dump_count = cmd.cmd_argument[1].number;
							addr = cmd.cmd_argument[0].number;
							for (cursor = 0; cursor < dump_count; cursor++) {
								printf("%02X\n", mach->memory[addr + cursor]);
							}
							printf("OK\n");
						}
					}
					break;
				}
				case CMD_DISASSEBMLE:
				{
					ADDRESS d_addr;
					int d_count;
					char * d_str;
					unsigned short instr;
					
					if (arg_count == 0) {
						d_addr = mach->registers[15];
						d_count = 10;
					} else if (arg_count == 1) {
						d_addr = mach->registers[15];
						d_count = cmd.cmd_argument[0].number;
					} else if (arg_count == 2) {
						if (cmd.cmd_argument[0].number >= 0 && cmd.cmd_argument[0].number < 0x10000) d_addr = cmd.cmd_argument[0].number;
						else {
							if (!comp_out) fprintf(stderr, "invalid address 0x%X\n", cmd.cmd_argument[0].number); else printf("BAD_ADDR\n");
							break;
						}
						if (cmd.cmd_argument[1].number > 0) d_count = cmd.cmd_argument[1].number;
						else {
							if (!comp_out) fprintf(stderr, "invalid count %d\n", cmd.cmd_argument[1].number); else printf("BAD_COUNT\n");
							break;
						}
					}
					for (q = 0; q < d_count; q++) {
						instr = mach->read_func(mach->memory, d_addr, 0);
						d_str = disassemble(instr);
						if (!comp_out) printf("0x%04X:\t%s\n", d_addr, d_str);
						else printf("%s\n", d_str);
						free(d_str);
						d_addr += 2;
					}
					if (comp_out) printf("OK\n");
					break;
				}	
				case CMD_SET_REG:
					if (arg_count != 2) {
						if (!comp_out) fprintf(stderr, "set-reg reg-no value\n"); else printf("BAD_ARG\n");
						break;
					} else {
						if (cmd.cmd_argument[0].number >= 0 && cmd.cmd_argument[0].number <= 15) {
							mach->registers[cmd.cmd_argument[0].number] = cmd.cmd_argument[1].number;
							printf("OK\n");
						} else {
							if (!comp_out) fprintf(stderr, "error: invalid register number %d\n", cmd.cmd_argument[0].number); else printf("BAD_REG\n");
						}
					}
					break;
					
				case CMD_SET_MEM:
					if (arg_count != 2) {
						if (!comp_out) fprintf(stderr, "set-mem addr value\n"); else printf("BAD_ARG\n");
						break;
					} else {
						if (cmd.cmd_argument[0].number >= 0 && cmd.cmd_argument[0].number < 0x10000) {
							mach->write_func(mach->memory, cmd.cmd_argument[0].number, cmd.cmd_argument[1].number, 0);
							printf("OK\n");
						} else {
							if (!comp_out) fprintf(stderr, "error: invalid address %d\n", cmd.cmd_argument[0].number); else printf("BAD_ADDR\n");
						}
					}
					break;
					
				case CMD_HELP:
					printf("Available commands:\nstep [steps]\nrun\ndump\ndisassemble [instructions]\nquit\n");
					break;
					
				case CMD_COMPUTER:
					comp_out = 1;
					printf("OK\n");
					break;
					
				case CMD_HUMAN:
					comp_out = 0;
					break;
					
				case CMD_AUTOSTAT:
					auto_stat = 1;
					break;
					
				case -1:
					printf("Unknown command '%s'\n", command);
					if (comp_out) printf("BAD_CMD\n");
					break;
					
				case -2:
					printf("Ambiguous command '%s'\n", command);
					if (comp_out) printf("BAD_CMD\n");
					break;
					
				default:
					printf("Command not implemented '%d'\n", command);
					if (comp_out) printf("BAD_CMD\n");
					break;
			}
		}
		fflush(stdout);
	}
	
	return 0;
}