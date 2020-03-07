/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    PEER = 258,
    GROUP = 259,
    IVAL = 260,
    RVAL = 261,
    NAME = 262,
    XSTRING = 263,
    SCOPE = 264,
    COLON = 265,
    LBRACE = 266,
    RBRACE = 267,
    TRUEBVAL = 268,
    FALSEBVAL = 269,
    CHAR = 270,
    WORD = 271,
    IP_ADDRESS = 272
  };
#endif
/* Tokens.  */
#define PEER 258
#define GROUP 259
#define IVAL 260
#define RVAL 261
#define NAME 262
#define XSTRING 263
#define SCOPE 264
#define COLON 265
#define LBRACE 266
#define RBRACE 267
#define TRUEBVAL 268
#define FALSEBVAL 269
#define CHAR 270
#define WORD 271
#define IP_ADDRESS 272

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 657 "configfile.y" /* yacc.c:1909  */

    scope *scp ;
    value *val ;
    char *name ;
    int integer ;
    double real ;
    char *string ;
    char chr ;

#line 98 "y.tab.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
