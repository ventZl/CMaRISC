#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include <vm.h>

#include <cmdline.h>

long cmdline_steps = 0;
long cmdline_dump = 0;
long cmdline_interactive = 0;
long cmdline_help = 0;
char * cmdline_infile = NULL;

struct cmdline_opts options[] = {
	{ "-s", "--steps", "N", "Execute N instructions and stop. If N is 0, run until error or interrupt.", (void *) &cmdline_steps, ARG_NUM, OPTIONAL, 0, NON_POSITIONAL},
	{ "-d", "--dump-registers", NULL, "Dump registers after executed instructions or after every step in unlimited execution (-s 0).", (void *) &cmdline_dump, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL},
	{ "-i", "--interactive", NULL, "Prompt for user action after every step.", (void *) &cmdline_interactive, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL},
	{ "-h", "--help", NULL, "Show this help.", (void *) &cmdline_help, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL},
	{ NULL, NULL, "executable_file", "File name of executable input file.", (void *)&cmdline_infile, ARG_STR, MANDATORY, 0, 1 }
};

struct cmdline_args commandline = { options, 5 };

int main(int argc, char ** argv) {
	char * memory = malloc(65535);
	memset(memory, 0, 65535);
	struct stat vmm_stat;
	uint16_t entrypoint = 0;
	char epbyte, is_ok = 0;
	int left_steps;
	
	int cmdline_retval = process_commandline(argc, argv, &commandline);
	if (cmdline_help) { print_help(&commandline, argv[0]); return 0; }
	if (cmdline_retval != 0) {
		printf("cmdline_retval %d\n", cmdline_retval);
		return cmdline_retval;
	}
	
	left_steps = cmdline_steps;
	
	int fd = open(cmdline_infile, O_RDONLY);
	if (fd != -1) {
		if (fstat(fd, &vmm_stat) == -1) {
			fprintf(stderr, "Unable to fstat() virtual memory image file\n");
			exit(1);
		}
		do {
			if (read(fd, &epbyte, 1) == -1) break;
			entrypoint = (entrypoint << 8) | epbyte;
			if (read(fd, &epbyte, 1) == -1) break;
			entrypoint = (entrypoint << 8) | epbyte;
			is_ok = 1;
		} while (0);
		if (is_ok == 0) {
			fprintf(stderr, "Unabel to read entrypoint from image!\n");
			exit(1);
		}
		int cursor = 0, remaining = vmm_stat.st_size - 2, rd;
		while (remaining > 0) {
			rd = read(fd, &(memory[cursor]), (remaining > 4096 ? 4096 : remaining));
			if (rd != -1) { cursor += rd; remaining -= rd; }
			else {
				fprintf(stderr, "Unable to read virtual memory data from image!\n");
				exit(1);
			}
		}
	} else {
		fprintf(stderr, "Unable to open virtual memory image file\n");
		exit(1);
	}
	VIRTUAL_MACHINE * mach = createVirtualMachine(memory, 65535, entrypoint);
	while (left_steps > 0 || cmdline_steps == 0) {
		VM_STATE state = traceVirtualMachine(mach, left_steps > 0 ? left_steps : 1);
		if (cmdline_dump) dumpRegistersVirtualMachine(mach);
		
		switch(state) {
			case VM_OK:
				printf("Virtual machine OK\n");
				break;
				
			case VM_DIVIDE_BY_ZERO:
				printf("Division by zero\n");
				break;
				
			case VM_ILLEGAL_OPCODE:
				printf("Illegal instruction\n");
				break;
				
			case VM_OUT_OF_MEMORY:
				printf("Segmentation fault\n");
				break;
		}
		
		if (cmdline_interactive) {
			int ccc;
			int silent = 0;
			do {
				ccc = 0;
				if (!silent) printf("Action? (C)ontinue (Q)uit (S)tep; Enter continues: ");
				silent = 0;
				epbyte = fgetc(stdin);
				switch (epbyte) {
					case 'c':
					case 'C':
						left_steps = 0;
						cmdline_steps = 0;
						break;
						
					case 'q':
					case 'Q':
						left_steps = 0;
						cmdline_steps = 1;
						break;
						
					case 's':
					case 'S':
						left_steps = 1;
						cmdline_steps = 1;
						break;
					case 'd':
					case 'D':
						dumpRegistersVirtualMachine(mach);
						ccc = 1;
						break;
						
					case '\n':
						silent = 1;
						ccc = 1;
				}
			} while (ccc);
		}
	}
	return 0;
}