#ifndef __SUNBLINDCTL_SRC_MPP_EXPANSION_H__
#define __SUNBLINDCTL_SRC_MPP_EXPANSION_H__

#include <vector>
#include <string>

class Macro;

class MacroExpansion {
public:
	MacroExpansion(Macro * macro): macro(macro) {}
	void addParameterReplacement(std::string replacement) { this->replacements.push_back(replacement); }
	std::string getBody();
	
protected:
	Macro * macro;
	std::vector<std::string> replacements;
};

#endif