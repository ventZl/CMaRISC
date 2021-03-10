#ifndef __SUNBLINDCTL_SRC_MPP_MACRO_H__
#define __SUNBLINDCTL_SRC_MPP_MACRO_H__

#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>

class LexicalAnalyzer;

class Macro {
public:
	Macro(std::string name) { this->name = name; }
	virtual ~Macro() {}
	void addArgName(std::string name) { this->args.push_back(name); }
	virtual void setBody(std::string body) { this->body = body; }
	virtual std::string getBody() { return this->body; }
	std::string getName() const { return this->name; }
	std::string getDefinitionPlace() const { return this->definition_place; }
	int getArgCount() const { return this->args.size(); }
	std::string getArgName(int no) const { return this->args[no]; }
	
protected:
	Macro() {}
	
	std::string name;
	std::vector<std::string> args;
	std::string body;
	std::string definition_place;
};

class BuiltinMacro: public Macro {
public:
	BuiltinMacro(std::string name): Macro(name) { this->definition_place = "(builtin)"; }
	
	virtual void setBody(std::string body) { throw std::runtime_error("Cannot redeclare builtin macro " + this->name + "!"); }
};

class LineMacro: public BuiltinMacro {
public:
	LineMacro(std::string name, LexicalAnalyzer * lexer): BuiltinMacro(name), lexer(lexer) {}
	virtual std::string getBody();
	
protected:
	unsigned * line;
	LexicalAnalyzer * lexer;
};

class FileMacro: public BuiltinMacro {
public:
	FileMacro(std::string name, LexicalAnalyzer * lexer): BuiltinMacro(name), lexer(lexer) {}
	virtual std::string getBody();
	
	//	void setFile(std::string file) { this->file = lexer->getCurrentFile(); }
	
	protected:
		LexicalAnalyzer * lexer;
		
};
		
class ConstMacro: public BuiltinMacro {
public:
	ConstMacro(std::string name, std::string value): BuiltinMacro(name) { this->body = value; }
	
};

class CounterMacro: public BuiltinMacro {
public:
	CounterMacro(std::string name): BuiltinMacro(name), counter(0) {}
	
	virtual std::string getBody() { std::stringstream ss; ss << this->counter++; std::string rs = ss.str(); return rs; }
	
protected:
	int counter;
};

#endif