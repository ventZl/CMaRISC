#include <iostream>
#include <stdio.h>

#include <compile.h>
#include <nodes.h>
#include <exception.h>
#include <stdlib.h>

REGISTER CC::RegisterSet::alloc(int flags) {
	unsigned bound_up;
	if (flags == REGISTER_GENERIC) bound_up = 11;
	else if (flags == REGISTER_IMMEDIATE) bound_up = 7;
	for(int q = bound_up; q >= 0; --q) {
		if (this->depth_used[q] == 0) {
			this->depth_used[q]++;
			if (q == 0) printf("Aquiring register 0!\n");
			return (REGISTER) q;
		}
	}
	fprintf(stderr, "INTERNAL ERROR: Unable to allocate register. All registers are claimed. Always free all claimed registers after they are used.\n");
	_Exit(0);
}

CC::RegisterSet::RegisterSet() {
	for (int q = 0; q < 16; ++q) {
		this->depth_used[q] = 0;
		this->binding[q] = NULL;
	}
}


std::string CC::RegisterSet::hook(REGISTER reg) {
	std::string assembly;
	
	if (this->depth_used[reg] > 0) {
		printf("Depth used of R%d is %d\n", reg, this->depth_used[reg]);
		assembly = opcode("PUSH", reg);
	}
	this->depth_used[reg]++;
	return assembly;
}

std::string CC::RegisterSet::free(REGISTER reg) {
	std::string assembly;
	
	if (reg > 15) {
		fprintf(stderr, "INTERNAL ERROR: Attempt to free nonexistent register %d!\n", reg);
		abort();
	}
	
	if (this->depth_used[reg] > 1) {
		assembly = opcode("POP", reg);
	}
	
	this->depth_used[reg]--;
	return assembly;
	
}

short unsigned int CC::RegisterSet::getRegisterUsageMask() {
	short unsigned mask = 0;
	for (int q = 0; q < 16; q++) {
		if (this->depth_used[q] > 0) mask |= (1 << q);
	}
	return mask;
}


CC::Context::Context() {
	this->label_counter = 1;
	this->registers = new RegisterSet();
}

CC::Context::~Context() {
	delete this->registers;
}


CC::Assembly * CC::Context::compileProgram(AST::Program * program) {
	this->pushTypeSet(new TypeSet());
	this->pushVariableSet(new VariableSet());
	return program->Compile(this);
}

CC::Assembly * CC::Context::createAssembly(REGISTER return_register) {
	CC::Assembly * ass = new CC::Assembly(return_register);
//	printf("new assembly on %p\n", ass);
	this->pushAssembly(ass);
	ass->bindContext(this);
	return ass;
}

void CC::Context::pushAssembly(CC::Assembly* assembly) {
	this->assembly_stack.push_back(assembly);
	return;
}


bool CC::Context::popAssembly(CC::Assembly* assembly) {
	if (this->assembly_stack.back() != assembly) return false;
	this->assembly_stack.pop_back();
//	printf("deleteing assembly %p\n", assembly);
	delete assembly;
	return true;
}

void CC::Context::destroyAssembly(CC::Assembly* assembly) {
	if (!this->popAssembly(assembly)) {
		int depth = 0;
		for(std::list<CC::Assembly *>::reverse_iterator it = this->assembly_stack.rbegin(); it != this->assembly_stack.rend(); ++it) {
			if (*it != assembly) depth++; else break;
			if (it == this->assembly_stack.rend()) depth = -1;
		}
		if (depth > 0) fprintf(stderr, "INTERNAL ERROR: Attempt to destory assembly not on top of stack (%d above)!\n", depth);
		else fprintf(stderr, "INTERNAL ERROR: Attempting to destroy assembly not in stack!\n");
		abort();
	}
	return;
}

CC::Variable* CC::Context::declareVariable(std::string name, CC::Type* type_spec, CC::Allocator* storage) {
	Variable * var = new Variable(name, type_spec, storage);
	//	printf("new var = %p\n", var);
	this->getVariableSet()->append(var);
	return var;
}

CC::Variable* CC::Context::findVariable(std::string name) {
	for (std::list<VariableSet *>::reverse_iterator it = this->variable_stack.rbegin(); it != this->variable_stack.rend(); ++it) {
		Variable * var = (*it)->find(name);
		if (var != NULL) return var;
	}
	return NULL;
}

std::string CC::Context::genUniqueLabel() {
	char c[32];
	snprintf(c, 31, ".label%d", this->label_counter++);
	return c;
}

CC::Type* CC::Context::findType(const CC::Type* type, int flags) {
	std::list<TypeSet *>::reverse_iterator it = this->type_stack.rbegin();
	int c_level = 0;
	for (; it != this->type_stack.rend(); ++it) {
		std::map<std::string, Type *>::iterator tit = (*it)->begin();
		for (; tit != (*it)->end(); ++tit) {
			if (type == tit->second) {
				if (!(flags & COMPLETE_TYPE) || tit->second->isComplete()) return tit->second;
				return NULL;
			}
		}
		if (flags & CURRENT_CONTEXT) return NULL;
	}
	return NULL;
}

bool CC::Context::existsType(Type * type, int flags) {
	std::list<TypeSet *>::reverse_iterator it = this->type_stack.rbegin();
	for (; it != this->type_stack.rend(); ++it) {
		std::map<std::string, Type *>::iterator tit = (*it)->begin();
		for (; tit != (*it)->end(); ++it) {
			if (type == tit->second) {
				if (!(flags & COMPLETE_TYPE) || tit->second->isComplete()) return true;
				return false;
			}
		}
		if (flags & CURRENT_CONTEXT) return false;
	}
	return false;
}


void CC::Context::declareType(Type * type) {
	Type * existing_type = this->findType(type, CURRENT_CONTEXT);
	if (existing_type != NULL) {
		if (existing_type->isComplete() && type->isComplete()) throw new SemanticException(std::string("Redefinition of type ") + existing_type->humanName());
	}
	this->type_stack.back()->append(type);
	//	this->type_stack.back()->append(std::pair<std::string, Type *>(type));
	return;
}

REGISTER CC::Assembly::append(CC::Assembly* assembly) {
	REGISTER reg;
	//	fprintf(stderr, "Merging assembly (%p)\n=================\n%s\n", assembly, assembly->getInstructions().c_str());
	this->instructions += assembly->getInstructions();
	reg = assembly->getReturnRegister();
	//	fprintf(stderr, "Return register is %d\n", reg);
	this->context->destroyAssembly(assembly);
	return reg;
}

void CC::Variable::completeType(CC::Context* ctxt) {
	if (this->type_spec == NULL) throw new SemanticException("INTERNAL ERROR: Variable with NULL type specification!");
	if (this->type_spec->isComplete()) return;
	Type * complete_type = ctxt->findType(this->type_spec, COMPLETE_TYPE);
	if (complete_type == NULL) throw new SemanticException(std::string("Invalid use of incomplete type `") + this->type_spec->humanName() + std::string("'"));
	this->type_spec = complete_type;
	return;
}

const CC::Type* CC::MemberType::getMemberType(std::string name) const {
	for (VariableSet::iterator it = this->members->begin(); it != this->members->end(); ++it) {
		if (name == it->first) return it->second->getType();
	}
	throw new SemanticException(std::string("Unknown member `") + name + std::string("in variable of type `") + this->humanName() + std::string("'"));
}

CC::Assembly * CC::LocalVariableAllocator::getAddress(REGISTER return_reg, CC::Context* ctxt, CC::Variable* var) {
	Assembly * code = ctxt->createAssembly(return_reg);
	REGISTER a_reg = ctxt->getRegisters()->alloc(REGISTER_GENERIC);
	code->setReturnRegister(a_reg);
	code->appendCode(opcode("MOV", a_reg, REG_BP));
	if (this->offset > 15) {
		fprintf(stderr, "WARNING: Automatic variable offset greater than 15, operation will be slow!\n");
		REGISTER o_reg = ctxt->getRegisters()->alloc(REGISTER_IMMEDIATE);
		code->appendCode(opcodeC("ILOAD", o_reg, (this->offset >> 8) & 0xFF));
		code->appendCode(opcodeC("ILOAD", o_reg, this->offset & 0xFF));
		code->appendCode(opcode("SUB", a_reg, o_reg));
		code->appendCode(ctxt->getRegisters()->free(o_reg));
	} else if (this->offset > 0) {
		code->appendCode(opcodeC("SUBC", a_reg, this->offset));
	}
	return code;
}

CC::Assembly* CC::RegisterAllocator::getAddress(REGISTER return_reg, CC::Context* ctxt, CC::Variable* var) {
	Assembly * code = ctxt->createAssembly(this->offset);
	return code;
}

CC::Assembly* CC::StaticVariableAllocator::getAddress(REGISTER return_reg, CC::Context* ctxt, CC::Variable* var) {
	Assembly * code = ctxt->createAssembly(return_reg);
	REGISTER a_reg = ctxt->getRegisters()->alloc(REGISTER_GENERIC);
	code->setReturnRegister(a_reg);
	code->appendCode(opcodeL("ILOAD", a_reg, this->name));
	return code;
}


CC::Variable* CC::VariableSet::find(std::string name) {
	VariableSet::iterator it = this->std::map<std::string, Variable *>::find(name);
	if (it == this->end()) {
//		fprintf(stderr, "warning: variable `%s' not found in this context.\n", name.c_str());
		return NULL; 
	} else {
//		fprintf(stderr, "info: variable `%s' found.\n", name.c_str());
		return it->second;
	}
}

unsigned int CC::VariableSet::getSize() const {
	unsigned size = 0;
	for (VariableSet::const_iterator it = this->begin(); it != this->end(); ++it) {
		size += it->second->getType()->sizeOf();
	}
	return size;
}

CC::Assembly * CC::Variable::getAddress(CC::Context* ctxt) {
	return this->allocator->getAddress(0xFF, ctxt, this);
}

unsigned int CC::Struct::getMemberOffset(std::string name) const {
	unsigned offset = 0;
	for (VariableSet::iterator it = this->members->begin(); it != this->members->end(); ++it) {
		if (it->first == name) return offset;
		offset += it->second->getType()->sizeOf();
	}
	throw new SemanticException("Request of nonexistent member of struct!");
}

unsigned int CC::Struct::sizeOf() const {
	unsigned offset = 0;
	for (VariableSet::iterator it = this->members->begin(); it != this->members->end(); ++it) {
		offset += it->second->getType()->sizeOf();
	}
	return offset;
}

bool CC::Struct::operator==(const CC::Type& other) const { 
	if (this->getName() != other.getName()) return false;
	if (typeid(*this) == typeid(other)) return true; 
	if (typeid(other) == typeid(CC::IncompleteStruct)) return true;
	return false; 
	if (typeid(*this) == typeid(other)) return true; else return false; 
}

unsigned int CC::Union::sizeOf() const {
	unsigned maxsize = 0, size;
	
	for (VariableSet::iterator it = this->members->begin(); it != this->members->end(); ++it) {
		size = it->second->getType()->sizeOf();
		if (size > maxsize) maxsize = size;
	}
	return size;
}

bool CC::Union::operator==(const CC::Type& other) const { 
	if (this->getName() != other.getName()) return false;
	if (typeid(*this) == typeid(other)) return true; 
	if (typeid(other) == typeid(CC::IncompleteUnion)) return true;
	return false; 
	if (typeid(*this) == typeid(other)) return true; else return false; 
}

bool CC::IncompleteStruct::operator==(const CC::Type& other) const { 
	if (this->getName() != other.getName()) return false;
	if (typeid(*this) == typeid(other)) return true; 
	if (typeid(other) == typeid(CC::Struct)) return true;
	return false; 
}

bool CC::IncompleteUnion::operator==(const CC::Type& other) const { 
	if (this->getName() != other.getName()) return false;
	if (typeid(*this) == typeid(other)) return true; 
	if (typeid(other) == typeid(CC::Union)) return true;
	return false; 
}

bool CC::FunctionType::operator==(const CC::Type& other) const { 
	const CC::FunctionType * f_other = dynamic_cast<const CC::FunctionType *>(&other);
	if (this->getName() != other.getName()) return false;
	if (typeid(other) != typeid(CC::FunctionType)) return false; 
	if (typeid(*(this->getReturnType())) != typeid(*(f_other->getReturnType()))) return false;
	return true; 
	// TODO porovnat aj typy argumentov!
}

bool CC::Array::operator==(const CC::Type& other) const { 
	return false; 
}

std::string CC::RegisterStorageClass::useAddress(REGISTER addr_reg, REGISTER dst_reg, unsigned operation) {
	if (operation == OP_READ) {
		return opcode("MOV", dst_reg, addr_reg);
	} else return opcode("MOV", addr_reg, dst_reg);
}

std::string CC::MemoryStorageClass::useAddress(REGISTER addr_reg, REGISTER dst_reg, unsigned operation) {
	if (operation == OP_READ) {
		return opcode("LOAD", addr_reg, INDIRECT, dst_reg);
	} else return opcode("STORE", addr_reg, INDIRECT, dst_reg);
}