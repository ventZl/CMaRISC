#include <sstream>
#include "assembly.h"

std::string convertInt(int number) {
	std::stringstream ss;//create a stringstream
	ss << number;//add number to the stream
	return ss.str();//return a string with the contents of the stream
}

std::string opcode(std::string opcode, REGISTER reg1, REGISTER reg2, std::string comment) {
	return std::string("\t") + opcode + std::string(" R") + convertInt(reg1) + std::string(", R") + convertInt(reg2) + (comment != "" ? std::string("\t; ") + comment : "") + std::string("\n");
}

std::string opcode(std::string opcode, REGISTER reg1, unsigned indir, REGISTER reg2, std::string comment) {
	std::string indir_reg;
	indir_reg = std::string("[") + (indir == PRE_DECREMENT ? std::string("--R") : std::string("R")) + convertInt(reg1) + (indir == POST_INCREMENT ? std::string("++") : std::string("")) + std::string("]");
	return std::string("\t") + opcode + std::string(" ") + indir_reg + std::string(", R") + convertInt(reg2) + (comment != "" ? std::string("\t; ") + comment : "") + std::string("\n");
}

std::string opcode(std::string opcode, REGISTER reg, std::string comment) {
	return std::string("\t") + opcode + std::string(" R") + convertInt(reg) + (comment != "" ? std::string("\t; ") + comment : "") + std::string("\n");
}

std::string opcodeN(std::string opcode, std::string comment) {
	return std::string("\t") + opcode + (comment != "" ? std::string("\t; ") + comment : "") + std::string("\n");
}

std::string opcodeC(std::string opcode, REGISTER reg, unsigned constant, std::string comment) {
	return std::string("\t") + opcode + std::string(" R") + convertInt(reg) + std::string(", ") + convertInt(constant) + (comment != "" ? std::string("\t; ") + comment : "") + std::string("\n");
}

std::string label(std::string label) {
	return label + std::string(":\n");
}

std::string opcodeL(std::string opcode, REGISTER reg, std::string label, std::string comment) {
	return std::string("\t") + opcode + std::string(" R") + convertInt(reg) + std::string(", ") + label + (comment != "" ? std::string("\t; ") + comment : "") + std::string("\n");
}

std::string opcodeL(std::string opcode, std::string label, std::string comment) {
	return std::string("\t") + opcode + std::string(" ") + label + (comment != "" ? std::string("\t; ") + comment : "") + std::string("\n");
}

