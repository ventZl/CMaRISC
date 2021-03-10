%{
	/* atributy vyrazu */
#include <string.h>

#include <nodes.h>
#include <compile.h>

%}

	/* definicia prenasanych hodnot */
%union  {
	AST::Node * node;
	AST::ArgumentList * argumentl;
	AST::StatementList * statementl;
	AST::MemberList * memberl;
	AST::ExpressionList * expressionl;
	AST::Variable * variable;
	AST::Expression * expression;
	AST::Statement * statement;
	CC::VariableSet * variables;
	CC::Type * type;
	char * string;
	char* meno;
	char* txt;
	int immediate;
	int token;
}

/* definicia term. symbolov a hodnoty nimi prenasane */
%token		INT  CHAR  VOID STRUCT UNION
%token		RETURN IF ELSE WHILE
%token <meno> 	ID
%token <txt>	STR_LITERAL
%token <immediate>   INT_LITERAL
%token  	LZZAT  PZZAT  LOZAT  POZAT  LHZAT  PHZAT
%token  	CIARKA  BCIARKA  BODKA  ROVNE ZVYS OTAZ DBOD
%token 		FOR DO SWITCH CASE BREAK SHORT LONG STATIC AUTO INLINE DEFAULT 

%start Program

/* priradenie priorit a asociativnosti operatorom */
%right        ROVNE
%right		OTAZ DBOD
%left		ORB
%left		ANDB XORB NOTB
%left		OR
%left		AND
%left		EQ NE
%left		LT LE GT GE
%left		PLUS MINUS
%left		KRAT DEL ZVYS
%right		UMINUS NOT
%left		BODKA
%right ELSE IF

/* priradenie prenasanych hodnot neterm. symbolom */
%type <type> VoidType CanonType UserType DataType
%type <node> Program
%type <expression> Vyraz Primarny
%type <statement> Function VarDef Prikaz
%type <argumentl> OptParameters Parameters
%type <memberl> Members 
%type <statementl> Statements Prikazy TeloMet
%type <expressionl> NepArgumenty Argumenty
%type <string> Ident

/*
%type <ihod>	NadTrieda
%type <sig>	NepParametre  Parametre  TypVoid  TypDat
%type <mod>	ModTriedy  ModClena  Mod1  Mod2
%type <vhod>	Vyraz  Primarny DCPrirad DCPristup NovyObjekt
%type <vhod>	MetVolanie NepArgumenty Argumenty
%type <kod>   Prikaz Prikazy
*/

%{
typedef unsigned int uint;

FILE *vyst;

AST::Program * program;

int yyparse(void), yylex(void);
void yyerror(const char *s);

//#include "lexical.cpp"
%}

%%
Program		: Statements
		{ program = new AST::Program($1); }
		;

Statements	: 
		{ $$ = new AST::StatementList(); }
		| Statements DataType BCIARKA	{ $$ = $1; $1->append(new AST::TypeDef($2)); }
		| Statements VarDef { $$ = $1; $1->append($2); }
		| Statements Function { $$ = $1; $1->append($2); }
		;

VarDef		: DataType Ident { $$ = new AST::VariableDefinition($1, $2, NULL); free($2); }
		| DataType Ident EQ Vyraz { $$ = new AST::VariableDefinition($1, $2, $4); free($2); }
		;
Function		: DataType  Ident  LOZAT  OptParameters  POZAT TeloMet { $$ = new AST::Function($1, $2, $4, $6); free($2); }
		;
OptParameters: /* e */ 		{ $$ = new AST::ArgumentList(); }
		| Parameters 		{ $$ = $1; }
		;
Parameters	: DataType  Ident
			{ 
				$$ = new AST::ArgumentList();
				$$->append(new CC::Variable($2, $1, NULL));
				free($2);
			}
		| Parameters  CIARKA  DataType  Ident
			{
				$$ = $1;
				$1->append(new CC::Variable($4, $3, NULL));
				free($4);
			}
		;
DataType: VoidType { $$ = $1; }
		|	CanonType { $$ = $1; }
		|	UserType { $$ = $1; }
		;
VoidType	: VOID { $$ = new CC::Void(); }
		;
CanonType		: INT { $$ = new CC::Integer(); }
		| CHAR { $$ = new CC::Char(); }
		;
UserType			: STRUCT Ident { $$ = new CC::IncompleteStruct($2); free($2); }
		| STRUCT Ident LZZAT Members PZZAT { $$ = new CC::Struct($2, $4->getVariables()); free($2); delete $4; }
		| UNION Ident LZZAT Members PZZAT { $$ = new CC::Union($2, $4->getVariables()); free($2); delete $4; }
		| UNION Ident { $$ = new CC::IncompleteUnion($2); free($2); }
		;
Members		: /* e */	{ $$ = new AST::MemberList(); }
		| DataType Ident BCIARKA { $$ = new AST::MemberList(); $$->append(new CC::Variable($2, $1, NULL)); free($2); }
		| Members DataType Ident { $1->append(new CC::Variable($3, $2, NULL)); $$ = $1; free($3); }
		;
TeloMet	: BCIARKA { $$ = new AST::StatementList(); }
		| LZZAT  Prikazy  PZZAT { $$ = $2; }
		| LZZAT PZZAT { $$ = new AST::StatementList(); }
		;
Prikazy 	: Prikaz {
				$$ = new AST::StatementList();
				$$->append($1);
			}
		| Prikazy Prikaz
			{
				$$ = $1;
				if ($1 != NULL) $1->append($2);
			}
		| Prikazy error
		;
Prikaz		: VarDef BCIARKA 						{ $$ = $1; }
		| Vyraz BCIARKA 							{ $$ = new AST::UnusedValue($1); }
		| RETURN BCIARKA 							{ $$ = new AST::Return(); }
		| BCIARKA 									{ $$ = NULL; }
		| RETURN Vyraz BCIARKA 						{ $$ = new AST::Return($2); }
		| WHILE LOZAT Vyraz POZAT Prikaz 			{ $$ = new AST::While($3, $5); }
		| IF LOZAT Vyraz POZAT Prikaz ELSE Prikaz 	{ $$ = new AST::If($3, $5, $7); }
		| IF LOZAT Vyraz POZAT Prikaz %prec ELSE	{ $$ = new AST::If($3, $5); }
		| LZZAT Prikazy PZZAT 						{ $$ = $2; }
		;
/*ZozIdent	: ID 			{ulozLP($1,$<sig>0);}		// TODO
		| ZozIdent CIARKA ID	{ulozLP($3,$<sig>0);} */
		;
Vyraz		: Primarny				{ $$ = $1; }
		| Vyraz PLUS Vyraz 			{ $$ = new AST::Add($1, $3); }
		| Vyraz MINUS Vyraz 		{ $$ = new AST::Sub($1, $3); }
		| Vyraz KRAT Vyraz 			{ $$ = new AST::Mul($1, $3); }
		| Vyraz DEL Vyraz 			{ $$ = new AST::Div($1, $3); }
		| Vyraz LT Vyraz 			{ $$ = new AST::Lt($1, $3); }
		| Vyraz LE Vyraz 			{ $$ = new AST::Lte($1, $3); }
		| Vyraz GT Vyraz 			{ $$ = new AST::Gt($1, $3); }
		| Vyraz GE Vyraz 			{ $$ = new AST::Gte($1, $3); }
		| Vyraz EQ Vyraz 			{ $$ = new AST::Equ($1, $3); }
		| Vyraz NE Vyraz 			{ $$ = new AST::Nequ($1, $3); }
		| Vyraz OR Vyraz 			{ $$ = new AST::Or($1, $3); }
		| Vyraz AND Vyraz 			{ $$ = new AST::And($1, $3); }
		| MINUS Vyraz %prec UMINUS 	{ yyerror("Vlastnost nie je podporovana: unarne minus");				/* TODO */	}
		| Vyraz ZVYS Vyraz			{ $$ = new AST::Modulo($1, $3); }
		| NOT Vyraz 				{ $$ = new AST::Neg($2); }
		| Vyraz OTAZ Vyraz DBOD Vyraz { $$ = new AST::Conditional($1, $3, $5);			/* TODO: nie je to IF! */ }
		;
Primarny	: Ident %prec ROVNE			{ $$ = new AST::Variable($1); free($1); }
		| Vyraz ROVNE Vyraz 				{ $$ = new AST::Assign($1, $3); }
		| INT_LITERAL 					{ $$ = new AST::Integer($1); }
//		| STR_LITERAL 					{ $$ = new AST::String($1); }
		| LOZAT  Vyraz  POZAT 			{ $$=$2;} 
//		| LOZAT INT POZAT Vyraz 		{ $$ = new AST::Integer($4); }
		| Ident  LOZAT  NepArgumenty POZAT { $$ = new AST::FunctionCall($1, $3); free($1); }
		;
/*DCPristup	: Vyraz BODKA ID
			{
				// TODO
			}
		; */
NepArgumenty    : /* e */			{ $$ = new AST::ExpressionList(); }
		| Argumenty 				{ $$ = $1; }
		;
Argumenty       : Vyraz
			{ 
				$$ = new AST::ExpressionList();
				$$->append($1);
			}
		| Argumenty  CIARKA  Vyraz
			{ 
				$$ = $1;
				$1->append($3);
			}
		;
Ident			: ID			{ $$ = strdup($1); }
		;
%%

int pocetChyb = 0;
int riadok;
extern char * yytext;

void yyerror(const char *s) {
	//extern int riadok;
	fprintf(stdout,"CHYBA: %s [%s:%d]\n", s, yytext, riadok);
	pocetChyb++;
}

void xxerror(char *s1,char *s2){
	//extern int riadok;
	fprintf(stdout,"CHYBA: %s %s [%d]\n", s1, s2, riadok);
	pocetChyb++;
}