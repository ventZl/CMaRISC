#ifndef ___MCC_ASSEMBLY_H__
#define ___MCC_ASSEMBLY_H__

#include <string>

#define REG_SP ((REGISTER) 13)
#define REG_BP ((REGISTER) 12)


typedef unsigned short REGISTER;

enum indirection_opcode { INDIRECT, PRE_DECREMENT, POST_INCREMENT };

std::string convertInt(int number);
std::string opcode(std::string opcode, REGISTER reg1, REGISTER reg2, std::string comment = "");
std::string opcode(std::string opcode, REGISTER reg1, unsigned indir, REGISTER reg2, std::string comment = "");
std::string opcode(std::string opcode, REGISTER reg, std::string comment = "");
std::string opcodeN(std::string opcode, std::string comment = "");
std::string opcodeC(std::string opcode, REGISTER reg, unsigned constant, std::string comment = "");
std::string label(std::string label);
std::string opcodeL(std::string opcode, REGISTER reg, std::string label, std::string comment = "");
std::string opcodeL(std::string opcode, std::string label, std::string comment = "");


#endif
