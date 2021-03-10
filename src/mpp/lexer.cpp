#include "lexer.h"
#include "preprocess.h"
#include <string.h>

/** Vrati jeden znak.
 * Ak boli nejake znaky natlacene do vstupneho buffera, vracia tieto znaky
 * Ak neboli, ziskava znaky priamo zo suboru, ktory je na vrchu file stacku.
 * @return vstupny znak
 */
char LexicalAnalyzer::getChar() {
	char c;
	if (!this->char_buff.empty() && !this->char_buff.back().empty()) {
		c = this->char_buff.back().back();
		this->char_buff.back().pop_back();
	} else {
		int count;
		if (this->file_stack.empty()) throw eof_exception();
		count = fread(&c, 1, 1, this->file_stack.back());
		if (count != 1) {
			fclose(this->file_stack.back());
			this->file_stack.pop_back();
			this->char_buff.pop_back();
			this->file_name_stack.pop_back();
			this->file_line_stack.pop_back();
			return this->getChar();
		}
	}
	if (c == '\n' || c == '\r') this->incrementLine();
	return c;
}

/** Natlaci do vstupneho buffera obsah retazca.
 * Pouziva sa pri expanzii makier.
 * @param str retazec, ktory ma byt natlaceny do buffera
 */
void LexicalAnalyzer::pushString(std::string str) {
	//	printf("Pushing back string '%s'\n", str.c_str());
	for (int q = str.length() - 1; q >= 0; q--) {
		this->pushChar(str[q]);
	}
}

/** Unused */
void LexicalAnalyzer::commitRecord() {
	this->recording = false;
	this->recorder.clear();
	return;
}

/** Unused */
void LexicalAnalyzer::rollbackRecord() {
	std::vector<char>::reverse_iterator it = this->recorder.rbegin();
	for (; it != this->recorder.rend(); ++it) {
		this->char_buff.back().push_back(*it);
	}
	this->recording = false;
	this->recorder.clear();
	return;
}

enum match_class { CLASS_ALPHA_BIG, CLASS_ALPHA_SMALL, CLASS_NUMBER, CLASS_WHITESPACE, CLASS_NEWLINE, CLASS_VERBATIM, CLASS_UNKNOWN };

/** Zisti, ci znak patri do vzorky znakov.
 * Vrati true, ak znak zodpoveda vzorke.
 * @return true ak znak zodpoveda niektoremu zo znakov vzorky, inac false
 */
bool LexicalAnalyzer::classificateCharacter(char c, const char * pattern) {
	int cls;
	bool matches = false;
	for (unsigned int q = 0; q < strlen(pattern); q++) {
		if (c >= 'A' && c <= 'Z') cls = CLASS_ALPHA_BIG;
		else if (c >= 'a' && c <= 'z') cls = CLASS_ALPHA_SMALL;
		else if (c >= '0' && c <= '9') cls = CLASS_NUMBER;
		else if (c == ' ' || c == '\t') cls = CLASS_WHITESPACE;
		else if (c == '\n' || c == '\r') cls = CLASS_NEWLINE;
		else cls = CLASS_VERBATIM;
		
		/* rozne skupiny znakov */
		if (pattern[q] == 'A') { 
			if (cls == CLASS_ALPHA_BIG) matches = true;
		} else if (pattern[q] == 'a') { 
			if (cls == CLASS_ALPHA_SMALL) matches = true;
		} else if (pattern[q] == '0') { 
			if (cls == CLASS_NUMBER) matches = true;
		} else if (pattern[q] == ' ') { 
			if (cls == CLASS_WHITESPACE) matches = true;
		} else if (c == pattern[q]) matches = true;
		else if (pattern[q] == '*' /* && cls == CLASS_VERBATIM*/) matches = true;
	}
	return matches;
}

/** Najde nasledujuci token.
 * Vrati token v zavislosti od toho, ake predpisy tokenov su dane na vstup.
 * @param patterns vzorky pre tokeny
 * @param size pocet vzoriek
 * @param maxlen maximalna dlzka tokenu
 * @return token
 */
Token * LexicalAnalyzer::findToken(const struct token_patterns patterns[], unsigned size, int maxlen) {
	//	usleep(50000);
	/*	fprintf(stderr, "Available character classes (%d):\n", size);
	 *	for (int q = 0; q < size; q++) {
	 *		fprintf(stderr, " token %s pattern: %s\n", lexer_token_names[patterns[q].token], patterns[q].chars);
	 }
	 */
	int matched = -1;				// offset patternu, ktory matchol ako posledny
	std::string str;
	char c;
	while (1) {
		try {
			c = this->getChar();
			//			printf("Read characted %c, matched = %s\n", c, matched == -1 ? "(none)" : lexer_token_names[patterns[matched].token]);
		} catch (eof_exception & e) {
			if (matched != -1) {
				return new Token(patterns[matched].token, str);
			} else {
				return new Token(TOKEN_EOF, "");
			}
		}
		
		bool something_matched = false; // priznak, ci aspon niektory pattern matchol
		for (unsigned q = 0; q < size; q++) {
			bool skip = false;
			bool matches = this->classificateCharacter(c, patterns[q].chars);
			
			if (matches) {
				//				printf("Something matched\n");
				something_matched = true;
				if (q != matched && matched != -1) {
					// matchlo to iny token, ako bol uz predtym matchnuty, takze vratime predosly token a tento budeme rematchovat pri dalsom poziadavku
					pushChar(c);
					return new Token(patterns[matched].token, str);
				} else if (q == matched || matched == -1) {
					// matchlo to bud ten isty, alebo uplne novy token
					str += c;
					matched = q;
					skip = true;			// triedy musia byt disjunktne (okrem triedy *), takze dalsie uz hladat netreba
				}
			}
			//			printf("str.length() = %d\tmaxlen = %d\n", str.length(), maxlen);
			if (maxlen > 0 && str.length() == maxlen) {
				return new Token(patterns[matched].token, str);
			}
			if (skip) break;		// skipneme dalsie hladanie z akehokolvek udaneho dovodu
		}
		if (!something_matched) {
			// tento znak nepatri do ziadnej podporovanej triedy znakov pre token
			if (matched == -1) {
				// znak nematchol, ale nie je ani nic namatchovane, takze je to na tomto mieste uplne zly znak -> chyba
				//				printf("Nothing matched!\n");
				pushChar(c);		// pushnutie znaku nazad umozni jeho reevaluaciu s inymi vzormi
				// ak sa v tomto bode matched == -1, znamena to, ze vstupny znak nevyhovuje ziadnemu vzoru -> chyba
				throw token_exception(patterns, size, c);
			} else {
				// znak nematchol, ale nieco uz namatchovane je, znak natlacime nazad do buffera a vratime, co uz je namatchovane
				pushChar(c);
				return new Token(patterns[matched].token, str);
			}
		}
	}
}

/** Inkrementuje pocitadlo riadkov
 */
void LexicalAnalyzer::incrementLine() {
	unsigned l = this->file_line_stack.back();
	this->file_line_stack.pop_back();
	this->file_line_stack.push_back(++l);
	return;
}

/** Dekrementuje pocitadlo riadkov
 */
void LexicalAnalyzer::decrementLine() {
	unsigned l = this->file_line_stack.back();
	this->file_line_stack.pop_back();
	this->file_line_stack.push_back(--l);
	return;
}
