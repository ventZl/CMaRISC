#ifndef __MCC_COMPILE_H__
#define __MCC_COMPILE_H__

#include <string>
#include <list>
#include <set>
#include <map>

#include <typeinfo>
#include "assembly.h"
#include <stdio.h>
#include <exception.h>
#include <stdlib.h>

namespace AST {
	class Program;
};

#define ALL_CONTEXTS 					0x01
#define CURRENT_CONTEXT 				0x02
#define ALL_TYPES 						0x04
#define COMPLETE_TYPE 					0x08

#define REGISTER_IMMEDIATE				1
#define REGISTER_GENERIC				2

#define OP_READ							1
#define OP_WRITE						2

namespace CC {
	class Context;
	
	class Assembly {
	protected:
		Assembly(REGISTER return_register): return_register(return_register) { }
		~Assembly() { }
		void bindContext(Context * ctxt) { this->context = ctxt; }
		
	public:
		REGISTER append(Assembly * assembly);					// vrati to register v ktorom je navratova hodnota prilepeneho kodu, alebo 0xFF, ak ziadna navratova hodnota nie je
		REGISTER getReturnRegister() { return this->return_register; }
		void appendCode(std::string code) { this->instructions += code; }
		void setReturnRegister(REGISTER reg) { if (this->return_register != 0xFF && this->return_register != reg) { fprintf(stderr, "Attempt to redefine return register! Will cause register allocator leak! Aborting!\n"); abort(); } this->return_register = reg; }
		std::string getInstructions() { return this->instructions; }
		
	protected:
		
		REGISTER return_register;
		std::string instructions;
		Context * context;
		
//		friend class Assembly;
		friend class Context;
	};
	
	class Variable;
	
	class StorageClass {
	public:
		virtual std::string useAddress(REGISTER addr_reg, REGISTER dst_reg, unsigned operation) = 0;
	};
	
	class VoidStorageClass: public StorageClass {
		virtual std::string useAddress(REGISTER addr_reg, REGISTER dst_reg, unsigned operation) { throw SemanticException("Cannot use something which is not Lvalue in such way"); };
	};
	
	class MemoryStorageClass: public StorageClass {
		virtual std::string useAddress(REGISTER addr_reg, REGISTER dst_reg, unsigned operation);
	};
	
	class RegisterStorageClass: public StorageClass {
		virtual std::string useAddress(REGISTER addr_reg, REGISTER dst_reg, unsigned operation);
	};
	
	class Allocator {
	public:
		virtual Assembly * getAddress(REGISTER return_reg, Context * ctxt, Variable * var) = 0;
		virtual StorageClass * getStorageClass() = 0;
	};
	
	class Type {
	public:
		Type(std::string name): name(name) {}
		virtual bool isComplete() const = 0;
		virtual std::string humanName() const { return std::string("(unknown: ") + this->name + std::string(")"); }
		std::string getName() const { return this->name; }
		virtual unsigned int sizeOf() const = 0;
		bool operator==(const Type &other) const { if (typeid(*this) == typeid(other)) return true; else return false; }
		
	protected:
		std::string name;
	};
	
	class Variable {
	public:
		Variable(std::string name): type_spec(NULL), name(name), allocator(NULL) {}
		Variable(std::string name, const Type * type_spec, CC::Allocator * alloc): type_spec(type_spec), name(name), allocator(alloc) {}
		virtual ~Variable() { delete this->type_spec; }
		
		void setCacheRegister(REGISTER cache_register);
		REGISTER getCacheRegister(Context * ctxt);
		void clearCacheRegister();
		std::string getName() { return this->name; }
		void completeType(CC::Context * ctxt);		/* skompletizuje typ premennej na zaklade toho v akom mieste kodu je deklarovana */
		const Type * getType() { return this->type_spec; }
		Assembly * getAddress(Context * ctxt);
		StorageClass * getStorageClass() { return this->allocator->getStorageClass(); }
		
	protected:
		REGISTER cache_register;
		const Type * type_spec;
		std::string name;
		CC::Allocator * allocator;
		int flags;
		
//		friend AST::VariableDefinition::Compile(CC::Context * ctxt);
	};
	
	class VariableSet: public std::map<std::string, Variable *> {
	public:
		VariableSet(): auto_allocated(0) {}
		unsigned allocAutomaticVariable(unsigned size) { unsigned temp_offs = this->auto_allocated; auto_allocated += size; return temp_offs; }
		void append(Variable * var) { this->insert(std::pair<std::string, Variable *>(var->getName(), var)); }
		unsigned getSize() const;
		Variable * find(std::string name);
		
	protected:
		unsigned auto_allocated;
	};
	
	class TypeSet: public std::map<std::string, Type *> {
	public:
		void append(Type * user_type) { this->insert(std::pair<std::string, Type *>(user_type->getName(), user_type)); }
		Type * find(std::string name);
	};
	
	class Type;
	
	class RegisterSet {
	public:
		RegisterSet();
		
		REGISTER alloc(int flags);			/* potrebujem nejaky register nejakeho typu */
		std::string hook(REGISTER reg);			/* potrebujem pouzit konkretny register */
		std::string free(REGISTER reg);					/* idem uvolnit register */
		unsigned short getRegisterUsageMask();
		void bindVariable(REGISTER reg, Variable * variable);
		REGISTER findVariable(Variable * variable);
		void unbindVariable(REGISTER reg);
		
	protected:
		Variable * binding[16];
		int depth_used[16];
	};
	
	class Context {
	public:
		Context();
		~Context();
		std::string genUniqueLabel();
		RegisterSet * getRegisters() { return this->registers; }
		
		Assembly * createAssembly(REGISTER return_register);
		void destroyAssembly(Assembly * assembly);
		
		Variable * findVariable(std::string name);
		Variable * declareVariable(std::string name, Type * type_spec, CC::Allocator * storage);
		
		Assembly * compileProgram(AST::Program * program);
		void declareType(Type * type);
		Type * useType(Type * type);
		Type * findType(const CC::Type* type, int flags);
		bool existsType(Type * type, int flags);
		unsigned getAutoOffset() const;
		void pushVariableSet(VariableSet * vs) { this->variable_stack.push_back(vs); }
		VariableSet * getVariableSet() { return this->variable_stack.back(); }
		VariableSet * popVariableSet() { VariableSet * vs = this->variable_stack.back(); this->variable_stack.pop_back(); return vs; }
		void pushTypeSet(TypeSet * ts) { this->type_stack.push_back(ts); }
		TypeSet * popTypeSet() { TypeSet * ts = this->type_stack.back(); this->type_stack.pop_back(); return ts; }
		
	protected:
		void pushAssembly(Assembly * assembly);
		bool popAssembly(Assembly * assembly);
		
		
		unsigned label_counter;
		RegisterSet * registers;
		std::list<Assembly *> assembly_stack;
		std::list<VariableSet *> variable_stack;
		std::list<TypeSet *> type_stack;
	};
	
	class LocalVariableAllocator: public Allocator {
	public:
		LocalVariableAllocator(Context * ctxt, unsigned size): offset(ctxt->getVariableSet()->allocAutomaticVariable(size)) {}
		Assembly* getAddress(REGISTER return_reg, CC::Context* ctxt, CC::Variable* var);
		StorageClass * getStorageClass() { return new MemoryStorageClass(); }
	
	protected:
		unsigned offset;
	};
	
	class RegisterAllocator: public Allocator {
	public:
		RegisterAllocator(unsigned number): offset(number) {}
		virtual Assembly * getAddress(REGISTER return_reg, Context * ctxt, Variable * var);
		StorageClass * getStorageClass() { return new RegisterStorageClass(); }
		
	protected:
		unsigned offset;
	};
	
	class StaticVariableAllocator: public Allocator {
	public:
		StaticVariableAllocator(std::string name): name(name) {}
		virtual Assembly * getAddress(REGISTER return_reg, Context * ctxt, Variable * var);
		StorageClass * getStorageClass() { return new MemoryStorageClass(); }
		
	protected:
		std::string name;
	};

	class IncompleteType: public Type {
	public:
		IncompleteType(std::string type_name): Type(type_name) {}
		virtual bool isComplete() const { return false; }
		unsigned int sizeOf() const { throw new SemanticException("Invalid use of incomplete type"); }
	};
	
	class CompleteType: public Type {
	public:
		CompleteType(std::string name): Type(name) {}
		virtual bool isComplete() const { return true; }
	};
	
	class IncompleteStruct: public IncompleteType {
	public:
		IncompleteStruct(std::string type_name): IncompleteType(type_name) {}
		virtual std::string humanName() const { return std::string("struct ") + this->name; }
		bool operator==(const Type &other) const ;
	};
	
	class IncompleteUnion: public IncompleteType {
	public:
		IncompleteUnion(std::string type_name): IncompleteType(type_name) {}
		virtual std::string humanName() const { return std::string("union ") + this->name; }
		bool operator==(const Type &other) const ;
	};

	class Void: public CompleteType {
	public:
		Void(): CompleteType("void") {}
		virtual unsigned int sizeOf() const { return 0; }
		virtual std::string humanName() const { return std::string("void"); }
	};
	
	class ScalarType: public CompleteType {
	public:
		ScalarType(): CompleteType("") {}
	};
	
	class Integer: public ScalarType {
	public:
		virtual unsigned int sizeOf() const { return 2; }
		virtual std::string humanName() const { return std::string("int"); }
	};
	
	class Char: public ScalarType {
		virtual unsigned int sizeOf() const { return 1; }
		virtual std::string humanName() const { return std::string("char"); }
	};
	
	class FunctionType: public CompleteType {
	public:
		FunctionType(std::string name, const Type * return_type, VariableSet * arguments): CompleteType(name), return_type(return_type), arguments(arguments) {}
		virtual std::string humanName() const { return this->name + std::string("()"); }
		virtual unsigned int sizeOf() const { return 2; }
		bool operator==(const Type &other) const ;
		
		const Type * getReturnType() const { return this->return_type; }
		VariableSet * getArguments() const { return this->arguments; }
		
	protected:
		VariableSet * arguments;
		const Type * return_type;
	};
	
	class MemberType: public CompleteType {
	public:
		MemberType(std::string name, VariableSet * members): CompleteType(name), members(members) {}
		virtual unsigned getMemberOffset(std::string name) const = 0;
		const Type * getMemberType(std::string name) const ;
		
	protected:
		VariableSet * members;
	};
	
	class Struct: public MemberType {
	public:
		Struct(std::string name, VariableSet * members): MemberType(name, members) {}
		virtual unsigned int sizeOf() const;
		virtual unsigned getMemberOffset(std::string name) const;
		virtual std::string humanName() const { return std::string("struct ") + this->name; }
		bool operator==(const Type &other) const ;
	};
	
	class Union: public MemberType {
	public:
		Union(std::string name, VariableSet * members): MemberType(name, members) {}
		virtual unsigned int sizeOf() const;
		virtual unsigned getMemberOffset(std::string name) const { return 0; }
		virtual std::string humanName() const { return std::string("union ") + this->name; }
		bool operator==(const Type &other) const ;
	};
	
	class Array: public CompleteType {
	public:
		Array(const Type * cell_type): CompleteType(""), cell_type(cell_type) {}
		Array(std::string name, const Type * cell_type): CompleteType(name), cell_type(cell_type) {}
		const Type * getCellType() const { return this->cell_type; }
		virtual unsigned int sizeOf() const { return 2; }
		virtual std::string humanName() const { return this->cell_type->humanName() + std::string("[]"); }
		bool operator==(const Type &other) const ;
		
	protected:
		const Type * cell_type;
	};
	
	class Pointer: public CompleteType {
	public:
		Pointer(const Type * pointed_type): CompleteType(""), pointed_type(pointed_type) {}
		Pointer(std::string name, const Type * pointed_type): CompleteType(name), pointed_type(pointed_type) {}
		virtual unsigned int sizeOf() const { return 2; }
		const Type * getPointedType() const { return this->pointed_type; }
		virtual std::string humanName() const { return this->pointed_type->humanName() + std::string(" *"); }
		
	protected:
		const Type * pointed_type;
	};
	
	
};

#endif
