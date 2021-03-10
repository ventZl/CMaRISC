#include <stdio.h>
#include <stdlib.h>
#include "preprocess.h"
#include "expansion.h"

#define USE_PATTERNS(pattern) pattern, (sizeof(pattern)/sizeof(struct token_patterns))

static const struct token_patterns begin_patterns[] = { { " ", TOKEN_WHITESPACE }, { "#", TOKEN_HASH }, { "Aa_", TOKEN_STRING }, { "\n", TOKEN_EOL }, { "*", TOKEN_VERBATIM } };
static const struct token_patterns line_patterns[] = { { "Aa_", TOKEN_STRING }, { "\n", TOKEN_EOL }, { "*", TOKEN_VERBATIM } };
static const struct token_patterns bracket_patterns[] = { { "<", TOKEN_LBR_SHARP }, { "\"", TOKEN_QUOTE }, { ">", TOKEN_RBR_SHARP } };
static const struct token_patterns include_file_patterns[] = { { ">", TOKEN_RBR_SHARP }, { "\"", TOKEN_QUOTE }, { "*", TOKEN_STRING } };
static const struct token_patterns start_command_patterns[] = { { "Aa", TOKEN_STRING }, { "1", TOKEN_NUMBER } };
static const struct token_patterns macro_def_patterns[] = { { " ", TOKEN_WHITESPACE }, { "(", TOKEN_LBR }, { "Aa_", TOKEN_STRING } };
static const struct token_patterns macro_arg_patterns[] = { { "Aa_", TOKEN_STRING }, { ",", TOKEN_COMMA }, { ")", TOKEN_RBR }, { "(", TOKEN_LBR }, { " ", TOKEN_WHITESPACE }, { "\n", TOKEN_EOL } };
static const struct token_patterns macro_call_patterns[] = { { "(", TOKEN_LBR }, { "\n", TOKEN_EOL }, { "*", TOKEN_VERBATIM } };
static const struct token_patterns macro_parm_patterns[] = { { ",", TOKEN_COMMA }, { ")", TOKEN_RBR }, { "*", TOKEN_STRING } };
static const struct token_patterns whitespace_patterns[] = { { " ", TOKEN_WHITESPACE} };
static const struct token_patterns macro_body_patterns[] = { { "\n", TOKEN_EOL }, { "*", TOKEN_VERBATIM } };
static const struct token_patterns macro_name_patterns[] = { { "Aa_", TOKEN_STRING } };
static const struct token_patterns eol_patterns[] = { { "\n", TOKEN_EOL } };
static const struct token_patterns garbage_patterns[] = { { "\n", TOKEN_EOL }, { "*", TOKEN_VERBATIM } };

static const struct token_patterns preprocessor_commands[] = {
	{ "define", TOKEN_DEFINE },
	{ "undef", TOKEN_UNDEF },
	{ "if", TOKEN_IF },
	{ "ifdef", TOKEN_IFDEF },
	{ "ifndef", TOKEN_IFNDEF },
	{ "else", TOKEN_ELSE },
	{ "endif", TOKEN_ENDIF },
	{ "include", TOKEN_INCLUDE }
};

static const char * lexer_state_names[] = { "STATE_BEGIN", "STATE_LINE", "STATE_BEGIN_COMMAND", "STATE_COMMAND", "STATE_MACRO_CALL" };
static const char * lexer_token_names[] = { "TOKEN_WHITESPACE", "TOKEN_VERBATIM", "TOKEN_HASH", "TOKEN_STRING", "TOKEN_LBR", "TOKEN_RBR", "TOKEN_LBR_SHARP", "TOKEN_RBR_SHARP", "TOKEN_QUOTE",
"TOKEN_DEFINE", "TOKEN_UNDEF", "TOKEN_IF", "TOKEN_ELSE", "TOKEN_ENDIF", "TOKEN_INCLUDE", "TOKEN_WARNING", "TOKEN_ERROR", "TOKEN_IFDEF", "TOKEN_IFNDEF", 
"TOKEN_NUMBER", "TOKEN_EOF", "TOKEN_EOL", "TOKEN_COMMA"
};

/** Konstruktor preprocesoroveho objektu.
 * Skonstruuje a zaregistruje niekolko vstavanych makier. Vytvori instanciu 
 * lexera.
 * @param entry_file nazov suboru, ktory sa bude spracovavat
 */
Preprocessor::Preprocessor(std::string entry_file) {
	this->lexer = new LexicalAnalyzer();
	
	this->file = entry_file;
	this->counter_macro = new CounterMacro("__COUNTER__");
	this->file_macro = new FileMacro("__FILE__", this->lexer);
	this->entry_file_macro = new ConstMacro("__BASE_FILE__", entry_file);
	this->line_macro = new LineMacro("__LINE__", this->lexer);
	
	this->addMacro(this->counter_macro);
	this->addMacro(this->file_macro);
	this->addMacro(this->entry_file_macro);
	this->addMacro(this->line_macro);
	
	this->skip = false;
}

/** Destruktor preprocesoroveho objektu.
 * Zmaze vstavane makra
 */
Preprocessor::~Preprocessor() {
	delete this->lexer;
	delete this->counter_macro;
	delete this->file_macro;
	delete this->entry_file_macro;
	delete this->line_macro;
}

/** Zaregistruje nove makro.
 * @param macro objekt noveho makra
 */
void Preprocessor::addMacro(Macro* macro) {
	macro_list_iterator it = this->macros.find(macro->getName());
	if (it != this->macros.end()) {
		this->emitError("Macro " + macro->getName() + " already defined");
	}
	this->macros.insert(std::pair<std::string, Macro *>(macro->getName(), macro));
}

/** Spracuje postupnost tokenov.
 * Metoda sluzi ako skratka pre parsovanie striktne sformovanej postupnosti tokenov
 * @param lexer pointer na lexer
 * @param run predpis postupnosti tokenov
 * @param run_size velkost postupnosti
 * @return pocet tokenov, ktore matchli
 */
int Preprocessor::preprocessRun(LexicalAnalyzer * lexer, token_run* run, unsigned int run_size) {
	int tok_processed = 0;
	for (int q = 0; q < run_size; q++) {
		try {
			if (run[q].token != NULL) delete run[q].token;
			run[q].token = lexer->findToken(run[q].patterns, run[q].patterns_count);
		} catch (token_exception & e) {
			return tok_processed;
		}
		tok_processed++;
	}
	return tok_processed;
}

/** Convenience metoda na citanie whitespace.
 * Precita jeden token a vrati priznak, ci token bol, alebo nebol whitespacom
 * @param lexer objekt lexera
 * @return true ak token bol whitespace inac false
 */
bool readWhitespace(LexicalAnalyzer * lexer) {
	bool ret;
	Token * tok = lexer->findToken(USE_PATTERNS(whitespace_patterns));
	if (tok->getType() == TOKEN_WHITESPACE) ret = true;
	else ret = false;
	delete tok;
	return ret;
}

/** Sparsuje definiciu makra.
 * Parsuje definicie makra v tvare 
 * #define NAZOV TELO
 * a
 * #define NAZOV(parameter1,parameter2) TELO
 * @param lexer objekt lexera
 * @return novy stav, ktory ma nadobudnut stavovy stroj po sparsovani definicie makra
 */
int Preprocessor::defineMacro(LexicalAnalyzer* lexer) {
	Token * tok;
	Macro * new_macro;
	
	if (!readWhitespace(lexer)) {
		this->emitError("Invalid directive"); 
	}
	
	tok = lexer->findToken(USE_PATTERNS(macro_name_patterns));
	
	if (tok->getType() == TOKEN_STRING) {
		new_macro = new Macro(tok->getString());
	}
	
	delete tok;
	
	tok = lexer->findToken(USE_PATTERNS(macro_arg_patterns));
	
	if (tok->getType() == TOKEN_LBR) {
		Token * arg_tok;
		do {
			arg_tok = lexer->findToken(USE_PATTERNS(macro_arg_patterns));
			if (arg_tok->getType() == TOKEN_STRING) new_macro->addArgName(arg_tok->getString());
			else {
				this->emitError("#define requires argument name in argument list");
			}
			delete arg_tok;
			arg_tok = lexer->findToken(USE_PATTERNS(macro_arg_patterns));
		} while (arg_tok->getType() != TOKEN_RBR);
		delete arg_tok;
	}
	
	if (tok->getType() == TOKEN_LBR) {
		delete tok;
		tok = lexer->findToken(USE_PATTERNS(macro_arg_patterns));
	}
	
	if (tok->getType() == TOKEN_WHITESPACE) {
		// makro zrejme ma telo
		tok = lexer->findToken(USE_PATTERNS(macro_body_patterns));
		if (tok->getType() == TOKEN_VERBATIM) {
			new_macro->setBody(tok->getString());
		}
	}
	
	if (tok->getType() != TOKEN_EOL) {
		delete tok;
		
		tok = lexer->findToken(USE_PATTERNS(eol_patterns), 1);
	}
	
	delete tok;
	
	macro_list_iterator it = this->macros.find(new_macro->getName());
	if (it != this->macros.end()) {
		this->emitError("Macro " + new_macro->getName() + " already defined");
	}
	this->macros.insert(std::pair<std::string, Macro *>(new_macro->getName(), new_macro));
	
	return STATE_BEGIN;
}

/** Sparsuje oddefinovanie makra
 * Odstrani makro.
 * @param lexer objekt lexera
 * @return novy stav po sparsovani definicie
 */
int Preprocessor::undefineMacro(LexicalAnalyzer* lexer) {
	static struct token_run undefine_run[] = {
		{ NULL, USE_PATTERNS(whitespace_patterns), ANY_LENGTH },
		{ NULL, USE_PATTERNS(macro_name_patterns), ANY_LENGTH },
		{ NULL, USE_PATTERNS(eol_patterns), 1 }
	};
	int found_tokens = this->preprocessRun(lexer, undefine_run, 3);
	if (found_tokens == 3) {
		macro_list_iterator it = this->macros.find(undefine_run[1].token->getString());
		if (it != this->macros.end()) {
			this->macros.erase(it);
		} // ak makro nie je definovane, undef nespravi nic, ani neemituje chybu
	} else {
		this->emitError("Macro name missing for #undefine directive!");
	}
	return STATE_BEGIN;
}

/** Sparsuje direktivu ifdef / ifndef.
 * Zisti, ci je dane makro definovane a podla toho vytvori novu podmienku.
 * Na zaklade pravdivosti podmienky je nasledujuci text bud spracovany, alebo
 * az po najblizsie else ignorovany.
 * @param lexer objekt lexera
 * @param defined ak je true, podmienka sa vyhodnoti ako pravdiva ak je makrodefinovane, inac naopak
 * @return novy stav po parsovani
 */
int Preprocessor::ifDefined(LexicalAnalyzer* lexer, bool defined) {
	static struct token_run ifdef_run[] = {
		{ NULL, USE_PATTERNS(whitespace_patterns), ANY_LENGTH },
		{ NULL, USE_PATTERNS(macro_name_patterns), ANY_LENGTH },
		{ NULL, USE_PATTERNS(eol_patterns), 1 }
	};
	int found_tokens = this->preprocessRun(lexer, ifdef_run, 3);
	if (found_tokens == 3) {
		macro_list_iterator it = this->macros.find(ifdef_run[1].token->getString());
		if ((it == this->macros.end()) ^ defined) {
			this->enterConditional(true);
		} else {
			this->enterConditional(false);
		}
	} else {
		this->emitError("Macro name missing for #ifdef directive!");
	}
	return 0;
}

/** Sparsuje direktivu else
 * Invertuje pravdivostnu hodnotu poslednej podmienky.
 * @param lexer objekt lexera
 * @return novy stav po parsovani
 */
int Preprocessor::elseIf(LexicalAnalyzer* lexer) {
	Token * tok = NULL;
	bool warned = false;
	do {
		if (tok != NULL) delete tok;
		tok = lexer->findToken(USE_PATTERNS(garbage_patterns), 1);
		if (tok->getType() != TOKEN_EOL && !warned) {
			warned = true;
			this->emitWarning("extra tokens after #else directive");
		}
	} while (tok->getType() != TOKEN_EOL);
	delete tok;
	try {
		//		printf("BLABLABLA\n");
		this->invertConditional();
	} catch (invert_exception & e) {
		this->emitError("#else after #else");
	} catch (condition_exception & e) {
		this->emitError("#else without previous #if!");
	}
	return STATE_BEGIN;
}

/** Sparsuje direktivu endif
 * Vystupi z najvnutornejsieho cyklu.
 * @param lexer objekt lexera
 * @return novy stav po parsovani
 */
int Preprocessor::endIf(LexicalAnalyzer* lexer) {
	if (this->conditions.empty()) {
		this->emitError("#endif without previous #if!");
	} else {
		try {
			this->exitConditional();
			return STATE_BEGIN;
		} catch (invert_exception & e) {
			this->emitError("#else after #else");
		}
	}
	return 0;
}

/** Vypise standardne formatovanu chybu preprocesora
 * Chyba je prependnuta retazcom error subor:riadok:
 * @param err_str text chyboveho hlasenia
 */
void Preprocessor::emitError(std::string err_str) {
	fprintf(stderr, "%s:%d: error: %s\n", this->lexer->getCurrentFile().c_str(), this->lexer->getCurrentLine(), err_str.c_str());
	this->skip = true;
}

/** Vypise standardne formatovanu chybu preprocesora
 * Chyba je prependnuta retazcom error subor:riadok:
 * @param err_str text chyboveho hlasenia
 */
void Preprocessor::emitWarning(std::string err_str) {
	fprintf(stderr, "%s:%d: warning: %s\n", this->lexer->getCurrentFile().c_str(), this->lexer->getCurrentLine(), err_str.c_str());
}

/** Sparsuje volanie makra
 * @param lexer objekt lexera
 * @param macro objekt makra, ktore sa vola
 */
void Preprocessor::callMacro(LexicalAnalyzer* lexer, Macro * macro) {
	MacroExpansion * expansion = new MacroExpansion(macro);
	Token * tok = lexer->findToken(USE_PATTERNS(macro_call_patterns), 1);
	switch (tok->getType()) {
		case TOKEN_LBR:
		{
			Token * arg_tok = NULL;
			while(1) {
				if (arg_tok != NULL) delete arg_tok;
				arg_tok = lexer->findToken(USE_PATTERNS(macro_parm_patterns));
				if (arg_tok->getType() == TOKEN_STRING) {
					expansion->addParameterReplacement(arg_tok->getString());
				}
				delete arg_tok;
				arg_tok = lexer->findToken(USE_PATTERNS(macro_parm_patterns));
				if (arg_tok->getType() == TOKEN_RBR) break;
			} 
		}
		lexer->pushString(expansion->getBody());
		break;
		
		case TOKEN_WHITESPACE:
		case TOKEN_EOL:
		case TOKEN_VERBATIM:
			
			lexer->pushString(tok->getString());
			lexer->pushString(expansion->getBody());
			break;
	}
	delete tok;
	return;
}

/** Sparsuje vlozenie ineho suboru.
 * Podporovane su direktivy
 * #include <subor>
 * a
 * #include "subor"
 * @param lexer objekt lexera
 * @return novy stav po parsovani
 */
int Preprocessor::include(LexicalAnalyzer * lexer) {
	static struct token_run include_run[] = {
		{ NULL, USE_PATTERNS(whitespace_patterns), ANY_LENGTH },
		{ NULL, USE_PATTERNS(bracket_patterns), 1 },
		{ NULL, USE_PATTERNS(include_file_patterns), ANY_LENGTH },
		{ NULL, USE_PATTERNS(bracket_patterns), 1 },
		{ NULL, USE_PATTERNS(eol_patterns), 1 }
	};
	
	int found_tokens = this->preprocessRun(lexer, include_run, 5);
	if (found_tokens == 5) {
		std::string include_name;
		if (include_run[1].token->getType() == TOKEN_LBR_SHARP && include_run[3].token->getType() == TOKEN_RBR_SHARP) {
			include_name = this->findFile(this->getIncludePaths(), include_run[2].token->getString());
		} else if (include_run[1].token->getType() == TOKEN_QUOTE && include_run[3].token->getType() == TOKEN_QUOTE) {
			include_name = this->findFile(this->getCurrentPath(), include_run[2].token->getString());
		} else {
			this->emitError("Syntax error, include filename should be enclosed in <> or \"\"");
		}
		FILE * f = fopen(include_name.c_str(), "r");
		if (f != NULL) {
			lexer->pushSource(f, include_run[2].token->getString());
		} else {
			this->emitError("Unable to open " + include_name + "for include");
		}
	} else {
		this->emitError("Error parsing #include directive");
	}
	return STATE_BEGIN;
}

/** Spracuje vstupny subor.
 * @return obsah spracovaneho suboru
 */
std::string Preprocessor::preprocess() {
	std::string retstr;
	FILE * f = fopen(this->file.c_str(), "r");
	if (f != NULL) lexer->pushSource(f, this->file);
	Token * tok = NULL;
	int state = STATE_BEGIN;
	int q;
	try {
		while (!this->skip) {
			//			printf("Current state is %s (echo %s)\n", lexer_state_names[state], this->isOutputPermitted() ? "ON" : "OFF" );
			if (this->isOutputPermitted()) {
				switch (state) {
					case STATE_BEGIN:
						tok = lexer->findToken(USE_PATTERNS(begin_patterns));
						switch (tok->getType()) {
							case TOKEN_WHITESPACE:
								retstr += tok->getString();
								break;
								
							case TOKEN_HASH:
								state = STATE_BEGIN_COMMAND;
								break;
								
							case TOKEN_EOL:
								retstr += tok->getString();
								break;
								
							case TOKEN_STRING:
							{
								/* check if string isn't macro name */
								std::map<std::string, Macro *>::iterator it = this->macros.find(tok->getString());
								if (it != this->macros.end()) {
									// we have some macro with that name
									this->callMacro(lexer, it->second);
									state = STATE_LINE;
									//							str += it->second->getBody();
								} else {
									// no macro with that name, bail out
									retstr += tok->getString();
									state = STATE_LINE;
								}
								break;
							}
							
							case TOKEN_VERBATIM:
								state = STATE_LINE;
								retstr += tok->getString();
								break;
						}
						break;
						
							case STATE_LINE:
								tok = lexer->findToken(USE_PATTERNS(line_patterns));
								switch (tok->getType()) {
									case TOKEN_VERBATIM:
										retstr += tok->getString();
										break;
										
									case TOKEN_STRING:
									{
										/* check if string isn't macro name */
										std::map<std::string, Macro *>::iterator it = this->macros.find(tok->getString());
										if (it != this->macros.end()) {
											// we have some macro with that name
											this->callMacro(lexer, it->second);
											//							str += it->second->getBody();
										} else {
											// no macro with that name, bail out
											retstr += tok->getString();
										}
										break;
									}
									
									case TOKEN_EOL:
										state = STATE_BEGIN;
										retstr += tok->getString();
										break;
								}
								break;
								
									case STATE_BEGIN_COMMAND:
									{
										tok = lexer->findToken(USE_PATTERNS(start_command_patterns));
										int cmd_type = -1;
										switch (tok->getType()) {
											case TOKEN_STRING:
												for (q = 0; q < (sizeof(preprocessor_commands)/sizeof(struct token_patterns)); q++) {
													if (tok->getString() == preprocessor_commands[q].chars) {
														cmd_type = preprocessor_commands[q].token;
													}
												}
												switch (cmd_type) {
													case TOKEN_DEFINE:
														state = this->defineMacro(lexer);
														break;
														
													case TOKEN_UNDEF:
														state = this->undefineMacro(lexer);
														break;
														
													case TOKEN_IFDEF:
														state = this->ifDefined(lexer, true);
														break;
														
													case TOKEN_IFNDEF:
														state = this->ifDefined(lexer, false);
														break;
														
													case TOKEN_ELSE:
														state = this->elseIf(lexer);
														break;
														
													case TOKEN_ENDIF:
														state = this->endIf(lexer);
														break;
														
													case TOKEN_INCLUDE:
														state = this->include(lexer);
												}
										}
										break;
									}	
									
													default:
														this->emitError(std::string("Unhandled state ") + lexer_state_names[state] + " reached! Aborting!");
				}
			} else {
				switch (state) {
					case STATE_BEGIN:
						tok = lexer->findToken(USE_PATTERNS(begin_patterns));
						switch (tok->getType()) {
							case TOKEN_HASH:
								state = STATE_BEGIN_COMMAND;
								break;
								
							default:
								state = STATE_LINE;
								break;
						}
						break;
						
							case STATE_BEGIN_COMMAND:
							{
								tok = lexer->findToken(USE_PATTERNS(start_command_patterns));
								int cmd_type = -1;
								switch (tok->getType()) {
									case TOKEN_STRING:
										for (q = 0; q < (sizeof(preprocessor_commands)/sizeof(struct token_patterns)); q++) {
											if (tok->getString() == preprocessor_commands[q].chars) {
												cmd_type = preprocessor_commands[q].token;
											}
										}
										switch (cmd_type) {
											case TOKEN_IF:
												this->conditions.push_back(StaticCondition(false));
												state = STATE_LINE;
												break;
												
											case TOKEN_ELSE:
												state = this->elseIf(lexer);
												break;
												
											case TOKEN_ENDIF:
												state = this->endIf(lexer);
												break;
												
											default:
												//											printf("Handled token %s\n", tok->getString().c_str());
												state = STATE_LINE;
										}
								}
								break;
							}
							
											case STATE_LINE:
												tok = lexer->findToken(USE_PATTERNS(line_patterns));
												switch (tok->getType()) {
													case TOKEN_EOL:
														state = STATE_BEGIN;
														break;
														
													default:
														break;
												}
				}
			}
			if (tok != NULL) {
				if (tok->getType() == TOKEN_EOF) {
					delete tok;
					break;
				}
				delete tok;
				tok = NULL;
			}
		}
	} catch (token_exception & e) {
		fprintf(stderr, "Unexpected input in state %s!\nAvailable character classes (%d):\n", lexer_state_names[state], e.getPatternsSize());
		const struct token_patterns * patterns = e.getPatterns();
		for (int q = 0; q < e.getPatternsSize(); q++) {
			fprintf(stderr, " token %s pattern: %s\n", lexer_token_names[patterns[q].token], patterns[q].chars);
		}
		fprintf(stderr, "received char '%c' which doesn't belong to any class\n", e.getStopChar());
		return retstr;
	} catch (file_not_found_exception & e) {
		this->emitError(e.what());
	}
	return retstr;
}

/** Najde subor v zozname ciest.
 * @param paths zoznam moznych ciest, kde subor moze byt
 * @param file_name nazov suboru
 * @return nazov suboru aj s cestou v ktorej sa nachadza
 */
std::string Preprocessor::findFile(const std::set< std::string >& paths, std::string file_name) const {
	FILE * f;
	for (std::set<std::string>::iterator it = paths.begin(); it != paths.end(); ++it) {
		std::string fn = *it + "/" + file_name;
		//		printf("Trying %s\n", fn.c_str());
		f = fopen(fn.c_str(), "r");
		if (f != NULL) {
			fclose(f);
			return fn;
		}
	}
	throw file_not_found_exception();
}
