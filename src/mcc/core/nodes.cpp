#include <iostream>
#include <string>
#include <sstream>
#include <typeinfo>

#include "nodes.h"
#include "compile.h"
#include "exception.h"
#include <stdio.h>

/** Funkcia vrati string obsahujuci n tabulatorov
 * @param n pocet tabulatorov v stringu
 * @return string s tabulatormi
 */
std::string mktab(int n) { 
	std::string r; 
	for (int q = 0; q < n; q++) r += "  "; 
	return r; 
}

/** Funkcia kompilujuca program ako celok
 * Pouziva sa ako vstupny bod kompilacie programu ulozeneho v AST
 * @param ctxt kompilacny kontext v ktorom sa program skompiluje
 * @return assembler skompilovaneho modulu
 */
CC::Assembly * AST::Program::Compile(CC::Context * ctxt) {
	CC::Assembly * ass = ctxt->createAssembly(0xFF);
	for(StatementList::iterator it = this->statements->begin(); it != this->statements->end(); ++it) {
		ass->append((*it)->Compile(ctxt));
	}
	return ass;
}

/** Skompiluje deklaraciu noveho typu
 * TODO
 */
CC::Assembly* AST::TypeDef::Compile(CC::Context* ctxt) {
	CC::Assembly * ass = ctxt->createAssembly(0xFF);
	if (!ctxt->existsType(this->type, CURRENT_CONTEXT | COMPLETE_TYPE)) ctxt->declareType(this->type);
	else throw SemanticException(std::string("Type already exists: ") + this->type->humanName());
}

/** Vrati vysledok volania funkcie
 * Metoda vygeneruje assemblerovy kod naplnajuci argumenty volania funkcie a samotne zavolanie funkcie
 * @param ctxt kontext v ktorom je funkcia zavolana
 * @return assembler volania funkcie
 */
CC::Assembly * AST::FunctionCall::byValue(CC::Context* ctxt) {
//	printf("Compiling function call...\n");
	CC::Assembly * code = ctxt->createAssembly(this->returnRegister());
	CC::Variable * fn = ctxt->findVariable(this->function_name);
//	printf("fn = %p\n", fn);
	if (fn == NULL) throw SemanticException(std::string("undefined reference to `") + this->function_name + std::string("'"));
	const CC::FunctionType * fntype = dynamic_cast<const CC::FunctionType *>(fn->getType());
	if (fntype == NULL) throw SemanticException("Attempt to call something which is not function");
	CC::VariableSet * fnparms = fntype->getArguments();
	AST::ExpressionList * fnargs = this->arguments;
	if (fnparms->size() != fnargs->size()) throw SemanticException(std::string("Invalid argument count for function `") + this->function_name + std::string("'"));
	CC::VariableSet::iterator fnp_it = fnparms->begin();
	AST::ExpressionList::iterator fna_it = fnargs->begin();
	unsigned short regmask = ctxt->getRegisters()->getRegisterUsageMask();
	for (int q = 0; q < fnargs->size(); q++) {
		if (!(*(fnp_it->second->getType()) == *((*fna_it)->getType(ctxt)))) throw SemanticException(std::string("Type mismatch for argument ") + convertInt(q + 1) + std::string("of function ") + this->function_name + "(" + fnp_it->second->getType()->humanName() + " vs. " + (*fna_it)->getType(ctxt)->humanName() + ")");
		if (regmask & (1 << q) != 0) {
			code->appendCode(ctxt->getRegisters()->hook(q));
		}
		(*fna_it)->setReturnRegister(q);
		code->append((*fna_it)->byValue(ctxt));
	}
	code->appendCode(opcodeL("BRANCHL", this->function_name));
	for (int q = fnargs->size() - 1; q >= 0; q--) {
		if (regmask & (1 << q) != 0) {
			code->appendCode(ctxt->getRegisters()->free(q));
		}
	}
	this->setReturnRegister(0);
//	printf("Function dump:\n%s\n", code->getInstructions().c_str());
	return code;
}

/** Vrati typ navratovej hodnoty funkcie
 * @param ctxt kontext
 * @return navratovy typ
 */
const CC::Type * AST::FunctionCall::buildType(CC::Context* ctxt) {
	CC::Variable * func = ctxt->findVariable(this->function_name);
	CC::Assembly * code = ctxt->createAssembly(this->returnRegister());
	if (func != NULL) return func->getType(); else throw SemanticException(std::string("undefined reference to `") + this->function_name + "'");
}

/** Skompiluje zoznam statementov programu
 * @param ctxt kontext v ktorom budu statementy kompilovane
 * @return assembler skompilovanych statementov
 */
CC::Assembly * AST::StatementList::Compile(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(0xFF);
	CC::Assembly * sc;
	for (StatementList::iterator it = this->begin(); it != this->end(); ++it) {
//		printf("Compiling statement...\n");
		sc = (*it)->Compile(ctxt);
//		printf("Appending statement code...\n");
		code->append(sc);
	}
	return code;
}

/** Skompiluje kod navratu z funkcie.
 * Navrat z funkcie vyhodnoti navratovu hodnotu, ak nejaka existuje a ulozi ju do registra 0
 * @param ctxt kontext v ktorom sa navrat kompiluje
 * @return prelozeny assembler
 */
CC::Assembly * AST::Return::Compile(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(0xFF);
	if (this->child != NULL) {
		this->child->setReturnRegister(0);
		code->append(this->child->byValue(ctxt));
	}
	code->appendCode(opcodeL("BRANCH", ".return"));
//	code->appendCode("\tMOV SP, BP\n\tPOP BP\n\tRET\n");
	return code;
}

/** Skompiluje vyraz, ktoreho vysledok sa nevyuzije.
 * Fakticky vysledok skompilovania tohto uzla je, ze sa uvolni
 * register v ktorom je ulozeny vysledok kompilacie vyrazu.
 * @todo Do buducnosti by bolo vhodne zaviest optimalizaciu
 * ktora uplne odstrani uzly, ktore obsahuju vyrazy s nevyuzitou
 * navratovou hodnotou.
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly * AST::UnusedValue::Compile(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(0xFF);
	CC::Assembly * expression_code = this->child->byValue(ctxt);
	// toto je picovina
	//if (expression_code->getReturnRegister() == 0xFF) ctxt->destroyAssembly(expression_code);
	//else
	code->append(expression_code);
	if (expression_code->getReturnRegister() < 16) ctxt->getRegisters()->free(expression_code->getReturnRegister());
	return code;
}

/** Skompiluje definiciu funkcie.
 * Zisti, ci sa nejedna o duplicitnu definiciu funkcie a ak nie, vytvori novy funkcny typ, ktory
 * vlozi do typoveho kontextu prekladaca na aktualnej urovni. Formalne argumenty su vlozene ako registrove
 * premenne do kontextu premennych a tento je prekryty kontextom lokalnych premennych vo funkcii.
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly * AST::Function::Compile(CC::Context* ctxt) {
	CC::VariableSet * args = new CC::VariableSet();
//	printf("Compiling function %s\n", this->name.c_str());
	int q = 0;
	if (this->arguments != NULL) {
		for (ArgumentList::iterator it = this->arguments->begin(); it != this->arguments->end(); ++it, q++) {
			args->append(new CC::Variable((*it)->getName(), (*it)->getType(), new CC::RegisterAllocator(q)));
		}
	}
	CC::Type * fn_type = new CC::FunctionType(this->name, this->return_type, args);
	CC::Type * known_type = ctxt->findType(fn_type, 0);
	
	/* treba zistit, ci nahodou takato funkcia uz nie je definovana */
	if (known_type != NULL) {
		if (known_type->isComplete()) {
			if ((*known_type) == (*fn_type)) ;// OK, pouzivame predtym prototypovanu funkciu
			else throw SemanticException(std::string("Function `") + this->name + "' already exists");
		}
	} else {
		ctxt->declareType(fn_type);
	}
	CC::Variable * var = ctxt->declareVariable(this->name, fn_type, new CC::StaticVariableAllocator(this->name));

	ctxt->pushVariableSet(new CC::VariableSet());			// vytvorenie kontextu pre formalne argumenty - aby ich bolo mozne predefinovat v tele funkcie
	
	/* vlozenie premennych do kontextu formalnych argumentov funkcie */
	q = 0;
	if (this->arguments != NULL) {
		for (ArgumentList::iterator it = this->arguments->begin(); it != this->arguments->end(); ++it, q++) {
			ctxt->declareVariable((*it)->getName(), (CC::Type *) (*it)->getType(), new CC::RegisterAllocator(q));
		}
	}
	ctxt->pushVariableSet(new CC::VariableSet());
	CC::Assembly * code = ctxt->createAssembly(0xFF);
	code->appendCode("\n");
	/* pre main sa vygeneruje o nieco iny / skrateny prolog a epilog funkcie */
	if (this->name != "main")	code->appendCode(label(this->name) + opcode("PUSH", REG_BP) + opcode("MOV", REG_BP, REG_SP));
	else code->appendCode(label(this->name) + opcode("MOV", REG_BP, REG_SP));
	// a teraz sa divajte: vsakze to nedava zmysel, ze?
	REGISTER displacement = ctxt->getRegisters()->alloc(REGISTER_IMMEDIATE);
	std::string code_free_displacement = ctxt->getRegisters()->free(displacement);
	/* register si alokujem a hned potom ho uvolnim. dovod je ten, ze
	 * Ak mam vo funkcii viac ako 16 bytov lokalnych premennych, musim si 
	 * alokovat register do ktoreho ulozim velkost premennych aby som ju vedel 
	 * odcitat od SP. Lenze velkost lokalnych premennych sa dozviem az
	 * po skompilovani funkcie, lenze vyhradit miesto je nutne pred kompilaciou
	 * a pocas kompilacie ten register uz nebude zabrany. Preto si nejaky register
	 * alokujem a hned uvolnim. Vysledkom je aj tak len zmeneny SP, cize tento
	 * register po operacii aj tak nebude zabrany, pokial premenne nie su prilis velke.
	 */
	CC::Assembly * function_body = this->commands->Compile(ctxt);
	CC::VariableSet * local_vars = ctxt->popVariableSet();		// vyberiem vnutorny kontext premennych obsahujuci lokalne premenne (treba ich alokovat na zasobniku)
	unsigned locals_size = local_vars->getSize();
	if (locals_size > 31) {
		code->appendCode(opcodeC("ILOAD", displacement, (locals_size >> 8) & 0xFF));
		code->appendCode(opcodeC("ILOAD", displacement, locals_size & 0xFF));
		code->appendCode(opcode("SUB", REG_SP, displacement));
	} else if (locals_size > 15) {
		code->appendCode(opcodeC("SUBC", REG_SP, 15));
		code->appendCode(opcodeC("SUBC", REG_SP, locals_size - 15));
	} else if (locals_size > 0) {
		code->appendCode(opcodeC("SUBC", REG_SP, locals_size));
	}
	
	code->append(function_body);
	code->appendCode(label(".epilogue"));
	
	if (this->name != "main") {
		code->appendCode(opcode("MOV", REG_SP, REG_BP));
		code->appendCode(opcode("POP", REG_BP));
	}
	code->appendCode(opcodeN("RET"));
	ctxt->popVariableSet();		// vyberiem vonkajsi kontext premennych obsahujuci formalne parametre funkcie (aby som sa dostal do globalneho kontextu premennych)
	return code;
}

/** Skompiluje binarnu aritmeticku alebo logicku operaciu
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly * AST::BinaryOperation::byValue(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(this->return_reg);
	if (this->getType(ctxt) != NULL) {
		this->left->setReturnRegister(this->returnRegister());			/* Binarne operacie vracaju vysledok v rovnakom registri, ako je prvy operand */
		REGISTER lret = code->append(this->left->byValue(ctxt));
		REGISTER rret = code->append(this->right->byValue(ctxt));
		code->appendCode(this->genOpcode(lret, rret, this->compile_flags));
		this->setReturnRegister(lret);
		return code;
	} else throw SemanticException("Type mismatch of operands.");
}

/** Vrati typ vysledku binarnej operacie.
 * Ak sa typ operandu na lavej a pravej strane zhoduje, vrati tento typ.
 * V opacnom pripade vrati chybu prekladu.
 * @param ctxt prekladovy kontext
 * @return prelozeny assembler
 */
const CC::Type * AST::BinaryOperation::buildType(CC::Context * ctxt) {
	if (this->left->getType(ctxt) == this->right->getType(ctxt)) return this->left->getType(ctxt); else throw SemanticException("Incompatible argument types for binary operation");
}

/** Vygeneruje assembler operacie scitania.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Add::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	return opcode("ADD", arg1, arg2);
//	return std::string("ADD R") + convertInt(arg1) + std::string(", R") + convertInt(arg2) + std::string("\n");
}

/** Vygeneruje assembler operacie odcitania.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Sub::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	return opcode("SUB", arg1, arg2);
//	return std::string("SUB R") + convertInt(arg1) + std::string(", R") + convertInt(arg2) + std::string("\n");
}

/** Vygeneruje assembler operacie nasobenia.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Mul::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	return opcode("MUL", arg1, arg2);
//	return std::string("MUL R") + convertInt(arg1) + std::string(", R") + convertInt(arg2) + std::string("\n");
}

/** Vygeneruje assembler operacie delenia.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Div::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	return opcode("DIV", arg1, arg2);
//	return std::string("DIV R") + convertInt(arg1) + std::string(", R") + convertInt(arg2) + std::string("\n");
}

/** Vygeneruje assembler operacie modulo N.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Modulo::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	return opcode("MOD", arg1, arg2);
//	return std::string("MOD R") + convertInt(arg1) + std::string(", R") + convertInt(arg2) + std::string("\n");
}

/** Vygeneruje assembler operacie logickeho sucinu.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::And::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	return opcode("AND", arg1, arg2);
//	return std::string("AND R") + convertInt(arg1) + std::string(", R") + convertInt(arg2) + std::string("\n");
}

/** Vygeneruje assembler operacie logickeho suctu.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Or::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	return opcode("OR", arg1, arg2);
//	return std::string("OR R") + convertInt(arg1) + std::string(", R") + convertInt(arg2) + std::string("\n");
}

/** Vygeneruje assembler operacie logickej nonekvivalencie.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Xor::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	return opcode("XOR", arg1, arg2);
//	return std::string("XOR R") + convertInt(arg1) + std::string(", R") + convertInt(arg2) + std::string("\n");
}

/** Vygeneruje assembler operacie ekvivalencie.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Equ::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	return opcode("XOR", arg1, arg2) + opcode("NOT", arg1);
}

/** Vygeneruje assembler operacie nonekvivalencie.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Nequ::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	return opcode("XOR", arg1, arg2);
}

/** Vygeneruje assembler operacie vacsi.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Gt::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	/* arg2 - arg1; if (!sign_flag) ret 0, else ret arg2 - arg1 */
	std::string code = std::string("SUBS R") + convertInt(arg2) + std::string(", R") + convertInt(arg1) + std::string("\n");
	code += std::string("XOR CS R") + convertInt(arg1) + std::string(", R") + convertInt(arg1) + std::string("\n");
	return code;
	/* TODO check & fix */
}

/** Vygeneruje assembler operacie vacsi alebo rovny.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Gte::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	/* !Lt */
	std::string code = std::string("SUBS R") + convertInt(arg1) + std::string(", R") + convertInt(arg2) + std::string("\n");
	code += std::string("XOR CS R") + convertInt(arg1) + std::string(", R") + convertInt(arg1) + std::string("\n");
	code += std::string("XOR R") + convertInt(arg1) + std::string(", R") + convertInt(arg1) + std::string("\n");
	return code;
	/* TODO check & fix */
}

/** Vygeneruje assembler operacie mensi.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Lt::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	/* arg2 - arg1; if (!sign_flag) ret 0, else ret arg2 - arg1 */
	std::string code = std::string("SUBS R") + convertInt(arg1) + std::string(", R") + convertInt(arg2) + std::string("\n");
	code += std::string("XOR CS R") + convertInt(arg1) + std::string("\n");
	return code;
	/* TODO check & fix */
}

/** Vygeneruje assembler operacie mensi alebo rovny.
 * @param arg1 operand na lavej strane
 * @param arg2 operand na pravej strane
 * @param flags dodatocne volby vykonania operacie
 * @return opkod operacie
 */
std::string AST::Lte::genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) {
	/* !Gt */
	std::string code = std::string("SUBS R") + convertInt(arg2) + std::string(", R") + convertInt(arg1) + std::string("\n");
	code += std::string("XOR CS R") + convertInt(arg1) + std::string("\n");
	return code;
	/* TODO check & fix */
}

/** Skompiluje vstup konstanty do vypoctu.
 * Vlozi do registra konstantu
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly * AST::Integer::byValue(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(this->returnRegister());
	REGISTER const_register = this->returnRegister();
	if (this->returnRegister() > 7) const_register = ctxt->getRegisters()->alloc(REGISTER_IMMEDIATE);	// ak cielovy register nie je urceny, alebo je taky, ze sa donho neda vlozit immediate
	code->appendCode(opcodeC("ILOAD", const_register, (this->value >> 8) & 0xFF)); //std::string("ILOAD R") + convertInt(const_register) + ", " + convertInt(this->value >> 8) + "\n");
	code->appendCode(opcodeC("ILOAD", const_register, this->value & 0xFF)); //std::string("ILOAD R") + convertInt(const_register) + ", " + convertInt(this->value & 0xFF) + "\n");
	if (const_register != this->returnRegister()) {
		if (this->returnRegister() != 0xFF) {
			code->appendCode(opcode("MOV", this->returnRegister(), const_register));
			ctxt->getRegisters()->free(const_register);
		} else {
			code->setReturnRegister(const_register);
		}
	}
	return code;
}

/** Skompiluje znegovanie logickej hodnoty
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly * AST::Neg::byValue(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(this->returnRegister());
	REGISTER value_register = code->append(this->argument->byValue(ctxt));
	code->appendCode(opcode("SUBC", value_register, 0)); //std::string("SUBCS R") + convertInt(value_register) + std::string("0\n"));
	/* TODO check & fix */
}

/** Skompiluje pouzitie hodnoty premennej.
 * Premenna sa vyhlada v aktualnom kontexte premennych. Ak sa nenajde ziadna premenna
 * s takym nazvom, je hodena chyba. Podporovane su vsetky druhy in-memory a in-register premennych.
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly * AST::Variable::byValue(CC::Context * ctxt) {
	CC::Variable * var = ctxt->findVariable(this->name);
	CC::Assembly * code = ctxt->createAssembly(this->returnRegister());
	if (var == NULL) throw SemanticException(std::string("undefined reference to `") + this->name + "'");
	REGISTER var_addr_register = code->append(var->getAddress(ctxt));
	REGISTER var_value_register = this->returnRegister();
	if (var_value_register = 0xFF) ctxt->getRegisters()->alloc(REGISTER_GENERIC);
	code->appendCode(var->getStorageClass()->useAddress(var_addr_register, var_value_register, OP_READ));
	ctxt->getRegisters()->free(var_addr_register);
	return code;
}

/** Skompiluje pouzitie adresy premennej.
 * Vrati adresu premennej, ak je mozne ju zistit.
 * Tieto operacie je mozne pouzit na premenne, ktore su ulozene in-memory.
 * In-register premenne neumoznuju pracu s adresami.
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly * AST::Variable::byReference(CC::Context* ctxt) {
	CC::Variable * var = ctxt->findVariable(this->name);
	CC::Assembly * code = ctxt->createAssembly(this->returnRegister());
	if (var == NULL) throw SemanticException(std::string("undefined reference to `") + this->name + "'");
	REGISTER var_addr_register = code->append(var->getAddress(ctxt));
	if (this->returnRegister() != 0xFF) {
		code->appendCode(opcode("MOV", this->returnRegister(), var_addr_register));
		ctxt->getRegisters()->free(var_addr_register);
	} else {
		code->setReturnRegister(var_addr_register);
	}
	return code;
}

/** Vrati typ premennej
 * @param ctxt kompilacny kontext
 * @return typ premennej
 */
const CC::Type * AST::Variable::buildType(CC::Context* ctxt) {
	CC::Variable * var = ctxt->findVariable(this->name);
	if (var == NULL) throw SemanticException(std::string("Variable `") + this->name + "' not found!");
	return var->getType();
}

/** Skompiluje priradenie hodnoty vyrazu na adresu, kam ukazuje iny vyraz.
 * Cielom moze byt in-memory vyraz aj in-register adresa. Typy zdroja a ciela
 * sa musia zhodovat.
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly * AST::Assign::byValue(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(0xFF);
	REGISTER value_return;
	REGISTER variable_address;
	if (*(this->target->getType(ctxt)) == *(this->value->getType(ctxt))) {
		value_return = code->append(this->value->byValue(ctxt));
		variable_address = code->append(this->target->byReference(ctxt));
		code->appendCode(this->target->getStorageClass(ctxt)->useAddress(variable_address, value_return, OP_WRITE));
//		code->appendCode(std::string("STORE [") + convertInt(variable_address) + std::string("], ") + convertInt(value_return) + std::string("\n"));
//		ctxt->getRegisters()->free(value_return);
		code->setReturnRegister(value_return);
		return code;
	} else throw SemanticException(std::string("Type mismatch assigning expression to variable!\n"));
}

/** Skompiluje prikaz cyklu.
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly * AST::While::Compile(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(0xFF);
	std::string condition_label = ctxt->genUniqueLabel();
	std::string bypass_label = ctxt->genUniqueLabel();
	code->appendCode(condition_label + std::string(":\n"));
	REGISTER cond_return = code->append(this->condition->byValue(ctxt));
	code->appendCode(opcode("SUBCS", cond_return, 0) + opcodeL("BRANCH CZ", bypass_label));
//	code->appendCode(std::string("\tSUBCS R") + convertInt(cond_return) + std::string(", 0\n\tBRANCH CZ ") + bypass_label);
	ctxt->getRegisters()->free(cond_return);
	code->append(this->command->Compile(ctxt));
	code->appendCode(opcodeL("BRANCH", condition_label) + label(bypass_label));
	//code->appendCode(std::string("\tBRANCH ") + condition_label + std::string("\n") + bypass_label + std::string(":\n"));
	return code;
}

/** Skompiluje prikaz podmieneneho vykonania.
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly * AST::If::Compile(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(0xFF);
	std::string condition_label = ctxt->genUniqueLabel();
	std::string else_label = ctxt->genUniqueLabel();
	std::string bypass_label;
	if (this->false_command == NULL) bypass_label = else_label;
	else bypass_label = ctxt->genUniqueLabel();
	
	code->appendCode(condition_label + std::string(":\n"));
	REGISTER cond_return = code->append(this->condition->byValue(ctxt));
	code->appendCode(opcode("SUBCS", cond_return, 0) + opcodeL("BRANCH CZ", else_label));
//	code->appendCode(std::string("\tSUBCS R") + convertInt(cond_return) + std::string(", 0\n\tBRANCH CZ ") + else_label);
	ctxt->getRegisters()->free(cond_return);
	code->append(this->true_command->Compile(ctxt));
	if (this->false_command != NULL) {
		code->appendCode(opcodeL("BRANCH", bypass_label));
//		code->appendCode(std::string("\tBRANCH ") + bypass_label + std::string("\n"));
		code->appendCode(label(else_label));
//		code->appendCode(else_label + std::string(":\n"));
		code->append(this->false_command->Compile(ctxt));
	}
	code->appendCode(label(bypass_label));
//	code->appendCode(bypass_label + std::string(":\n"));
	return code;
}

/** Skompiluje definiciu premennej.
 * Ak premenna este v aktualnom kontexte neexistuje (v nadradenych moze, bude prekryta),
 * bude vytvorena nova premenna s typom a nazvom podla zadania.
 * Ak premenna ma inicializacny vyraz, bude vyhodnoteny a jeho hodnota bude
 * ulozena do premennej.
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly * AST::VariableDefinition::Compile(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(0xFF);
	CC::Variable * var = ctxt->declareVariable(this->name, this->type, new CC::LocalVariableAllocator(ctxt, this->type->sizeOf()));
//	printf("var = %p\n", var);
	if (this->initializer != NULL) {
		REGISTER variable_address = code->append(var->getAddress(ctxt));
		REGISTER initializer_value = code->append(this->initializer->byValue(ctxt));
		code->appendCode(opcode("STORE", variable_address, INDIRECT, initializer_value));
//		code->append(std::string("STORE [R") + convertInt(variable_address) + std::string("], R") + convertInt(initializer_value));
		ctxt->getRegisters()->free(initializer_value);
		ctxt->getRegisters()->free(variable_address);
	}
	return code;
}

/** Skompiluje referencovanie Lvalue vyrazu.
 * Vysledkom prekladu je vlozenie adresy vysledku vyrazu, ktory reprezentuje
 * Lvalue hodnotu. Ak argument nie je Lvalue, vznikne chyba.
 * @param ctxt kompilacny kontext
 * @return prelozeny assembler
 */
CC::Assembly* AST::Reference::byValue(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(this->returnRegister());
	REGISTER expr_addr_reg = code->append(this->expression->byReference(ctxt));
	code->setReturnRegister(expr_addr_reg);
	return code;
}

/** Vrati typ referencie.
 * @param ctxt kompilacny kontext
 * @return typ argumentu zapuzdreny v type Pointer
 */
const CC::Type * AST::Reference::buildType(CC::Context* ctxt) {
    return new CC::Pointer(this->expression->getType(ctxt));
}

/** Skompiluje zistenie adresy prvku v poli.
*/
CC::Assembly* AST::ArrayOffset::byReference(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(this->returnRegister());
	const CC::Array * array = dynamic_cast<const CC::Array *>(this->variable->getType(ctxt));
	if (array == NULL) throw SemanticException("Request of subscript on something which is not array!");
	REGISTER address_register = code->append(this->variable->byReference(ctxt));
	REGISTER offset_register = code->append(this->offset->byValue(ctxt));
	unsigned cell_size = array->getCellType()->sizeOf();
	REGISTER displacement_register = ctxt->getRegisters()->alloc(REGISTER_IMMEDIATE);
	code->appendCode(opcode("ILOAD", displacement_register, (cell_size >> 8) & 0xFF));
	code->appendCode(opcode("ILOAD", displacement_register, cell_size & 0xFF));
	code->appendCode(opcode("MUL", offset_register, displacement_register));
	ctxt->getRegisters()->free(displacement_register);
	return code;
}
	
CC::Assembly* AST::Dereference::byValue(CC::Context* ctxt) {
	CC::Assembly * code = ctxt->createAssembly(this->returnRegister());
	REGISTER addr_register = code->append(this->expression->byValue(ctxt));
	REGISTER value_register = this->returnRegister();
	if (value_register == 0xFF) value_register = ctxt->getRegisters()->alloc(REGISTER_GENERIC);
	code->appendCode(opcode("LOAD", addr_register, INDIRECT, value_register, std::string("dereferencing address in register ") + convertInt(addr_register)));
	code->appendCode(ctxt->getRegisters()->free(addr_register));
	code->setReturnRegister(value_register);
	return code;
}

const CC::Type * AST::Dereference::buildType(CC::Context* ctxt) {
	const CC::Pointer * pointer_type = dynamic_cast<const CC::Pointer *>(this->expression->getType(ctxt));
	if (pointer_type != NULL) return pointer_type->getPointedType();
	throw SemanticException("You cannot dereference something which is not pointer or variable");
}

CC::Assembly* AST::StructMember::byReference(CC::Context* ctxt) {
	throw SemanticException("Not implemented yet: access struct member");
	return NULL;
}

const CC::Type* AST::StructMember::buildType(CC::Context* ctxt) {
	const CC::Struct * _struct = dynamic_cast<const CC::Struct *>(this->expression->getType(ctxt));
	if (_struct == NULL) throw SemanticException("Requested member in something which is not struct or union!");
	return _struct->getMemberType(this->member);
}

const CC::Type* AST::Neg::buildType(CC::Context* ctxt) {
    return this->argument->getType(ctxt);
}

CC::VariableSet * AST::MemberList::getVariables() {
	CC::VariableSet * set = new CC::VariableSet();
	for (MemberList::iterator it = this->begin(); it != this->end(); ++it) {
		set->append(new CC::Variable(*(*it))); 				// skopirujeme premennu z MemberListu do VariableSet-u
	}
	return set;
}

CC::Assembly * AST::Conditional::byReference(CC::Context * ctxt) {
	throw SemanticException("Unimplemented yet: ternary operator!");
}

CC::Assembly * AST::Conditional::byValue(CC::Context * ctxt) {
	throw SemanticException("Unimplemented yet: ternary operator!");
}

const CC::Type * AST::Conditional::buildType(CC::Context * ctxt) {
	if (*(this->true_command->getType(ctxt)) == *(this->false_command->getType(ctxt))) return this->true_command->getType(ctxt);
	else throw SemanticException(std::string("Type mismatch of ternary operator arguments (") + this->true_command->getType(ctxt)->humanName() + " vs. " + this->false_command->getType(ctxt)->humanName() + ")");
}

CC::Assembly * AST::Lvalue::byValue(CC::Context * ctxt) {
	CC::Assembly * code = ctxt->createAssembly(this->returnRegister());
	REGISTER value_reg = this->returnRegister();
	if (value_reg == 0xFF) value_reg = ctxt->getRegisters()->alloc(REGISTER_GENERIC);
	REGISTER addr_reg = code->append(this->byReference(ctxt));
	code->appendCode(opcode("LOAD", addr_reg, INDIRECT, value_reg));
	code->setReturnRegister(value_reg);
	return code;
}

CC::StorageClass * AST::Conditional::getStorageClass(CC::Context * ctxt) {
	throw SemanticException("Unsupported operation: ternary operator");
}

CC::StorageClass * AST::Variable::getStorageClass(CC::Context * ctxt) {
	CC::Variable * var = ctxt->findVariable(this->name);
	if (var == NULL) throw SemanticException(std::string("Variable `") + this->name + "' not found!");
	return var->getStorageClass();
	
}
