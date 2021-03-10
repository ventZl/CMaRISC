#include "nodes.h"
#include "compile.h"
#include <iostream>

using namespace AST;
using namespace CC;

int main(int argc, char ** argv) {
	StatementList * fnstatements = new StatementList();
	fnstatements->append(new VariableDefinition(new CC::Integer(), "a", NULL));
	fnstatements->append(new UnusedValue(new Assign(new AST::Reference(new AST::Dereference(new AST::Variable("a"))), new AST::Integer(1))));
	fnstatements->append(new UnusedValue(new FunctionCall("first", new ExpressionList())));
	StatementList * fnstatements2 = new StatementList();
	fnstatements2->append(new VariableDefinition(new CC::Integer(), "a", NULL));
	fnstatements2->append(new UnusedValue(new Assign(new AST::Reference(new AST::Dereference(new AST::Variable("a"))), new AST::Integer(1))));
	StatementList * stlist = new StatementList();
	stlist->append(new Function(new Void(), "first", NULL, fnstatements2));
	stlist->append(new Function(new Void(), "main", NULL, fnstatements));
	Program * program = new Program(stlist);
	Context * cc = new Context();
	Assembly * output = cc->compileProgram(program);
	std::cout << "Compiled program dump:" << std::endl;
	std::cout << output->getInstructions() << std::endl;
}