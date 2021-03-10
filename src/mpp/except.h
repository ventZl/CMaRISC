#ifndef __SUNBLINDCTL_SRC_MPP_EXCEPT_H__
#define __SUNBLINDCTL_SRC_MPP_EXCEPT_H__

#include <stdexcept>

class eof_exception: public std::runtime_error {
public:
	eof_exception(): std::runtime_error("Nothing to read") {}
};

class token_exception: public std::runtime_error {
public:
	token_exception(const struct token_patterns * patterns, unsigned patterns_size, char stopchar): std::runtime_error("Unexpected input - doesn't belong to any token character class"), patterns(patterns), patterns_size(patterns_size), stopchar(stopchar) {}
	const struct token_patterns * getPatterns() { return this->patterns; }
	unsigned getPatternsSize() { return this->patterns_size; }
	char getStopChar() { return this->stopchar; }
	
protected:
	const struct token_patterns * patterns;
	unsigned patterns_size;
	char stopchar;
};

class invert_exception: public std::runtime_error {
public:
	invert_exception(): std::runtime_error("Conditional already inverted") {};
};
	
class condition_exception: public std::runtime_error {
public:
	condition_exception(): std::runtime_error("Conditional already inverted") {};
};
	
class file_not_found_exception: public std::runtime_error {
public:
	file_not_found_exception(): std::runtime_error("File not found in given path") {};
};
	

#endif