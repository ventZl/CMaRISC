/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
   2009, 2010 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INT = 258,
     CHAR = 259,
     VOID = 260,
     STRUCT = 261,
     UNION = 262,
     RETURN = 263,
     IF = 264,
     ELSE = 265,
     WHILE = 266,
     ID = 267,
     STR_LITERAL = 268,
     INT_LITERAL = 269,
     LZZAT = 270,
     PZZAT = 271,
     LOZAT = 272,
     POZAT = 273,
     LHZAT = 274,
     PHZAT = 275,
     CIARKA = 276,
     BCIARKA = 277,
     BODKA = 278,
     ROVNE = 279,
     ZVYS = 280,
     OTAZ = 281,
     DBOD = 282,
     OR = 283,
     AND = 284,
     NE = 285,
     EQ = 286,
     GE = 287,
     GT = 288,
     LE = 289,
     LT = 290,
     MINUS = 291,
     PLUS = 292,
     DEL = 293,
     KRAT = 294,
     NOT = 295,
     UMINUS = 296
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1685 of yacc.c  */
#line 8 "cc.y"

	Node * node;
	StatementList * statements;
	VariableList * variables;
	ExpressionList * expressions;
	Variable * variable;
	Expression * expression;
	Statement * statement;
	char* meno;
	char* txt;
	int ihod;



/* Line 1685 of yacc.c  */
#line 107 "cc.hpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


