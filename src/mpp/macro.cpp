#include "macro.h"
#include "lexer.h"

/** Vrati nazov aktualne spracovavaneho suboru.
 * @return nazov aktualneho suboru
 */
std::string FileMacro::getBody() {
	return this->lexer->getCurrentFile();
}

/** Vrati aktualne cislo riadka
 * @return aktualne cislo riadka v stringovej forme
 */
std::string LineMacro::getBody() {
	std::stringstream ss;
	ss << this->lexer->getCurrentLine();
	std::string rs = ss.str();
	return rs;
}
