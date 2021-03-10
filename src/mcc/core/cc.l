%{
	#include <nodes.h>
	#include "syntax.hpp"

//extern int yylval;
extern "C" int yywrap() { return 1; }
void yyerror(const char *s);
extern int riadok;

%}

%%
"if"      { return IF; }
"else"     { return ELSE; }
"for"      { return FOR; }
"while"     { return WHILE; }
"do"      { return DO; }
"switch"     { return SWITCH; }
"case"       { return CASE; }
"break"      { return BREAK; }
"int"        { return INT; }
"void"       { return VOID; }
"short"      { return SHORT; }
"long"       { return LONG; }
"char"       { return CHAR; }
"static"     { return STATIC; }
"auto"       { return AUTO; }
"inline"     { return INLINE; }
"default"    { return DEFAULT; }
"<"      { return LT; }
">"      { return GT; }
"<="     { return LE; }
">="     { return GE; }
"*"      { return KRAT; }
"."      { return BODKA; }
";"      { return BCIARKA; }
","      { return CIARKA; }
":"      { return DBOD; }
"["      { return LHZAT; }
"]"      { return PHZAT; }
"{"      { return LZZAT; }
"}"      { return PZZAT; }
"="      { return ROVNE; }
"=="     { return EQ; }
"!="     { return NE; }
"("      { return LOZAT; }
")"      { return POZAT; }
"&"      { return ANDB; }
"|"      { return ORB; }
"~"      { return NOTB; }
"^"      { return XORB; }
"?"      { return OTAZ; }

"&&"     { return AND; }
"||"     { return OR; }
"!"      { return NOT; }

"+"      { return PLUS; }
"-"      { return MINUS; }
"/"      { return DEL; }
"%"      { return ZVYS; }
"\n"     { riadok++; }
[ \t]+		

[0-9][0-9]*   { yylval.immediate = strtoul(yytext, NULL, 10); return INT_LITERAL; }
0[xX][0-9A-Fa-f]+ { yylval.immediate = strtoul(yytext, NULL, 16); return INT_LITERAL; }
[_a-zA-Z][_a-zA-Z0-9]*    { yylval.txt = yytext; return ID; }   
.        { yyerror("illegal token"); }
%%
