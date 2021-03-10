/* Minimalistic Archive Creator
 * For C Minimalistic RISC machine
 */

#include <cmdline.h>
#include <fcntl.h>
#include <unistd.h>
#include <object.h>

char * cmdline_outfile = NULL;
char ** cmdline_infile = NULL;
long cmdline_help = 0;

struct cmdline_opts options[] = {
	{ "-o", "--output", "out_file", "Write output object to this file.", (void *) &cmdline_outfile, ARG_STR, false, 0, NON_POSITIONAL },
	{ "-h", "--help", NULL, "Show this help.", (void *) &cmdline_help, ARG_BOOL, true, 0, NON_POSITIONAL}, 
	{ NULL, NULL, "source_file", "File names of archived objects.", &cmdline_infile, ARG_STR, false, 0, NON_POSITIONAL},
};

struct cmdline_args commandline = { options, 3 };

/** Vytvori nazov sekcie.
 * @param filename nazov suboru z ktoreho sa sekcia vytvara
 * @param suffix retazec, ktory sa doplni na konic nazvu sekcie
 * @return nazov sekcie
 */
char * create_section_name(const char * filename, const char * suffix) {
	char * slash;
	char * dot;
	char * tmp_sect_name;
	slash = strrchr(filename, '/');
	if (slash == NULL) slash = filename;
	else slash++;
	dot = strrchr(filename, '.');
	if (dot == NULL || dot < slash) dot = filename + strlen(filename);
	tmp_sect_name = malloc((dot - slash) + strlen(suffix) + 2);
	memset(tmp_sect_name, 0, (dot - slash) + strlen(suffix) + 2);
	tmp_sect_name[0] = '@';
	strncat(tmp_sect_name, slash, dot - slash);
	strcat(tmp_sect_name, suffix);
	return tmp_sect_name;
}

/** Archiver objektovych suborov.
 * Vytvori archivny objektovy subor, ktory obsahuje sekcie (zatial .data a .text) vsetkych vstupnych objektovych suborov z prikazoveho riadka.
 * Sekcie su nazvane podla mien objektovych suborov a povodnych nazvov sekcii
 * Vystupom je objektovy archiv.
 * @param argc pocet argumentov
 * @param argv hodnoty argumentov
 * @return stav behu aplikacie
 */
int main(int argc, char ** argv) {
	int cmdline_retval = process_commandline(argc, argv, &commandline);
	if (cmdline_help) { print_help(&commandline, argv[0]); return 0; }
	if (cmdline_retval != 0) return cmdline_retval;
	
	int fd, q;
	
	OBJECT * object;
	OBJECT * binary = object_create(cmdline_outfile);
	
	SECTION * obj_section, * ar_section;
	
	for (q = 0; q < options[commandline.count - 1].matched; q++) {
		char * section_name;
		printf("Archiving '%s'\n", cmdline_infile[q]);
		object = object_load(cmdline_infile[q]);
		if (object == NULL) exit(3);
		obj_section = object_get_section_by_name(object, ".data");
		section_name = create_section_name(cmdline_infile[q], ".data");
		printf("New section name is '%s'\n", section_name);
		ar_section = section_create(section_name);
		section_copy(ar_section, obj_section);
		object_add_section(binary, ar_section);
		section_free(obj_section, 0);
		section_free(ar_section, 0);
		free(section_name);
		
		obj_section = object_get_section_by_name(object, ".text");
		section_name = create_section_name(cmdline_infile[q], ".text");
		printf("New section name is '%s'\n", section_name);
		ar_section = section_create(section_name);
		section_copy(ar_section, obj_section);
		object_add_section(binary, ar_section);
		section_free(obj_section, 0);
		section_free(ar_section, 0);
		free(section_name);
		
		object_free(object);
	}
	
	object_write(binary);
	
	return 0;
}
