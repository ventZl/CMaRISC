#ifndef __SUNBLIND_OBJECT_H__
#define __SUNBLIND_OBJECT_H__

#include <stdint.h>

typedef unsigned short ADDRESS;

enum reloc_type { RELOC_MASKED, RELOC_HI, RELOC_LO };

struct relocation {
	ADDRESS base;				// relokacia sa vykona relativne k tejto adrese (ktora je zasa relativna k bazovej adrese segmentu v pamati)
	ADDRESS position;			// relokacia sa nachadza na tejto adrese a zabera presne 1 Byte
	uint8_t shift;				// o kolko bitov sa posunie vypocitana adresa doprava
	uint8_t bits;				// kolko bitov sa odmaskuje a vyORuje s povodnym bytom na adrese relokacie
	uint8_t sign_pos;			// na akom bite ma byt znamienko (0 ak kladne, 1 ak zaporne). ak je hodnota 0xFF, znamienko sa nezapise
} __attribute__((packed));

typedef struct relocation RELOCATION;

struct symbol {
	char * name;
	ADDRESS address;
	uint16_t relocation_count;
	RELOCATION * relocations;
	uint8_t flags;
};

struct wire_symbol {
	char name[32];
	ADDRESS address;
	uint16_t relocation_count;
	uint8_t flags;
	RELOCATION relocations[0];
} __attribute__((packed));

typedef struct symbol SYMBOL;
typedef struct wire_symbol _SYMBOL;

struct section {
	char * name;
	uint16_t size;
	uint16_t symbol_count;
	uint8_t * data;
	SYMBOL * symbols;
};

struct wire_section {
	char name[32];
	uint16_t size;
	uint16_t symbol_count;
	uint8_t data[0];
	SYMBOL symbols[0];
} __attribute__((packed));

typedef struct section SECTION;
typedef struct wire_section _SECTION;

struct object {
	char * filename;
	uint16_t section_count;
	SECTION * sections;
};

struct wire_object {
	uint16_t signature;
	uint16_t section_count;
	SECTION sections[0];
} __attribute__((packed));

typedef struct object OBJECT;
typedef struct wire_object _OBJECT;

void objects_be_verbose(int level);
SECTION * section_create(const char * name);
int section_append_data(SECTION * section, const unsigned char * data, unsigned length);
ADDRESS section_get_next_address(const SECTION * section);
int section_dump(SECTION * section, const char * filename);
int section_free(SECTION * section, char itself);
int binary_write(SECTION * section, ADDRESS entrypoint, const char * filename);
int binary_read(const char * filename, unsigned char * memory, ADDRESS * entrypoint, unsigned memsize);
int section_data_copy(SECTION * section, unsigned char * dest, unsigned size);

SYMBOL * symbol_set(SECTION * section, unsigned char * name, ADDRESS address, uint8_t flags);
int symbol_add_relocation(SECTION * section, unsigned char * name, ADDRESS position, ADDRESS base, uint8_t shift, uint8_t bits, uint8_t sign_pos);
ADDRESS symbol_get_address(SECTION * section, unsigned char * symbol);

int object_add_section(OBJECT * object, const SECTION * section);
int section_copy(SECTION * section_o, const SECTION * section_i);
int section_append(SECTION * section_o, const SECTION * section_appended);
int section_do_relocation(SECTION * section);
SYMBOL * section_try_relocation(SECTION * section);

OBJECT * object_create(const char * filename);
int object_write(const OBJECT * object);
OBJECT * object_load(const char * filename);
SECTION * object_get_section_by_name(OBJECT * object, const char * section_name);
int object_free(OBJECT * object);

#endif
