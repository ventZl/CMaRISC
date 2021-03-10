#ifndef __MCC_NODES_H__
#define __MCC_NODES_H__

#include <list>

#include <compile.h>
#include <stdlib.h>

std::string mktab(int n);

namespace AST {

/** Top level objekt pre vsetky triedy popisujuce konstrukty AST
 */
class Node {
public:
	virtual std::string dump(int L) = 0;
};

/** Statement - konstrukty, ktore nevracaju navratovu hodnotu
 * Statementy su tie konstrukty, ktore bud deklaruju nove objekty
 * v programe, alebo definuju prvky funkcie, ktore same o sebe
 * nemaju ziadnu navratovu hodnotu.
 */
class Statement: public Node {
public:
	virtual CC::Assembly * Compile(CC::Context * ctxt) = 0;
};

/** Vyraz - konstrukty, ktore generuju navratovu hodnotu
 * Vyrazy su tie konstrukty, ktore vytvaraju navratovu hodnotu.
 * Napr. pouzitie premennej generuje navratovu hodnotu rovnu
 * aktualnej hodnote ulozenej v premennej.
 */
class Expression: public Node {
public:
	Expression(CC::Type * type): type(type)/*, lvalue(lvalue)*/ { this->return_reg = 0xFF; }
	unsigned short returnRegister() { return this->return_reg; }
	void setReturnRegister(unsigned short reg) { this->return_reg = reg; }
	const CC::Type * const getType(CC::Context * ctxt) { if (this->type == NULL) this->type = this->buildType(ctxt); return this->type; }
	virtual const CC::Type * buildType(CC::Context * ctxt) { return this->type; }
//	virtual bool const isLvalue() { return this->lvalue; }
	virtual CC::Assembly * byValue(CC::Context * ctxt) = 0;
	virtual CC::Assembly * byReference(CC::Context * ctxt) = 0;
	virtual CC::StorageClass * getStorageClass(CC::Context * ctxt) = 0;
	
protected:
	unsigned short return_reg;
	const CC::Type * type;
};

/** Lvalue - left-hand operand value - ma storage class 
 * Operand tohto typu sa moze objavit na lavej aj pravej strane priradovacieho
 * vyrazu. Ma nejaky storage class (typicky - premenne, alebo registre) a je donho
 * mozne hodnotu ulozit.
 */
class Lvalue: public Expression {
public:
	Lvalue(CC::Type * type): Expression(type) {}
	virtual CC::Assembly * byValue(CC::Context * ctxt);
};

/** Rvalue - right-hand operand value - nema storage class 
 * Operand tohto typu sa moze objavit iba na pravej strane priradovacieho vyrazu.
 * Nema ziaden storage class (typicky - volanie funkcie, numericka konstanta, vysledok
 * konstantneho vypoctu), preto nie je kam ulozit hodnotu.
 */
class Rvalue: public Expression {
public:
	Rvalue(CC::Type * type): Expression(type) {}
	virtual CC::Assembly * byReference(CC::Context * ctxt) { throw SemanticException("Operand is not Lvalue, cannot reference"); }
	virtual CC::StorageClass * getStorageClass(CC::Context * ctxt) { return new CC::VoidStorageClass(); }
};

/** Zoznam vyrazov
 * Zoskupuje viacero vyrazov do usporiadaneho zoznamu.
 */
class ExpressionList: public std::list<Expression *> {
public:
	ExpressionList() {}
	void append(Expression * node) { this->push_back(node); }
	virtual std::string dump(int L) { std::string reg = mktab(L) + "[\n"; for (std::list<Expression *>::iterator it = this->begin(); it != this->end(); ++it) { if (it != this->begin()) reg += ",\n"; reg += (*it)->dump(L + 1); } return reg + "\n" + mktab(L) + "]"; }
};

/** Zoznam objektov
 * Zoskupuje viacero objektov do usporiadaneho zoznamu.
 */
class StatementList: public std::list<Statement *>, public Statement {
public:
	StatementList() {}
	void append(Statement * node) { this->push_back(node); }
	virtual CC::Assembly * Compile(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "[\n"; for (std::list<Statement *>::iterator it = this->begin(); it != this->end(); ++it) { if (it != this->begin()) reg += ",\n"; reg += (*it)->dump(L + 1); } return reg + "\n" + mktab(L) + "]"; }
};

/** Top level uzol AST.
 * Tento uzol sa nevytvara parsovanim zdrojoveho kodu, ale
 * je vytvoreny automaticky spustenim parsovania. Vsetky uzly
 * vyparsovane zo zdrojoveho kodu su potomkami tohto uzla. A preklad
 * programu sa zacina rovnako od tohto uzla.
 */
class Program: public Node {
public:
	Program(StatementList * objects): statements(objects) {}
	virtual CC::Assembly * Compile(CC::Context * ctxt);
	virtual std::string dump(int L) { return mktab(L) + "Program(\n" + this->statements->dump(L + 1) + "\n" + mktab(L) + ")"; }
	
protected:
	StatementList * statements;
};

/** Objekt definicie noveho typu.
 * @note Tento prekladac C nie je tak benevolentny ako standard (kvoli zjednoduseniu logiky)
 * a umoznuje definovat nove strukturovane typy iba po zadani klucoveho slova typedef
 */
class TypeDef: public Statement {
public:
	TypeDef(CC::Type * type): type(type) {}
	virtual CC::Assembly * Compile(CC::Context * ctxt);
	virtual std::string dump(int L) { return mktab(L) + "TypeDef(\n" + this->type->humanName() + mktab(L) + ")"; }
	
protected:
	CC::Type * type;
};

/** Zoznam premennych.
 * Zoskupuje premenne v nejakom kontexte do usporiadaneho zoznamu.
 */
class VariableList: public std::list<CC::Variable *> {
public:
	VariableList() {}
	virtual void append(CC::Variable * var) = 0;
	virtual std::string dump(int L) { std::string reg = mktab(L) + "[ "; for (std::list<CC::Variable *>::iterator it = this->begin(); it != this->end(); ++it) { if (it != this->begin()) reg += ", "; reg += (*it)->getType()->humanName() + " " + (*it)->getName(); } return reg + " ]"; }
};

/** Definicia premennej.
 * Definicia premennej sama o sebe negeneruje ziaden kod (preto je statement).
 * Vyhradi novu premennu v aktualne najvyssom namespace premennych (ak v tomto uz
 * taka premenna je, vznikne chyba) a pripadne priradi inicializacny vyraz.
 */
class VariableDefinition: public Statement {
public:
	VariableDefinition(CC::Type * type, std::string name, Expression * initializer): type(type), name(name), initializer(initializer) {}
	virtual CC::Assembly * Compile(CC::Context * ctxt);
	virtual std::string dump(int L) { return mktab(L) + std::string("VariableDefinition(") + this->type->humanName() + ", \"" + this->name + "\",\n" + (this->initializer == NULL ? mktab(L + 1) + "NULL\n" : this->initializer->dump(L + 1)) + mktab(L) + ")"; }
	
protected:
	CC::Type * type;
	std::string name;
	Expression * initializer;
};

/** Zoznam argumentov funkcie.
 */
class ArgumentList: public VariableList {
public:
	virtual void append(CC::Variable * var) { /* TODO add allocator */ this->push_back(var); }
};

/** Zoznam clenskych premennych funkcie.
 */
class MemberList: public VariableList {
public:
	MemberList() {}
	virtual void append(CC::Variable * var) { /* TODO add allocator */ this->push_back(var); }
	CC::VariableSet * getVariables(); ///<<< Vrati zoznam premennych so spravne nastavenym alokatorom
};

/** Definicia funkcie.
 * Samotna definicia funkcie negeneruje ziaden zdrojovy kod, ten generuje az definicia jej tela.
 * Pri definovani funkcie sa overi, ci uz funkcia s takym nazvom neexistuje.
 */
class Function: public Statement {
public:
	Function(CC::Type * type, std::string name, ArgumentList * arguments, StatementList * statements): return_type(type), name(name), arguments(arguments), commands(statements) {}
	virtual CC::Assembly * Compile(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "Function(" + this->return_type->humanName() + ", \"" + this->name + "\",\n" + this->arguments->dump(L + 1) + ",\n" + this->commands->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	
protected:
	const CC::Type * return_type;
	std::string name;
	ArgumentList * arguments;
	StatementList * commands;
};

/** Pouzitie premennej.
 * Vyraz sposobi pouzitie bud hodnoty premennej, alebo jej adresy v zavislosti na kode.
 * Premenna ma storage class, preto je Lvalue.
 */
class Variable: public Lvalue {
public:
	Variable(std::string name): Lvalue(NULL), name(name) {}
	virtual CC::Assembly * byReference(CC::Context * ctxt);
	virtual const CC::Type * buildType(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "Variable(\"" + this->name + "\")"; return reg; }
	virtual CC::StorageClass * getStorageClass(CC::Context* ctxt);
	virtual CC::Assembly * byValue(CC::Context * ctxt);
		//	virtual bool const isLvalue() { return this->lvalue; }
	
protected:
	std::string name;
};

class Reference;

/** Dereferencovanie adresy.
 * Vyraz vrati hodnotu ulozenu na adrese danej argumentom tejto funkcie.
 * Hodnota (cislo) nema storage class, preto je Rvalue.
 */
class Dereference: public Rvalue {
public:
	Dereference(Expression * expression): Rvalue(NULL), expression(expression) {}
	virtual CC::Assembly * byValue(CC::Context * ctxt);
	virtual const CC::Type * buildType(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "Dereference(\n" + this->expression->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	
protected:
	Expression * getInferiorExpression() const { return this->expression; }
	Expression * expression;
	
	friend class Reference;
};

/** Pouzitie clena struktury.
 * Vyraz sposobi pouzitie bud hodnoty, alebo adresy clena instancie struktury.
 * Clen struktury ma storage class, preto je Lvalue
 */
class StructMember: public Lvalue {
public:
	StructMember(Expression * expression, std::string member): Lvalue(NULL), expression(expression), member(member) {}
	virtual CC::Assembly * byReference(CC::Context * ctxt);
	virtual const CC::Type * buildType(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "StructMember(\n" + this->expression->dump(L + 1) + ", " + this->member + "\n" + mktab(L) + ")"; return reg; }
	virtual CC::StorageClass * getStorageClass(CC::Context * ctxt) { return this->expression->getStorageClass(ctxt); }
	
protected:
	Expression * expression;
	std::string member;
};

/** Pouzitie prvku pola.
 * Vyraz sposobi pouzitie bud hodnoty, alebo adresy prvku pola.
 * Prvok pola ma storage class, preto je Lvalue
 */
class ArrayOffset: public Lvalue {
public:
	ArrayOffset(Expression * variable, Expression * offset): Lvalue(NULL), variable(variable), offset(offset) {}
	virtual CC::Assembly * byReference(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "ArrayOffset(\n" + this->variable->dump(L + 1) + ", " + this->offset->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	virtual CC::StorageClass * getStorageClass(CC::Context * ctxt) { return this->variable->getStorageClass(ctxt); }
	
protected:
	Expression * variable;
	Expression * offset;
};

class Assign;

/** Ziskanie adresy vyrazu, ktory musi byt Lvalue.
 * Vrati adresu podvyrazu. Podvyraz musi byt Lvalue, pretoze musi mat storage class.
 * Adresa prvku je ciselna hodnota, preto je Rvalue.
 */
class Reference: public Rvalue {
public:
	Reference(Expression * expression): Rvalue(NULL), expression(expression) {}
	virtual CC::Assembly * byValue(CC::Context * ctxt);
	virtual const CC::Type * buildType(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "Reference(\n" + this->expression->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	
protected:
	Expression * getInferiorExpression() { return this->expression; }
	
	Expression * expression;
	
	friend class Assign;
};

/** Automaticky generovany konstrukt programu pre zapuzdrenie vyrazov, ktorych hodnota nie je pouzita.
 */
class UnusedValue: public Statement {
public:
	UnusedValue(Expression * node): child(node) { this->child = node; }
	virtual CC::Assembly * Compile(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "UnusedValue(\n" + this->child->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	
protected:
	Expression * child;
};

/** Vygeneruje kod pre navrat z funkcie.
 * V pripade, ze funkcia ma navratovy typ iny ako void, preda volajucemu kodu navratovu hodnotu.
 */
class Return: public Statement {
public:
	Return(): child(NULL) {}
	Return(Expression * node): child(node) {}
	virtual CC::Assembly * Compile(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "Return(\n" + this->child->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	
protected:
	Expression * child;
};

/** Podmieneny cyklus s podmienkou na zaciatku
 * Vytvori cyklus s podmienkou na zaciatku.
 */
class While: public Statement {
public:
	While(Expression * condition, Statement * command): condition(condition), command(command) {}
	virtual CC::Assembly * Compile(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "While(\n" + this->condition->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	
protected:
	Expression * condition;
	Statement * command;
};

/** Vetvenie.
 * Vytvori podmienene vykonany kod.
 */
class If: public Statement {
public:
	If(Expression * condition, Statement * true_command, Statement * false_command) { this->condition = condition; this->true_command = true_command; this->false_command = false_command; }
	If(Expression * condition, Statement * true_command) { this->condition = condition; this->true_command = true_command; this->false_command = NULL; }
	virtual CC::Assembly * Compile(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "If(\n" + this->condition->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	
protected:
	Expression * condition;
	Statement * true_command;
	Statement * false_command;
};

/** Vytvori vyraz s podmienenou vystupnou hodnotou.
 * Vytvori vyraz, ktoreho vystupna hodnota je zavisla od podmienky.
 * Implementuje ternarny operator ? : v C
 */
class Conditional: public Lvalue {
public:
	Conditional(Expression * condition, Expression * true_command, Expression * false_command): Lvalue(NULL), condition(condition), true_command(true_command), false_command(false_command) {}
	virtual CC::Assembly * byValue(CC::Context * ctxt);
	virtual CC::Assembly * byReference(CC::Context * ctxt);
	virtual const CC::Type * buildType(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "Conditional(\n" + this->condition->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	virtual CC::StorageClass * getStorageClass(CC::Context * ctxt);
	
protected:
	Expression * condition;
	Expression * true_command;
	Expression * false_command;
};

/** Spolocna trieda pre vsetky binarne operatory jazyka C
 */
class BinaryOperation: public Rvalue {
public:
	BinaryOperation(Expression * left, Expression * right): Rvalue(NULL), left(left), right(right), compile_flags(0) {}
	virtual CC::Assembly * byValue(CC::Context * ctxt);
	virtual const CC::Type * buildType(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "BinaryOperation(\n" + this->left->dump(L + 1) + ", \n" + this->right->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags) = 0;
	Expression * left;
	Expression * right;
	int compile_flags;
};

/** Operacia scitania
 */
class Add: public BinaryOperation {
public:
	Add(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia odcitania
 */
class Sub: public BinaryOperation {
public:
	Sub(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia nasobenia
 */
class Mul: public BinaryOperation {
public:
	Mul(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia delenia
 */
class Div: public BinaryOperation {
public:
	Div(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia zvysku po deleni
 */
class Modulo: public BinaryOperation {
public:
	Modulo(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia porovnania dvoch cisel na vztah <
 */
class Lt: public BinaryOperation {
public:
	Lt(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia porovnania dvoch cisel na vztah <=
 */
class Lte: public BinaryOperation {
public:
	Lte(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia porovnania dvoch cisel na vztah >
 */
class Gt: public BinaryOperation {
public:
	Gt(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia porovnania dvoch cisel na vztah >=
 */
class Gte: public BinaryOperation {
public:
	Gte(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia porovnania dvoch cisel na vztah ==
 */
class Equ: public BinaryOperation {
public:
	Equ(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia porovnania dvoch cisel na vztah !=
 */
class Nequ: public BinaryOperation {
public:
	Nequ(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia logickeho sucinu
 */
class And: public BinaryOperation {
public:
	And(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia logickeho suctu
 */
class Or: public BinaryOperation {
public:
	Or(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia logickej nonekvivalencie
 */
class Xor: public BinaryOperation {
public:
	Xor(Expression * left, Expression * right): BinaryOperation(left, right) {}
	
protected:
	virtual std::string genOpcode(REGISTER arg1, REGISTER arg2, unsigned flags);
};

/** Operacia negacie
 */
class Neg: public Rvalue {
public:
	Neg(Expression * argument): Rvalue(NULL), argument(argument) {}
	virtual CC::Assembly * byValue(CC::Context * ctxt);
	virtual const CC::Type * buildType(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "Neg(" + this->argument->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	
protected:
	Expression * argument;
};

/** Priradenie vysledku vyrazu do nejakeho LValue
 * Toto je LValue kvoli tomu, aby bolo mozne spravit a = b = c = 0
 */
class Assign: public Rvalue {
public:
	Assign(Expression * target, Expression * value): Rvalue(NULL), target(target), value(value) {}
	virtual CC::Assembly * byValue(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "Assign(\n" + this->target->dump(L + 1) + ",\n" + this->value->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	
public:
	Expression * target;
//	std::string variable_name;			/// *TODO* potom to treba prerobit tak, aby cielom mohol byt expression, ktory je LVALUE
	Expression * value;
};

/** Celociselna konstanta.
 * Pouzitie celociselnej konstanty vo vypocte. Konstanta je Rvalue, pretoze pochopitelne nema storage class
 */
class Integer: public Rvalue {
public:
	Integer(int value): Rvalue(new CC::Integer()) { this->value = value; }
	virtual CC::Assembly * byValue(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "Integer(" + convertInt(this->value) + ")"; return reg; }
	
protected:
	int value;
};

/** Volanie funkcie
 */
class FunctionCall: public Rvalue {
public:
	FunctionCall(std::string function_name, ExpressionList * arguments): Rvalue(NULL), function_name(function_name), arguments(arguments) {}
	virtual CC::Assembly * byValue(CC::Context * ctxt);
	virtual const CC::Type * buildType(CC::Context * ctxt);
	virtual std::string dump(int L) { std::string reg = mktab(L) + "FunctionCall(" + this->function_name + ",\n" + this->arguments->dump(L + 1) + "\n" + mktab(L) + ")"; return reg; }
	
protected:
	std::string function_name;
	ExpressionList * arguments;
};

};

#endif
