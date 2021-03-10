#ifndef __SUNBLINDCTL_SRC_MPP_LEXER_H__
#define __SUNBLINDCTL_SRC_MPP_LEXER_H__

#include <vector>
#include <map>
#include <string>
#include <stdio.h>
#include "except.h"

class Macro;

typedef std::map<std::string, Macro *> macro_list;
typedef macro_list::iterator macro_list_iterator;

class Token {
public:
	Token(int type, std::string string): type(type), string(string) {}
	Token(int type, int number): type(type), number(number) {}
	int getType() const { return this->type; }
	std::string getString() const { return this->string; }
	
protected:
	int type;
	std::string string;
	int number;
};

class Condition {
public:
	Condition(bool value): value(value), inverted(false) {}
	bool getValue() const { return this->value; }
	virtual void invert() { if (!this->inverted) { this->inverted = true; this->value = !this->value; } else throw invert_exception(); return; }
	
protected:
	bool value;
	bool inverted;
};

class StaticCondition: public Condition {
public:
	StaticCondition(bool value): Condition(value) {}
	virtual void invert() { if (!this->inverted) { this->inverted = true; } else throw invert_exception(); return;return; }
};

class LexicalAnalyzer {
public:
	LexicalAnalyzer(): recording(false) { std::vector<char> v; this->char_buff.push_back(v); }
	void pushSource(FILE * f, std::string name) { this->file_stack.push_back(f); std::vector<char> v; this->char_buff.push_back(v); this->file_name_stack.push_back(name); this->file_line_stack.push_back(1); return; }
	char getChar();
	void pushChar(char c) { if (c == '\n' || c == '\r') this->decrementLine(); this->char_buff.back().push_back(c); }
	void pushString(std::string str);
	Token * findToken(const token_patterns patterns[], unsigned int size, int maxlen = -1);
	void commitRecord();
	void beginRecord() { if (!this->recording) this->recording = true; else std::runtime_error("Recording already running."); }
	void rollbackRecord();
	std::string getCurrentFile() const { if (this->file_name_stack.empty()) return "(unknown)"; return this->file_name_stack.back(); }
	unsigned getCurrentLine() const { if (this->file_line_stack.empty()) return 0; return this->file_line_stack.back(); }
	
protected:
	bool classificateCharacter(char c, const char* pattern);
	Token * matchCommand(std::string str);
	Token * matchMacro(std::string str);
	
	void incrementLine() ;
	void decrementLine() ;
	
	bool recording;
	std::vector<char> recorder;
	std::vector<std::vector<char> > char_buff;
	std::vector<FILE *> file_stack;
	std::vector<std::string> file_name_stack;
	std::vector<unsigned> file_line_stack;
};

#endif