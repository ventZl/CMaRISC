/* Minimalistic Assembler
 * For C Minimalistic RISC machine
 */

#include <cmdline.h>
#include "analyzer.h"
#include "assembler.h"
#include <fcntl.h>
#include <unistd.h>

char * cmdline_outfile = NULL;
char ** cmdline_infile = NULL;
long cmdline_safe = 0;
long cmdline_log = 0;
long cmdline_help = 0;
char * cmdline_dump_text_file = NULL;
char * cmdline_dump_data_file = NULL;

struct cmdline_opts options[] = {
	{ "-o", "--output", "out_file", "Write output object to this file.", (void *) &cmdline_outfile, ARG_STR, false, 0, NON_POSITIONAL},
	{ "-s", "--safe", NULL, "Warn about certain potentially unsafe instructions", (void *) &cmdline_safe, ARG_BOOL, true, 0, NON_POSITIONAL},
	{ "-d", "--dump-text", "filename", "Dump text of compiled object into file", (void *) &cmdline_dump_text_file, ARG_STR, true, 0, NON_POSITIONAL},
	{ "-D", "--dump-data", "filename", "Dump data of compiled object into file", (void *) &cmdline_dump_data_file, ARG_STR, true, 0, NON_POSITIONAL},
	{ "-l", "--log", NULL, "Write assembly log", (void *) &cmdline_log, ARG_BOOL, true, 0, NON_POSITIONAL},
	{ "-h", "--help", NULL, "Show this help", (void *) &cmdline_help, ARG_BOOL, true, 0, NON_POSITIONAL}, 
	{ NULL, NULL, "source_file", "File name of assembly input file.", &cmdline_infile, ARG_STR, false, 0, NON_POSITIONAL},
};

struct cmdline_args commandline = { options, 7 };

/** Minimalisticky assembler, prelozi jeden zdrojovy subor do jedneho objektoveho suboru.
 * @param arc pocet vstupnych argumentov
 * @param argv vstupne argumenty
 * @return navratovy kod aplikacie
 */
int main(int argc, char ** argv) {
	int cmdline_retval = process_commandline(argc, argv, &commandline);
	if (cmdline_help) { print_help(&commandline, argv[0]); return 0; }
	if (cmdline_retval != 0) return cmdline_retval;

	int fd, q;

	if (cmdline_safe) do_warn_unsafe_assembly();
	
	OBJECT * object = object_create(cmdline_outfile);

	for (q = 0; q < options[6].matched; q++) {
		if (cmdline_log) printf("Trying to analyze %s and write object to %s\n", cmdline_infile[q], cmdline_outfile);
		fd = open(cmdline_infile[0], O_RDONLY);
		analyze_input(fd, object);
		close(fd);
		object_write(object);
	}
	
	if (cmdline_dump_data_file != NULL) {
		if (cmdline_log) printf("Dumping data into %s\n", cmdline_dump_data_file);
		section_dump(object_get_section_by_name(object, ".data"), cmdline_dump_data_file);
	}
	
	if (cmdline_dump_text_file != NULL) {
		if (cmdline_log) printf("Dumping text into %s\n", cmdline_dump_text_file);
		section_dump(object_get_section_by_name(object, ".text"), cmdline_dump_text_file);
	}
	
	return 0;
}