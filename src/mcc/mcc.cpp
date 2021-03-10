/* Minimalistic Assembler
 * For C Minimalistic RISC machine
 */

#include <cmdline.h>
#include <nodes.h>
#include <compile.h>
#include <iostream>

extern int yyparse();
extern FILE *yyin;

char * cmdline_outfile = NULL;
char ** cmdline_infile = NULL;
long cmdline_preprocess = 0;
long cmdline_compile = 0;
long cmdline_tree = 0;
char * cmdline_warn = "normal";
long cmdline_fnobuiltin = 0;
long cmdline_optimize = 0;
long cmdline_help = 0;

struct cmdline_opts options[] = {
	{ "-o", "--output", "out_file", "Write output object to this file.", (void *) &cmdline_outfile, ARG_STR, OPTIONAL, 0, NON_POSITIONAL},
	{ "-E", "--preprocess", NULL, "Preprocess only. Write raw C source to output.", (void *) &cmdline_preprocess, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL},
	{ "-c", "--compile", NULL, "Compile only. Write linkable object to output.", (void *) &cmdline_compile, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL},
	{ "-W", "--warn", "none|normal|all", "Warning level.", (void *) &cmdline_warn, ARG_STR, OPTIONAL, 0, NON_POSITIONAL},
	{ "-B", "--fnobuiltin", NULL, "Do not include builtin objects.", (void *) &cmdline_fnobuiltin, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL},
	{ "-O", "--optimize", "0-2", "Optimization level", (void *) &cmdline_optimize, ARG_NUM, OPTIONAL, 0, NON_POSITIONAL},
	{ "-h", "--help", NULL, "Show this help", (void *) &cmdline_help, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL},
	{ "-T", "--tree", NULL, "Write abstract syntactic tree to file infile.tree", (void *) &cmdline_tree, ARG_BOOL, OPTIONAL, 0, NON_POSITIONAL},
	{ NULL, NULL, "source_file", "File name of source file.", &cmdline_infile, ARG_STR, MANDATORY, 0, ANY_COUNT},
};

struct cmdline_args commandline = { options, 9 };

extern AST::Program * program;
extern char * yytext;
extern int riadok;
extern int pocetChyb;

int main(int argc, char ** argv) {
	CC::Assembly * output;
	int cmdline_retval = process_commandline(argc, argv, &commandline);
	if (cmdline_help) { print_help(&commandline, argv[0]); return 0; }
	if (cmdline_retval != 0) return cmdline_retval;

	yyin = fopen( cmdline_infile[0], "r" );
	if (yyin == NULL) {
		fprintf(stderr, "error: cannot open input file %s!\n", cmdline_infile[0]);
		return 1;
	}
	yyparse();
	if (pocetChyb == 0) {
		if (cmdline_tree) {
			char * filename = (char *) malloc(strlen(cmdline_infile[0]) + 4);
			memset(filename, 0, strlen(cmdline_infile[0]) + 4);
			if (strrchr(cmdline_infile[0], '.') != NULL) {
				strncat(filename, cmdline_infile[0], strrchr(cmdline_infile[0], '.') - cmdline_infile[0]);
				strcat(filename, ".tree");
			} else {
				strcat(filename, cmdline_infile[0]);
				strcat(filename, ".tree");
			}
			FILE * f = fopen(filename, "w");
			fprintf(f, "%s", program->dump(0).c_str());
		}
		CC::Context * cc = new CC::Context();
		output = cc->compileProgram(program);
		std::cout << "Compiled program dump:" << std::endl;
		std::cout << output->getInstructions() << std::endl;
	}
	
	return 0;
}