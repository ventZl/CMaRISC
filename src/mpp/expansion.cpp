#include <sstream>

#include "macro.h"
#include "except.h"
#include "expansion.h"

enum replacement_states { REPLACEMENT_BEGIN, REPLACEMENT_NORMAL, REPLACEMENT_WHITESPACE, REPLACEMENT_STRINGIFY, REPLACEMENT_JOIN, REPLACEMENT_NONWHITESPACE };

/** Spocita vyskyt danych znakov v retazci.
 * @param str retazec
 * @param start_pos pociatocna pozicia pre spocitavanie
 * @param end_pos konecna pozicia pre spocitavanie
 * @param c znak, ktory sa hlada
 * @return pocet vyskytov znaku v retazci na vymedzenych poziciach
 */
int count_chars(std::string str, size_t start_pos, size_t end_pos, char c) {
	int count = 0;
	for (int q = start_pos; q < end_pos; q++) {
		if (str[q] == c) count++;
	}
	return count;
}

/** Vrati prelozene expandovane makro.
 * Pri expandovani sa:
 * * nahradia argumenty makra parametrami pri volani
 * * vykona stringifikacia parametrov makra
 * * vykona spojenie tokenov indikovane tokenom ##
 * @return expandovane makro
 */
std::string MacroExpansion::getBody() {
	if (this->replacements.size() != this->macro->getArgCount()) {
		std::stringstream ss;
		ss << this->replacements.size();
		std::string arg_passed = ss.str();
		ss.flush();
		ss << this->macro->getArgCount();
		std::string arg_takes = ss.str();
		throw std::runtime_error("Macro \"" + this->macro->getName() +"\" passed " + arg_passed + " but takes only " + arg_takes);
	}
	
	std::string body = this->macro->getBody();
	int arg_pos, arg_start = 0;
	
	for (int q = 0; q < this->macro->getArgCount(); q++) {
		do {
			bool stringify = false;
			arg_pos = body.find(this->macro->getArgName(q), arg_start);
			if (arg_pos != std::string::npos) {
				arg_start = arg_pos;
				std::string replacement = this->replacements[q];
				size_t nwcp;
				if (arg_pos > 0) {
					nwcp = body.find_last_not_of(" \t\a\b", arg_pos - 1);	// non whitespace character position
					if (nwcp != std::string::npos && body[nwcp] == '#') {
						if (nwcp == 0 || (nwcp > 0 && body[nwcp - 1] != '#')) {	// pred nahradou makra je #, treba stringify
							stringify = true;
							replacement = "\"" + replacement + "\"";
						}
					}
				}
				if (count_chars(body, 0, arg_pos, '"') % 2 == 0) {
					body.replace(arg_pos, this->macro->getArgName(q).length(), replacement);
					if (stringify) {
						// este ideme odstranit samotnu mrezu z vystupu
						body.replace(nwcp, arg_pos - nwcp, "");
						arg_start += 2 - (arg_pos - nwcp);
					}
				}
				arg_start += this->macro->getArgName(q).length();
			}
		} while (arg_pos != std::string::npos);
	}
	
	arg_start = 0;
	
	do {
		arg_pos = body.find("##", arg_start);
		if (arg_pos != std::string::npos) {
			size_t whitespace_begin, whitespace_end;
			if (arg_pos != 0) {
				whitespace_begin = body.find_last_not_of(" \t\a\b", arg_pos - 1);
			}
			whitespace_end = body.find_first_not_of(" \t\a\b", arg_pos + 2);
			if (whitespace_begin != std::string::npos && whitespace_end != std::string::npos) {
				body.replace(whitespace_begin + 1, (whitespace_end - whitespace_begin) - 1, "");
			}
		}
	} while (arg_pos != std::string::npos);
	
	return body;
}