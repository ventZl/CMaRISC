#include <stdlib.h>

#include <object.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static char _be_verbose = 0;

/** Nastavi "ukecanost" operacii na uroven.
 * @param level uroven ukecanosti.
 */
void objects_be_verbose(int level) {
	_be_verbose = level;
	return;
}

/** Vytvori novy objekt reprezentujuci sekciu programu
 * Jedna sekcia sa moze pouzit napriklad na ulozenie kodu, alebo dat zodpovedajucich jednemu assemblerovemu vstupu.
 * @param name nazov sekcie. bezne je .text pre kod a .data pre data
 * @return popisovac sekcie
 */
SECTION * section_create(const char * name) {
	SECTION * new_section = malloc(sizeof(SECTION));
	memset(new_section, 0, sizeof(SECTION));
	new_section->name = strdup(name);
}

/** Vlozi na koniec sekcie dalsie data
 * Normalne sa data pridavaju iba na koniec sekcie. Funkcia automaticky
 * resizne sekciu o dlzku vkladanych dat.
 * @param section sekcia do ktorej maju byt data pridane
 * @param data pointer na vkladane data
 * @param length dlzka vkladanych dat
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
int section_append_data(SECTION * section, const unsigned char * data, unsigned length) {
	if (section == NULL) {
		fprintf(stderr, "internal ERROR: section is NULL\n");
		exit(1);
	}
	section->data = realloc(section->data, section->size + length);
	memcpy(&(section->data[section->size]), data, length);
	section->size += length;
	if (_be_verbose > 2) printf("New section '%s' size is %d\n", section->name, section->size);
	return 1;
}

/** Skopiruje data sekcie.
 * @param section sekcia, ktora sa ma kopirovat
 * @param dest cielova oblast v pamati
 * @param size velkost cielovej oblasti
 * @return 0 ak sa sekcia skopirovala, inac -1
 */
int section_data_copy(SECTION * section, unsigned char * dest, unsigned size) {
	if (section->size <= size) {
		memcpy(dest, section->data, section->size);
	} else return -1;
	return 0;
}
/** Nasledujuca adresa v sekcii
 * Vrati adresu na ktoru bude sledovat najblizsie vlozenie dat do sekcie
 * @param section sekcia
 * @return adresa (defakto dlzka sekcie)
 */
ADDRESS section_get_next_address(const SECTION * section) {
	return section->size;
}

/** Zapise binarne data sekcie do suboru.
 * Data su zapisane v presne takom stave v akom sa nachadzaju. Ak este nebola vykonana 
 * relokacia, budu na mieste vsetkych relokovanych dat nuly.
 * @param section pointer na popisovac sekcie
 * @param filename nazov suboru do ktoreho sa sekcia zapise
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
int section_dump(SECTION * section, const char * filename) {
	int fd = open(filename, O_WRONLY | O_CREAT, 0644);
	int cursor = 0, cw;
	while (cursor < section->size) {
		cw = write(fd, &(section->data[cursor]), (section->size - cursor > 4096 ? 4096 : section->size - cursor));
		if (cw < 0) {
			if (_be_verbose > 2) perror("Section data write\n");
			exit(1);
		} else {
			cursor += cw;
		}
	}
	close(fd);
	return 1;
}

/** Zapise binarne data sekcie do suboru.
 * Data su zapisane v presne takom stave v akom sa nachadzaju. Ak este nebola vykonana 
 * relokacia, budu na mieste vsetkych relokovanych dat nuly.
 * @param section pointer na popisovac sekcie
 * @param filename nazov suboru do ktoreho sa sekcia zapise
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
int binary_write(SECTION * section, ADDRESS entrypoint, const char * filename) {
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) return 0;
	int cursor = 0, cw;
	char epbyte;
	if (write(fd, "BIN", 3) == -1) { close(fd); return 0; }
	epbyte = entrypoint >> 8;
	if (write(fd, &epbyte, 1) == -1) { close(fd); return 0; }
	epbyte = entrypoint & 0xFF;
	if (write(fd, &epbyte, 1) == -1) { close(fd); return 0; }
	while (cursor < section->size) {
		cw = write(fd, &(section->data[cursor]), (section->size - cursor > 4096 ? 4096 : section->size - cursor));
		if (cw < 0) {
			if (_be_verbose > 2) perror("Section data write\n");
			exit(1);
		} else {
			cursor += cw;
		}
	}
	close(fd);
	return 1;
}

/** Nacita objektovy subor do pamate a vrati adresu na ktorej sa ma zacat vykonavanie programu.
 * @param filename nazov suboru z ktoreho sa objekt nacita
 * @param memory adresa pre virtualnu pamat
 * @param entrypoint miesto, kam bude ulozena pociatocna adresa vykonavania (relativna k pociatku virtualnej pamate)
 * @param memsize velkost virtualnej pamate
 * @return chybovy kod, 0 znamena ziadnu chybu, -1 chybu pri zistovani vlastnosti suboru alebo nespravnu signaturu, -2 chybne nacitanu adresu, -3 chybu pri citani obrazu zo suboru, -4 prilis velky objekt vzhladom k velkosti vyhradenej pamate, -5 neexistujuci subor
 */
int binary_read(const char * filename, unsigned char * memory, ADDRESS * entrypoint, unsigned memsize) {
	char epbyte, is_ok;
	char signature[3];
	struct stat vmm_stat;
	
	int fd = open(filename, O_RDONLY);
	if (fd != -1) {
		if (fstat(fd, &vmm_stat) == -1) {
			close(fd);
			fprintf(stderr, "Unable to fstat() virtual memory image file\n");
			return -1;
		}
		
		read(fd, signature, 3);
		
		if (strncmp(signature, "BIN", 3) != 0) {
			return -1;
		}
		
		do {
			*entrypoint = 0;
			if (read(fd, &epbyte, 1) == -1) break;
				*entrypoint = (*entrypoint << 8) | epbyte;
			if (read(fd, &epbyte, 1) == -1) break;
				*entrypoint = (*entrypoint << 8) | epbyte;
			is_ok = 1;
		} while (0);
		if (is_ok == 0) {
			close(fd);
			fprintf(stderr, "Unable to read entrypoint from image!\n");
			return -2;
		}
		if (vmm_stat.st_size - 5 > memsize) {
			fprintf(stderr, "Image larger than memory size!\n");
			return -4;
		}
		int cursor = 0, remaining = vmm_stat.st_size - 5, rd;
		while (remaining > 0) {
			rd = read(fd, &(memory[cursor]), (remaining > 4096 ? 4096 : remaining));
			if (rd != -1) { cursor += rd; remaining -= rd; }
			else {
				fprintf(stderr, "Unable to read virtual memory data from image!\n");
				return -3;
			}
		}
	} else {
		fprintf(stderr, "Unable to open virtual memory image file\n");
		return -5;
	}
	return 0;
}

/** Najde symbol v sekcii
 * Najde symbol v sekcii podla jeho nazvu. 
 * @param name nazov, ktory sa hlada. Implementacia obmedzuje nazvy na max. 32 znakov (resp. 31 znakov) 
 * @return pointer na popisovac symbolu, alebo NULL ak sa taky symbol nenasiel
 */
static SYMBOL * symbol_find(SECTION * section, unsigned char * name) {
	int q;
	for (q = 0; q < section->symbol_count; q++) {
		if (strcmp(section->symbols[q].name, name) == 0) return &(section->symbols[q]);
	}
	return NULL;
}

/** Vytvori popisovac symbolu
 * Vytvori novy popisovac symbolu a zapise donho nazov
 * @param section sekcia v ktorej ma byt symbol alokovany
 * @param name nazov, ktory ma symbol mat
 * @return adresa popisovaca symbolu
 */
static SYMBOL * symbol_create(SECTION * section, unsigned char * name) {
	section->symbols = realloc(section->symbols, (section->symbol_count + 1) * sizeof(SYMBOL));
	memset(&(section->symbols[section->symbol_count]), 0, sizeof(SYMBOL));
	section->symbols[section->symbol_count].name = strdup(name);
	return &(section->symbols[section->symbol_count++]);
}

/** Nastavi vlastnosti symbolu
 * Umoznuje zmenit adresu a flagy symbolu
 * @param section sekcia v ktorej sa ma symbol zmenit
 * @param name nazov symbolu, ktory sa meni
 * @param address nova adresa na ktoru ma byt hodnota symbolu nastavena
 * @param flags momentalne 0, pre buduce pouzitie
 * @return adresa symbolu, ktory sa zmenil, pripadne vytvoril
 */
SYMBOL * symbol_set(SECTION * section, unsigned char * name, ADDRESS address, uint8_t flags) {
	SYMBOL * symbol = symbol_find(section, name);
	if (symbol == NULL) symbol = symbol_create(section, name);
	symbol->address = address;
	symbol->flags = flags;
	return symbol;
}

/** Vrati adresu symbolu
 * Ak symbol zatial nie je resolvovany, alebo neexistuje, vrati 0xFFFF.
 * @param section pointer na popisovac sekcie v ktorej sa symbol bude hladat
 * @param symbol nazov symbolu, ktoreho adresa sa ma zistit
 * @return adresa symbolu
 */
ADDRESS symbol_get_address(SECTION * section, unsigned char * symbol) {
	SYMBOL * sym = symbol_find(section, symbol);
	if (sym == NULL) {
		if (_be_verbose > 1) fprintf(stderr, "Symbol '%s' not found!\n", symbol);
		return 0xFFFF;
	}
	return sym->address;
}

/** Prida k symbolu novu relokaciu. Interna implementacia.
 * @param symbol symbol ktoremu ma byt pridana relokacia
 * @param position adresa na ktorej sa relokacia nachadza
 * @param base adresa ku ktorej sa vztiahne relokacia. Pouziva sa ak sa adresa symbolu berie relativne, nie absolutne
 * @param shift pocet bitov o ktore sa vypocitana adresa posunie doprava
 * @param bits pocet bitov, ktore sa z adresy pouziju
 * @param sign_pos bit na ktory sa ulozi znamienko, ak sa znamienko nema ukladat, nastavit na 0xFF
 * @return chybovy kod, 1 znaci ziadnu chybu
 */
static int relocation_append(SYMBOL * symbol, ADDRESS position, ADDRESS base, uint8_t shift, uint8_t bits, uint8_t sign_pos) {
	symbol->relocations = realloc(symbol->relocations, (symbol->relocation_count + 1) * sizeof(RELOCATION));
	symbol->relocations[symbol->relocation_count].base = base;
	symbol->relocations[symbol->relocation_count].position = position;
	symbol->relocations[symbol->relocation_count].bits = bits;
	symbol->relocations[symbol->relocation_count].shift = shift;
	symbol->relocations[symbol->relocation_count].sign_pos = sign_pos;
	symbol->relocation_count++;
	printf("Appended relocation\n");
	return 1;
}

/** Prida k symbolu novu relokaciu. 
 * @param symbol symbol ktoremu ma byt pridana relokacia
 * @param position adresa na ktorej sa relokacia nachadza
 * @param base adresa ku ktorej sa vztiahne relokacia. Pouziva sa ak sa adresa symbolu berie relativne, nie absolutne
 * @param shift pocet bitov o ktore sa vypocitana adresa posunie doprava
 * @param bits pocet bitov, ktore sa z adresy pouziju
 * @param sign_pos bit na ktory sa ulozi znamienko, ak sa znamienko nema ukladat, nastavit na 0xFF
 * @return chybovy kod, 1 znaci ziadnu chybu
 */
int symbol_add_relocation(SECTION * section, unsigned char * name, ADDRESS position, ADDRESS base, uint8_t shift, uint8_t bits, uint8_t sign_pos) {
	SYMBOL * symbol = symbol_find(section, name);
	if (symbol == NULL) symbol = symbol_set(section, name, 0xFFFF, 0);
	return relocation_append(symbol, position, base, shift, bits, sign_pos);
}

/** Pripoji do objektoveho suboru kopiu sekcie.
 * Zdrojova sekcia je skopirovana, takze zmeny vykonane po zavolani tejto funkcie sa nemusia prejavit
 * @param object objektovy subor
 * @param section sekcia, ktora ma byt skopirovana
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
int object_add_section(OBJECT * object, const SECTION * section) {
	if (object == NULL) {
		if (_be_verbose > 3) fprintf(stderr, "internal ERROR: object is NULL\n");
		exit(1);
	}
	object->sections = realloc(object->sections, (object->section_count + 1) * sizeof(SECTION));
	memset(&(object->sections[object->section_count]), 0, sizeof(SECTION));
	section_copy(&(object->sections[object->section_count]), section);
	object->section_count++;
	return 1;
}

/** Skopiruje relokaciu.
 * @param relocation_o pointer na cielovu relokaciu
 * @param relocation_i pointer na zdrojovu relokaciu
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
static int relocation_copy(RELOCATION * relocation_o, const RELOCATION * relocation_i) {
	memcpy(relocation_o, relocation_i, sizeof(RELOCATION));
	return 1;
}

/** Uvolni relokaciu.
 * @param relocation pointer na relokaciu
 * @param ifself ak je nastavene na 1, je uvolneny aj samotny blok pamate, v opacnom pripade je relokacia iba deasociovana (pointer nastaveny na NULL)
 * @return priznak uspechu, 1 znamena uspech
 */
int relocation_free(RELOCATION * relocation, char itself) {
	if (relocation == NULL) return 1;
	if (itself) {
		free(relocation);
		relocation = NULL;
	}
	return 1;
}

/** Zmeni bazovu adresu symbolu.
 * Vyuziva sa v pripade, ze sa spoja 2 sekcie do jednej a symboly v 
 * pripajanej sekcii potrebuju pripocitat k vsetkym adresam dlzku predoslej
 * sekcie. Nie je mozne pripocitat zapornu dlzku, teda sekciu posunut na nizsie
 * adresy.
 * @param symbol symbol, ktoremu ma byt zmenena bazova adresa
 * @param new_base nova bazova adresa
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
static int symbol_rebase(SYMBOL * symbol, unsigned int new_base) {
	int q;
	if (symbol->address != 0xFFFF) symbol->address += new_base;
	
	for (q = 0; q < symbol->relocation_count; q++) {
		if (symbol->relocations[q].base != 0) symbol->relocations[q].base += new_base;
		symbol->relocations[q].position += new_base;
	}
	return 1;
}

/** Skopiruje symbol
 * Vytvori totalnu kopiu zdrojoveho symbolu, ktory s cielovym nebude zdielat ani relokacie.
 * @param symbol_o pointer na cielovy symbol
 * @param symbol_i pointer na zdrojovy symbol
 * @return chybovy kod, 1 znaci ziadnu chybu
 */
static int symbol_copy(SYMBOL * symbol_o, const SYMBOL * symbol_i) {
	int q;
	
	memset(symbol_o, 0, sizeof(SYMBOL));
	symbol_o->address = symbol_i->address;
	symbol_o->name = strdup(symbol_i->name);
	symbol_o->flags = symbol_i->flags;
	symbol_o->relocation_count = symbol_i->relocation_count;
	symbol_o->relocations = malloc(symbol_o->relocation_count * sizeof(RELOCATION));
	for (q = 0; q < symbol_o->relocation_count; q++) {
		relocation_copy(&(symbol_o->relocations[q]), &(symbol_i->relocations[q]));
	}
	return 1;
}

/** Uvolni symbol.
 * @param symbol pointer na symbol, ktory ma byt uvolneny
 * @param itself ak je nastaveny na 1, aj sa uvolni alokovana pamat, inac sa iba pointer nastavi na NULL
 * @return priznak uspesnosti, 1 znaci uspech
 */
int symbol_free(SYMBOL * symbol, char itself) {
	int q;
	if (symbol == NULL) return 1;
	for (q = 0; q < symbol->relocation_count; q++) {
		relocation_free(&(symbol->relocations[q]), 0);
	}
	free(symbol->name);
	free(symbol->relocations);
	if (itself) {
		free(symbol);
		symbol = NULL;
	}
	return 1;
}


/** Spoji 2 symboly do jedneho.
 * Vyuziva sa pri spajani viacerych sekcii, ktore mozu mat symboly s rovnakymi
 * nazvami. Zabezpeci logiku toho, ake symboly mozno vzajomne spojit.
 * @param symbol_o symbol ku ktoremu sa pripoja relokacie
 * @param symbol_appended symbol ktoreho relokacie sa budu pripajat
 * @return chybovy kod, 1 znaci ziadnu chybu
 */
static int symbol_append(SYMBOL * symbol_o, const SYMBOL * symbol_appended) {
	int q;
	if (_be_verbose > 3) printf("Appending symbol '%s'\n", symbol_appended->name);
	if (symbol_o->address != symbol_appended->address) {
		// symboly nemaju rovnaku adresu
		if (symbol_o->address == 0xFFFF && symbol_appended->address != 0xFFFF) { symbol_o->address = symbol_appended->address; }
		else if (symbol_o->address != 0xFFFF && symbol_appended->address == 0xFFFF) { /* nic netreba robit, appendovany symbol sa nemeni */ }
		else if (symbol_o->address != 0xFFFF && symbol_appended->address != 0xFFFF) { fprintf(stderr, "error: attempt to append different symbols!\n"); exit(1); }
	} // symboly maju rovnaku adresu, mozno ich v pohode mergnut
	
	for (q = 0; q < symbol_appended->relocation_count; q++) {
		symbol_o->relocations = realloc(symbol_o->relocations, (symbol_o->relocation_count + 1) * sizeof(RELOCATION));
		relocation_copy(&(symbol_o->relocations[symbol_o->relocation_count]), &(symbol_appended->relocations[q]));
		symbol_o->relocation_count++;
	}
	
	return 1;
}

/** Skopiruje sekciu
 * Vytvori totalnu kopiu sekcie, zdrojova a cielova sekcia nebudu zdielat ani data ani symboly
 * @param section_o pointer na cielovu sekciu
 * @param section_i pointer na zdrojovu sekciu
 * @return chybovy kod, 1 znaci ziadnu chybu
 */
int section_copy(SECTION * section_o, const SECTION * section_i) {
	int q;
	
	if (section_o->data != NULL) free(section_o->data);
	if (section_o->name == NULL) section_o->name = strdup(section_i->name);
	section_o->data = malloc(section_i->size);
	memcpy(section_o->data, section_i->data, section_i->size);
	section_o->size = section_i->size;
	section_o->symbol_count = section_i->symbol_count;
	section_o->symbols = malloc(section_o->symbol_count * sizeof(SYMBOL));
	for (q = 0; q < section_o->symbol_count; q++) {
		symbol_copy(&(section_o->symbols[q]), &(section_i->symbols[q]));
	}
	return 1;
}

/** Uvolni sekciu.
 * @param section pointer na sekciu, ktora ma byt uvolnena
 * @param itself ak je nastaveny na 1, aj sa uvolni alokovana pamat, inac sa iba pointer nastavi na NULL
 * @return priznak uspesnosti, 1 znaci uspech
 */
int section_free(SECTION * section, char itself) {
	int q;
	if (section == NULL) return 1;
	for (q = 0; q < section->symbol_count; q++) {
		symbol_free(&(section->symbols[q]), 0);
	}
	free(section->name);
	free(section->symbols);
	
	if (itself) {
		free(section);
		section = NULL;
	}
	return 1;
}

// prekopirovat data
// zmenit velkost sekcie
// prekopirovat symboly + adjustnut adresy (ak nie su 0xFFFF)
// adjustnut bazove adresy relokacie symbolov (ak nie su 0x0000) a pozicie relokacii (vzdy)
/** Pripoji sekciu na inu sekciu.
 * Po pripojeni vykona zmenu bazovej adresy a spojenie u vsetkych symbolov.
 * @param section_o sekcia ku ktorej sa pripoji ina sekcia
 * @param section_appended sekcia, ktora sa pripoji a zmeni sa jej bazova adresa
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
int section_append(SECTION * section_o, const SECTION * section_appended) {
	int as_data_base = section_o->size;
	int as_symbol_base = section_o->symbol_count;
	int q, w;
	
	SYMBOL * symbol_m;
	// prekopirovat data
	section_o->data = realloc(section_o->data, section_o->size + section_appended->size);
	memcpy(&(section_o->data[section_o->size]), section_appended->data, section_appended->size);
	
	// zmenit velkost sekcie
	section_o->size += section_appended->size;
	
	// prekopirovat symboly
	/* pozor, nemozno ich prekopirovat len tak... 
	 * ak je ten isty symbol vo dvoch sekciach, tak:
	 * a) ani jeden zo symbolov nie je definovany (oba maju adresu 0xFFFF) - mozu byt mergnute
	 * b) prave jeden zo symbolov je definovany - mozu byt mergnute, pricom sa za adresu zobere ta, ktora je definovana
	 * c) oba symboly su definovane (ani jeden nema adresu 0xFFFF) - nemozno ich mergnut
	 * 
	 * v pripade c by zrejme malo nabrat efekt pole flags. ak je aspon jeden symbol globalny podla flagov, nastava nejednoznacnost
	 * relokacie. ak su oba symboly lokalne, mozno ich premenovat (trebars aj na nahodne meno - nahodny prefix alebo suffix nazvu)
	 * a povazovat ich stale za 2 rozlicne symboly
	 */
	
	printf("Appending section - merging symbols\n");
	
	for (q = 0; q < section_appended->symbol_count; q++) {
		printf("  Merging symbol '%s'\n", section_appended->symbols[q].name);
		symbol_m = symbol_find(section_o, section_appended->symbols[q].name);
		if (symbol_m == NULL) { // takyto symbol sa v cielovej sekcii nenasiel, mozno s kludom anglicana kopirovat symbol do sekcie
			printf("    -> No such symbol in target, copying (with %d relocations)\n", section_appended->symbols[q].relocation_count);
			section_o->symbols = realloc(section_o->symbols, (section_o->symbol_count + 1) * sizeof(SYMBOL));
			symbol_copy(&(section_o->symbols[section_o->symbol_count]), &(section_appended->symbols[q]));
			symbol_rebase(&(section_o->symbols[section_o->symbol_count]), as_data_base);
			section_o->symbol_count++;
			printf("Target section '%s' has now %d symbols\n", section_o->name, section_o->symbol_count);
		} else {
			printf("    -> Symbol found...");
			// symbol s takym nazvom uz v sekcii existuje
			if ((symbol_m->address == section_appended->symbols[q].address)	// adresy su rovnake. je jedno ake, symboly su rovnake (pripadne oba neresolvovane)
				|| (symbol_m->address != section_appended->symbols[q].address && (symbol_m->address == 0xFFFF || section_appended->symbols[q].address == 0xFFFF)) // adresy su rozne, ale aspon jedna z nich je neresolvovana
			) {
				printf("same as in appended, merging\n");
				// ak su adresy rovnake, viac menej nas nemusi trapit, symboly su rovnake
				// symboly sa mozu mergnut, ale druhy symbol treba pred mergovanim rebasenut, inac by 
				// sa stali zle veci
				SYMBOL new_sym;
				symbol_copy(&new_sym, &(section_appended->symbols[q]));
				symbol_rebase(&new_sym, as_data_base);
				symbol_append(symbol_m, &(section_appended->symbols[q]));
			} else {
				// symboly nemaju rovnake adresy
				fprintf(stderr, "error: duplicate symbol '%s'\n", symbol_m->name);
				exit(1);
			}
		}
	}
	
	return 1;
}

// pri relokacii sa musi OR-ovat s tym, co je na povodnej adrese, pretoze niektore bity sa niekedy prekryvaju! (a ano, je to v poriadku)
/** Vykona relokaciu symbolov v sekcii
 * Relokacia symbolov znaci, ze sa ich adresy zapisu na vsetky miesta
 * kde sa symboly pouzivaju.
 * @param section sekcia v ktorej sa vykona relokacia
 * @return chybovy kod operacie, 1 znamena ziadnu chybu
 */
int section_do_relocation(SECTION * section) {
	int q, w;
	int32_t sym_addr;
	int32_t rel_addr, rel_byte;
	
	printf("Performing relocation of %d symbols\n", section->symbol_count);
	for (q = 0; q < section->symbol_count; q++) {
		printf("Symbol '%s' has %d relocations\n", section->symbols[q].name, section->symbols[q].relocation_count);
		sym_addr = section->symbols[q].address;
		if (sym_addr == 0xFFFF) {
			fprintf(stderr, "error: unresolved symbol '%s'\n", section->symbols[q].name);
			exit(1);
		}
		for (w = 0; w < section->symbols[q].relocation_count; w++) {
			rel_addr = sym_addr - section->symbols[q].relocations[w].base;
			printf("Relocation summary:\n=================\nRelocation place:%04X\nSymbol address: %04X\nSymbol base: %04X\nRelocation target: %d\nRel shift: %d\nRel bits:%d\n", section->symbols[q].relocations[w].position, sym_addr, section->symbols[q].relocations[w].base, rel_addr, section->symbols[q].relocations[w].shift, section->symbols[q].relocations[w].bits);
			rel_byte = (abs(rel_addr) >> section->symbols[q].relocations[w].shift) & ((1 << section->symbols[q].relocations[w].bits) - 1);
			// zapiseme vypocitanu relokaciu do pamate
			section->data[section->symbols[q].relocations[w].position] |= rel_byte & 0xFF;
			
			// ak sa ma zapisat aj znamienko, tak sa zapise
			if (rel_addr < 0) {
				if (section->symbols[q].relocations[w].sign_pos == 0xFF) {
					if (_be_verbose) fprintf(stderr, "warning: relocation results in negative address but relocation doesn't write sign\n");
				} else if (section->symbols[q].relocations[w].sign_pos < 8) {
					section->data[section->symbols[q].relocations[w].position] |= (1 << section->symbols[q].relocations[w].sign_pos);
				}
			}
			int dd = *((char *) &(section->data[section->symbols[q].relocations[w].position]));
			printf("Relocation written: %02X (rel byte %d)\n", dd, rel_byte);
		}
	}
}

// pri relokacii sa musi OR-ovat s tym, co je na povodnej adrese, pretoze niektore bity sa niekedy prekryvaju! (a ano, je to v poriadku)
/** Vykona relokaciu symbolov v sekcii
 * Relokacia symbolov znaci, ze sa ich adresy zapisu na vsetky miesta
 * kde sa symboly pouzivaju.
 * @param section sekcia v ktorej sa vykona relokacia
 * @return chybovy kod operacie, 1 znamena ziadnu chybu
 */
SYMBOL * section_try_relocation(SECTION * section) {
	int q, w;
	int32_t sym_addr;
	int32_t rel_addr;
	
	for (q = 0; q < section->symbol_count; q++) {
		sym_addr = section->symbols[q].address;
		if (sym_addr == 0xFFFF) {
			return &(section->symbols[q]);
		}
	}
}

/** Vytvori popisovac objektoveho suboru
 * @param filename nazov suboru, ktory bude objektovy subor niest
 * @return popisovac objektoveho suboru
 */
OBJECT * object_create(const char * filename) {
	OBJECT * new_object = malloc(sizeof(OBJECT));
	memset(new_object, 0, sizeof(OBJECT));
	new_object->filename = strdup(filename);
	return new_object;
}

/** Serializuje relokaciu.
 * Pouziva sa interne pri zapise do suboru
 * @param relocation pointer na zapisovanu relokaciu
 * @param w_relocation adresa na ktoru sa zapise, kam bola serializovana relokacia ulozena
 * @param s_relocation adresa na ktoru sa zapise dlzka relokacie
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
static inline int relocation_write(const RELOCATION * relocation, RELOCATION ** w_relocation, unsigned * s_relocation) {
	*w_relocation = malloc(sizeof(RELOCATION));
	memcpy(*w_relocation, relocation, sizeof(RELOCATION));
	*s_relocation = sizeof(RELOCATION);
	return 1;
}

/** Serializuje symbol.
 * Pouziva sa interne pri zapise do suboru
 * @param relocation pointer na zapisovany symbol
 * @param w_relocation adresa na ktoru sa zapise, kam bol serializovany symbol ulozeny
 * @param s_relocation adresa na ktoru sa zapise dlzka symbolu
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
static int symbol_write(const SYMBOL * symbol, _SYMBOL ** w_symbol, unsigned * s_symbol) {
	int q;
	
	int s_sym = sizeof(_SYMBOL);
	_SYMBOL * w_sym = malloc(s_sym);
	
	unsigned s_rel = 0;
	RELOCATION * w_rel = NULL;
	
	memset(w_sym, 0, s_sym);
	
	strncpy((char *) &(w_sym->name), symbol->name, 31);
	w_sym->address = symbol->address;
	w_sym->flags = symbol->flags;
	printf("Writing symbol with %d relocations\n", symbol->relocation_count);
	w_sym->relocation_count = symbol->relocation_count;
	for (q = 0; q < symbol->relocation_count; q++) {
		if (relocation_write(&(symbol->relocations[q]), &w_rel, &s_rel)) {
			w_sym = realloc(w_sym, s_sym + s_rel);
			memcpy((char *) w_sym + s_sym, w_rel, s_rel);
			s_sym += s_rel;
			free(w_rel);
		} else {
			fprintf(stderr, "unable to write symbol\n");
			exit(1);
		}
	}
	
	*w_symbol = w_sym;
	*s_symbol = s_sym;
	return 1;
}

/** Serializuje sekciu.
 * Pouziva sa interne pri zapise do suboru
 * @param relocation pointer na zapisovanu sekciu
 * @param w_relocation adresa na ktoru sa zapise, kam bola serializovana sekcia ulozena
 * @param s_relocation adresa na ktoru sa zapise dlzka sekcie
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
static int section_write(const SECTION * section, _SECTION ** w_section, unsigned * s_section) {
	int q;
	
	int s_sect = sizeof(_SECTION);
	_SECTION * w_sect = malloc(s_sect);
	
	unsigned s_sym = 0;
	_SYMBOL * w_sym = NULL;
	
	memset(w_sect, 0, s_sect);
	
	strncpy((char *) &(w_sect->name), section->name, 31);
	w_sect->size = section->size;
	w_sect->symbol_count = section->symbol_count;
	
	if (_be_verbose > 2) printf("section '%s' size is %d\n", section->name, section->size);
	
	w_sect = realloc(w_sect, s_sect + section->size);
	memcpy(((char *) w_sect) + s_sect, section->data, section->size);
	s_sect += section->size;
	
	for (q = 0; q < section->symbol_count; q++) {
		if (symbol_write(&(section->symbols[q]), &w_sym, &s_sym)) {
			w_sect = realloc(w_sect, s_sect + s_sym);
			memcpy((char *) w_sect + s_sect, w_sym, s_sym);
			s_sect += s_sym;
			free(w_sym);
		} else {
			fprintf(stderr, "unable to write symbol\n");
			exit(1);
		}
	}
	
	*w_section = w_sect;
	*s_section = s_sect;
	return 1;
}

/** Serializuje objekt (so vsetkymi sekciami).
 * Pouziva sa interne pri zapise do suboru
 * @param relocation pointer na zapisovany objekt
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
int object_write(const OBJECT * object) {
	int fd, q, remain, written;
	
	unsigned s_obj = sizeof(_OBJECT);
	_OBJECT * w_obj = malloc(s_obj);
	unsigned s_sect = 0;
	_SECTION * w_sect = NULL;
	
	w_obj->signature = 0xAA55;
	w_obj->section_count = object->section_count;
	
	for (q = 0; q < object->section_count; q++) {
		if (section_write(&(object->sections[q]), &w_sect, &s_sect)) {
			w_obj = realloc(w_obj, s_obj + s_sect);
			memcpy((char *) w_obj + s_obj, w_sect, s_sect);
			s_obj += s_sect;
			free(w_sect);
		} else {
			fprintf(stderr, "unable to write section\n");
			return 0;
		}
	}
	
	fd = open(object->filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	
	if (fd == -1) {
		perror("object file open");
		exit(2);
	}
	
	remain = s_obj;
	
	//	printf("Object size %d\n", remain);
	
	while (remain > 0) {
		//		printf("writing %d of %d bytes...\n", (remain > 4096 ? 4096 : remain), remain);
		written = write(fd, &(((char *) w_obj)[s_obj - remain]), (remain > 4096 ? 4096 : remain));
		//		sleep(1);
		if (written < 0) {
			perror("object write");
			return 0;
		}
		remain -= written;
	}
	
	close(fd);
	
	return 1;
}

/** Nacita relokaciu.
 * @param fd popisovac vstupneho suboru
 * @param symbol pointer na pamat alokovanu pre novu relokaciu.
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
static int relocation_load(int fd, RELOCATION * relocation) {
	int ra = read(fd, relocation, sizeof(RELOCATION));
	
	if (_be_verbose) printf("      Relocation\n        Address %04X\n        Base %04X\n", relocation->position, relocation->base);
	if (ra == sizeof(RELOCATION)) return 1; else return 0;
}

/** Nacita symbol.
 * @param fd popisovac vstupneho suboru
 * @param symbol pointer na pamat alokovanu pre novy symbol
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
static int symbol_load(int fd, SYMBOL * symbol) {
	_SYMBOL w_sym;
	int q, ra, m;
		
	ra = read(fd, &w_sym, sizeof(_SYMBOL));
	
	symbol->name = strdup(w_sym.name);
	symbol->address = w_sym.address;
	symbol->flags = w_sym.flags;
	symbol->relocation_count = w_sym.relocation_count;
	symbol->relocations = malloc(sizeof(RELOCATION) * symbol->relocation_count);
	
	if (_be_verbose) printf("    Symbol '%s'\n      Address %04X\n      Loading %d relocations\n", symbol->name, symbol->address, symbol->relocation_count);
	
	for (q = 0; q < symbol->relocation_count; q++) {
		relocation_load(fd, &(symbol->relocations[q]));
	}
	
	if (_be_verbose) printf("    Symbol end\n");
	
	return 1;
}

/** Nacita sekciu.
 * @param fd popisovac vstupneho suboru
 * @param symbol pointer na pamat alokovanu pre novu sekciu
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
static int section_load(int fd, SECTION * section) {
	_SECTION w_sect;
	int q, ra, rm;
	
	ra = read(fd, &w_sect, sizeof(_SECTION));
	
	section->name = strdup(w_sect.name);
	section->size = w_sect.size;
	section->symbol_count = w_sect.symbol_count;
	section->data = malloc(section->size);
	
	if (_be_verbose) printf("  Section '%s'\n    Loading %d symbols\n", section->name, section->symbol_count);
	
	rm = section->size;
	
	while (rm > 0) {
		ra = read(fd, &(section->data[section->size - rm]), (rm > 4096 ? 4096 : rm));
		
		rm -= ra;
	}
	
	section->symbols = malloc(sizeof(SYMBOL) * section->symbol_count);
	
	for (q = 0; q < section->symbol_count; q++) {
		symbol_load(fd, &(section->symbols[q]));
	}
	
	if (_be_verbose) printf("  Section end\n");
	
	return 1;
}

/** Uvolni objekt.
 * @param object pointer na popisovac objektu.
 * @return chybovy kod, 1 znamena ziadnu chybu
 */
int object_free(OBJECT * object) {
	int q;
	if (object == NULL) return 1;
	for (q = 0; q < object->section_count; q++) {
		section_free(&(object->sections[q]), 0);
	}
	free(object->filename);
	free(object->sections);
	free(object);
	object = NULL;
	return 1;
}



/** Nacita objekt zo suboru.
 * @param filename nazov suboru z ktoreho sa ma nacitat objekt
 */
OBJECT * object_load(const char * filename) {
	int fd, ra, q;
	
	fd = open(filename, O_RDONLY);
	
	if (fd == -1) {
		perror("object file open");
		return NULL;
	}
	
	_OBJECT w_obj;
	OBJECT * object = malloc(sizeof(OBJECT));
	
	ra = read(fd, &w_obj, sizeof(_OBJECT));
	
	if (ra != sizeof(_OBJECT)) return NULL;
	
	object->filename = strdup(filename);
	object->section_count = w_obj.section_count;
	object->sections = malloc(sizeof(SECTION) * object->section_count);
	
	SECTION * sect;
	
	if (_be_verbose) printf("Object\n  Loading %d sections...\n", object->section_count);
	
	for (q = 0; q < object->section_count; q++) {
		section_load(fd, &(object->sections[q]));
	}
	
	if (_be_verbose) printf("Object end\n");
	
	return object;
}

/** Vrati sekciu v objekte na zaklade jej nazvu.
 * @param object objekt v ktorom ma byt sekcia vyhladana
 * @param secion_name nazov, podla ktoreho ma byt sekcia najdena
 * @return pointer na sekciu, alebo NULL pointer, ak sa sekcia nenasla
 */
SECTION * object_get_section_by_name(OBJECT * object, const char * section_name) {
	int q;
	for (q = 0; q < object->section_count; q++) {
		if (strcmp(section_name, object->sections[q].name) == 0) return &(object->sections[q]);
	}
	
	return NULL;
}