/* Minimalistic Linker
 * For C Minimalistic RISC machine
 */
	
#include <cmdline.h>
#include <fcntl.h>
#include <unistd.h>
#include <object.h>

char * cmdline_outfile = NULL;
char ** cmdline_infile = NULL;
long cmdline_safe = 0;
long cmdline_log = 0;
long cmdline_help = 0;
char * cmdline_entrypoint = "main";
char * cmdline_library = NULL;
long cmdline_verbose_1 = 0;
long cmdline_verbose_2 = 0;
long cmdline_verbose_3 = 0;
long cmdline_debug = 0;

struct cmdline_opts options[] = {
	{ "-o", "--output", "out_file", "Write output object to this file.", (void *) &cmdline_outfile, ARG_STR, MANDATORY, 0, NON_POSITIONAL },
	{ "-d", "--debuggable", NULL, "Write output with symbols.", (void *) &cmdline_debug, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL },
	{ "-e", "--entrypoint", "symbol_name", "Take this symbol as entrypoint.", (void *) &cmdline_entrypoint, ARG_STR, OPTIONAL, 0, NON_POSITIONAL },
	{ "-v", "--verbose", NULL, "Write verbose information about linking process.", (void *) &cmdline_verbose_1, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL },
	{ "-vv", "--more-verbose", NULL, "Write more verbose information about linking process.", (void *) &cmdline_verbose_2, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL },
	{ "-l", "--library", "library_name", "Use this library to resolve unresolved symbols after final linkage.", (void *) &cmdline_library, ARG_STR, OPTIONAL, 0, NON_POSITIONAL },
	{ "-vvv", "--most-verbose", NULL, "Write very verbose information about linking process.", (void *) &cmdline_verbose_3, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL },
	{ "-h", "--help", NULL, "Show this help", (void *) &cmdline_help, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL}, 
	{ NULL, NULL, "source_file", "File name of linked objects.", &cmdline_infile, ARG_STR, MANDATORY, 0, NON_POSITIONAL},
};

struct cmdline_args commandline = { options, 9 };

int main(int argc, char ** argv) {
	int cmdline_retval = process_commandline(argc, argv, &commandline);
	if (cmdline_help) { print_help(&commandline, argv[0]); return 0; }
	if (cmdline_retval != 0) return cmdline_retval;
	
	int fd, q;
	
	OBJECT ** objects = malloc(sizeof(OBJECT *) * options[commandline.count - 1].matched);
	OBJECT * binary = object_create(cmdline_outfile);
	OBJECT * std_library = NULL;
	OBJECT * debug_binary = NULL;
	if (cmdline_library != NULL) {
		std_library = object_load(cmdline_library);
		if (std_library == NULL) {
			fprintf(stderr, "Unable to load standard library '%s'!\n", cmdline_library);
			exit(1);
		}
	}
	
	SECTION * global_data = section_create(".data");
	SECTION * global_text = section_create(".text");
	SECTION * global_binary = section_create(".binary");
	
	SECTION * section;
	SYMBOL * unresolved_symbol = NULL;
	
	if (cmdline_verbose_1) objects_be_verbose(1);
	if (cmdline_verbose_2) objects_be_verbose(2);
	if (cmdline_verbose_3) objects_be_verbose(3);
	
	for (q = 0; q < options[commandline.count - 1].matched; q++) {
		if (cmdline_verbose_1 || cmdline_verbose_2 || cmdline_verbose_3) printf("Trying to link '%s'\n", cmdline_infile[q]);
		objects[q] = object_load(cmdline_infile[q]);
		if (objects[q] == NULL) exit(3);
		section = object_get_section_by_name(objects[q], ".data");
		if (section != NULL) section_append(global_data, section);
		section = object_get_section_by_name(objects[q], ".text");
		if (section != NULL) section_append(global_text, section);
	}
	section_append(global_binary, global_data);
	section_append(global_binary, global_text);
	
//	section_free(global_data, 1);
//	section_free(global_text, 1);
	
	char resolve_symbol_name[96];
	
	if (std_library != NULL) {
		while ((unresolved_symbol = section_try_relocation(global_binary)) != NULL) {
			sprintf(resolve_symbol_name, "@%s.text", unresolved_symbol->name);
			section = object_get_section_by_name(std_library, resolve_symbol_name);
			if (section != NULL) {
				section_append(global_binary, section);
			} else {
				fprintf(stderr, "error: unresolvable symbol '%s'!\n", unresolved_symbol->name);
				exit(1);
			}
		}
	}
	
	section_do_relocation(global_binary);
	
	ADDRESS entrypoint_address = symbol_get_address(global_binary, cmdline_entrypoint);
	if (entrypoint_address == 0xFFFF) {
		fprintf(stderr, "Entrypoint symbol '%s' is not resolved!\n", cmdline_entrypoint);
		exit(1);
	}
	
	symbol_set(global_binary, "@@entrypoint", entrypoint_address, 0);
	
	if (!cmdline_debug) {
		if (!binary_write(global_binary, entrypoint_address, cmdline_outfile)) {
			fprintf(stderr, "Failed to write binary '%s'\n", cmdline_outfile);
			exit(1);
		}
	} else {
		debug_binary = object_create(cmdline_outfile);
		if (debug_binary == NULL) {
			fprintf(stderr, "Internal error: cannot create object\n");
			exit(1);
		}
		object_add_section(debug_binary, global_binary);
		if (!object_write(debug_binary)) {
			fprintf(stderr, "Failed to write debug binary '%s'\n", cmdline_outfile);
			exit(1);
		}
	}
	if (cmdline_verbose_1 || cmdline_verbose_2 || cmdline_verbose_3) printf("%s() address is 0x%04X\n", cmdline_entrypoint, entrypoint_address);
	
	return 0;
}
