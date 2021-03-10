#ifndef __MCC_EXCEPTION_H___
#define __MCC_EXCEPTION_H___

#include <stdexcept>

class SemanticException: public std::exception {
public:
	SemanticException(std::string message): message(message) {}
	~SemanticException() throw() {}
	virtual const char* what() const throw() { return this->message.c_str(); }
	
protected:
	std::string message;
};

#endif