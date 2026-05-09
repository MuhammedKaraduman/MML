/*
 * mml.l -- Mini Math Language Lexer
 * Flex specification: converts character stream into tokens for mml.y
 */

%option case-insensitive
%option noyywrap
%option nounput
%option noinput
%option yylineno

%{
#include "mml.h"
#include "y.tab.h"
%}

/* Character class definitions */
ALPHA     [A-Za-z_]
ALNUM     [A-Za-z0-9_]
DIGIT     [0-9]
INTEGER   {DIGIT}+
FLOAT     {DIGIT}+"."{DIGIT}*|"."{DIGIT}+
NUMBER    {FLOAT}|{INTEGER}

%%

[ \t\r\n]+          { /* skip whitespace */ }
"//"[^\n]*          { /* skip line comments */ }

"SET"               { return SET; }
"PRINT"             { return PRINT; }
"IF"                { return IF; }
"THEN"              { return THEN; }
"ELSE"              { return ELSE; }
"END"               { return END; }
"WHILE"             { return WHILE; }
"DO"                { return DO; }
"FOR"               { return FOR; }
"FROM"              { return FROM; }
"TO"                { return TO; }
"STEP"              { return STEP; }
"DEF"               { return DEF; }
"AND"               { return AND; }
"OR"                { return OR; }
"NOT"               { return NOT; }

"SIN"               { yylval.bfunc = FUNC_SIN;   return BUILTIN; }
"COS"               { yylval.bfunc = FUNC_COS;   return BUILTIN; }
"TAN"               { yylval.bfunc = FUNC_TAN;   return BUILTIN; }
"SQRT"              { yylval.bfunc = FUNC_SQRT;  return BUILTIN; }
"ABS"               { yylval.bfunc = FUNC_ABS;   return BUILTIN; }
"LOG"               { yylval.bfunc = FUNC_LOG;   return BUILTIN; }
"EXP"               { yylval.bfunc = FUNC_EXP;   return BUILTIN; }
"FLOOR"             { yylval.bfunc = FUNC_FLOOR; return BUILTIN; }
"CEIL"              { yylval.bfunc = FUNC_CEIL;  return BUILTIN; }

"=="                { yylval.cmp = CMP_EQ;  return CMP; }
"!="                { yylval.cmp = CMP_NEQ; return CMP; }
"<="                { yylval.cmp = CMP_LTE; return CMP; }
">="                { yylval.cmp = CMP_GTE; return CMP; }
"<"                 { yylval.cmp = CMP_LT;  return CMP; }
">"                 { yylval.cmp = CMP_GT;  return CMP; }

"+"                 { return '+'; }
"-"                 { return '-'; }
"*"                 { return '*'; }
"/"                 { return '/'; }
"%"                 { return '%'; }
"^"                 { return '^'; }

"="                 { return '='; }
"("                 { return '('; }
")"                 { return ')'; }
","                 { return ','; }

{NUMBER}            {
                        yylval.num = atof(yytext);
                        return NUMBER;
                    }

\"[^\"]*\"          {
                        int len = strlen(yytext) - 2;
                        char *s = (char *)malloc(len + 1);
                        strncpy(s, yytext + 1, len);
                        s[len] = '\0';
                        yylval.str = s;
                        return STRING;
                    }

{ALPHA}{ALNUM}*     {
                        yylval.str = xstrdup(yytext);
                        return IDENT;
                    }

.                   {
                        fprintf(stderr,
                            "[LEX ERROR] Unknown character: '%s'\n", yytext);
                    }

%%
