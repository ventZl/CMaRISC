#ifndef __PREPROCESS_H__
#define __PREPROCESS_H__

#include <set>
#include <map>
#include <string>
#include "lexer.h"
#include "macro.h"

enum lexer_state { STATE_BEGIN, STATE_LINE, STATE_BEGIN_COMMAND, STATE_COMMAND, STATE_MACRO_CALL };
enum lexer_token { TOKEN_WHITESPACE, TOKEN_VERBATIM, TOKEN_HASH, TOKEN_STRING, TOKEN_LBR, TOKEN_RBR, TOKEN_LBR_SHARP, TOKEN_RBR_SHARP, TOKEN_QUOTE,
TOKEN_DEFINE, TOKEN_UNDEF, TOKEN_IF, TOKEN_ELSE, TOKEN_ENDIF, TOKEN_INCLUDE, TOKEN_WARNING, TOKEN_ERROR, TOKEN_IFDEF, TOKEN_IFNDEF, TOKEN_NUMBER, TOKEN_EOF, TOKEN_EOL, TOKEN_COMMA
};

typedef std::map<std::string, Macro *> macro_list;
typedef macro_list::iterator macro_list_iterator;

struct token_patterns {
	const char * chars;
	int token;
};

class Token;

struct token_run {
	Token * token;
	const struct token_patterns * patterns;
	int patterns_count;
	int len_limit;
};

#define ANY_LENGTH		-1

class Preprocessor {
public:
	Preprocessor(std::string entry_file);
	~Preprocessor();
	std::string preprocess();
	
	void addIncludePath(std::string path) { this->include_paths.insert(path); }
	void addMacro(Macro * macro);
	
protected:
	int preprocessRun(LexicalAnalyzer* lexer, token_run* run, unsigned int run_size);
	
	void emitError(std::string err_str);
	void emitWarning(std::string err_str);
	
	int defineMacro(LexicalAnalyzer * lexer);
	int undefineMacro(LexicalAnalyzer * lexer);
	int ifDefined(LexicalAnalyzer* lexer, bool defined);
	int elseIf(LexicalAnalyzer * lexer);
	int endIf(LexicalAnalyzer * lexer);
	void callMacro(LexicalAnalyzer* lexer, Macro* macro);
	int include(LexicalAnalyzer * lexer);
	std::string findFile(const std::set<std::string> & paths, std::string file_name) const;
	std::set<std::string> getIncludePaths() const { return this->include_paths; }
	std::set<std::string> getCurrentPath() const { std::set<std::string> path; path.insert("."); return path; }
	
	void enterConditional(bool satisfied) { this->conditions.push_back(Condition(satisfied)); }
	void exitConditional() { if (this->conditions.empty()) throw condition_exception(); this->conditions.pop_back(); }
	void invertConditional() { if (this->conditions.empty()) throw condition_exception(); this->conditions.back().invert(); }
	bool isOutputPermitted() const { if (this->conditions.empty()) return true; return this->conditions.back().getValue(); }
	
	std::vector<Condition> conditions;
	macro_list macros;
	std::string file;
	std::set<std::string> include_paths;
	
	FileMacro * file_macro;
	ConstMacro * entry_file_macro;
	LineMacro * line_macro;
	CounterMacro * counter_macro;
	
	LexicalAnalyzer* lexer;

	bool skip;
};

#endif
