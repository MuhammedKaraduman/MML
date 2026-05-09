/*
 * mml.y — Mini Math Language Parser
 * Bison/Yacc grammar with semantic actions that build an AST.
 * The AST is evaluated by the executor after each top-level parse.
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mml.h"

/* Provided by flex-generated lex.yy.c */
extern int  yylex(void);
extern int  yylineno;
void        yyerror(const char *msg);

/* Temporary arg buffer used while parsing CALL argument lists */
static ExprNode *arg_buf[MAX_ARGS];
static int       arg_count;

/* Temporary param buffer used while parsing DEF parameter lists */
static char param_buf[MAX_PARAMS][MAX_IDENT];
static int  param_count;
%}

/* ── Value types carried by grammar symbols ───────────────────────── */
%union {
    double       num;       /* NUMBER token */
    char        *str;       /* IDENT / STRING token */
    int          cmp;       /* CmpOp enum  (CMP token) */
    int          bfunc;     /* BuiltinFunc enum (BUILTIN token) */
    ExprNode    *expr;      /* expression node */
    CondNode    *cond;      /* condition node */
    StmtNode    *stmt;      /* statement / statement list */
    PrintItem   *pitem;     /* print argument list */
}

/* ── Token declarations ───────────────────────────────────────────── */
%token SET PRINT IF THEN ELSE END WHILE DO FOR FROM TO STEP DEF
%token AND OR NOT
%token <num>   NUMBER
%token <str>   IDENT STRING
%token <cmp>   CMP
%token <bfunc> BUILTIN

/* ── Operator precedence (lowest → highest) ──────────────────────── */
%left  OR
%left  AND
%right NOT
%nonassoc CMP
%left  '+' '-'
%left  '*' '/' '%'
%right '^'
%right UMINUS   /* pseudo-token for unary minus */

/* ── Non-terminal types ───────────────────────────────────────────── */
%type <expr>  expr
%type <cond>  cond
%type <stmt>  stmt stmt_list
%type <pitem> print_args print_item

%%

/* ════════════════════════════════════════════════════════════════════
 * Top-level program: one or more statements
 * ════════════════════════════════════════════════════════════════════ */
program
    : stmt_list
        { exec_program($1); }
    | /* empty */
    ;

/* ════════════════════════════════════════════════════════════════════
 * Statement list (left-recursive → efficient LALR parse)
 * ════════════════════════════════════════════════════════════════════ */
stmt_list
    : stmt
        { $$ = $1; }
    | stmt_list stmt
        { $$ = stmt_append($1, $2); }
    ;

/* ════════════════════════════════════════════════════════════════════
 * Statements
 * ════════════════════════════════════════════════════════════════════ */
stmt
    /* ── SET x = expr ── */
    : SET IDENT '=' expr
        { $$ = make_stmt_set($2, $4); }

    /* ── PRINT arg [, arg ...] ── */
    | PRINT print_args
        { $$ = make_stmt_print($2); }

    /* ── IF cond THEN body END ── */
    | IF cond THEN stmt_list END
        { $$ = make_stmt_if($2, $4, NULL); }

    /* ── IF cond THEN body ELSE body END ── */
    | IF cond THEN stmt_list ELSE stmt_list END
        { $$ = make_stmt_if($2, $4, $6); }

    /* ── WHILE cond DO body END ── */
    | WHILE cond DO stmt_list END
        { $$ = make_stmt_while($2, $4); }

    /* ── FOR var FROM expr TO expr DO body END ── */
    | FOR IDENT FROM expr TO expr DO stmt_list END
        { $$ = make_stmt_for($2, $4, $6, NULL, $8); }

    /* ── FOR var FROM expr TO expr STEP expr DO body END ── */
    | FOR IDENT FROM expr TO expr STEP expr DO stmt_list END
        { $$ = make_stmt_for($2, $4, $6, $8, $10); }

    /* ── DEF name(params) = expr ── */
    | DEF IDENT '(' param_list ')' '=' expr
        { $$ = make_stmt_def($2, param_buf, param_count, $7); }
    ;

/* ════════════════════════════════════════════════════════════════════
 * DEF parameter list (populates param_buf / param_count globals)
 * ════════════════════════════════════════════════════════════════════ */
param_list
    : IDENT
        {
            param_count = 0;
            strncpy(param_buf[param_count++], $1, MAX_IDENT - 1);
            free($1);
        }
    | param_list ',' IDENT
        {
            if (param_count < MAX_PARAMS) {
                strncpy(param_buf[param_count++], $3, MAX_IDENT - 1);
            }
            free($3);
        }
    ;

/* ════════════════════════════════════════════════════════════════════
 * PRINT argument list
 * ════════════════════════════════════════════════════════════════════ */
print_args
    : print_item
        { $$ = $1; }
    | print_args ',' print_item
        { $$ = pitem_append($1, $3); }
    ;

print_item
    : expr
        { $$ = make_pitem_expr($1); }
    | STRING
        { $$ = make_pitem_str($1); }
    ;

/* ════════════════════════════════════════════════════════════════════
 * Boolean conditions
 * ════════════════════════════════════════════════════════════════════ */
cond
    : expr CMP expr
        { $$ = make_cond_cmp((CmpOp)$2, $1, $3); }
    | cond AND cond
        { $$ = make_cond_and($1, $3); }
    | cond OR cond
        { $$ = make_cond_or($1, $3); }
    | NOT cond
        { $$ = make_cond_not($2); }
    | '(' cond ')'
        { $$ = $2; }
    ;

/* ════════════════════════════════════════════════════════════════════
 * Expressions
 * Precedence is handled by %left/%right declarations above,
 * not by grammar stratification — keeps rules concise.
 * ════════════════════════════════════════════════════════════════════ */
expr
    : NUMBER
        { $$ = make_expr_num($1); }

    | IDENT
        { $$ = make_expr_ident($1); }

    /* ── Arithmetic binary operators ── */
    | expr '+' expr
        { $$ = make_expr_binop(OP_ADD, $1, $3); }
    | expr '-' expr
        { $$ = make_expr_binop(OP_SUB, $1, $3); }
    | expr '*' expr
        { $$ = make_expr_binop(OP_MUL, $1, $3); }
    | expr '/' expr
        { $$ = make_expr_binop(OP_DIV, $1, $3); }
    | expr '%' expr
        { $$ = make_expr_binop(OP_MOD, $1, $3); }
    | expr '^' expr
        { $$ = make_expr_binop(OP_POW, $1, $3); }

    /* ── Unary minus ── */
    | '-' expr %prec UMINUS
        { $$ = make_expr_unop($2); }

    /* ── Parenthesised expression ── */
    | '(' expr ')'
        { $$ = $2; }

    /* ── Built-in function: SIN(x), SQRT(x), … ── */
    | BUILTIN '(' expr ')'
        { $$ = make_expr_builtin((BuiltinFunc)$1, $3); }

    /* ── User-defined function call: f(a, b, …) ──
         Uses global arg_buf / arg_count filled by call_args. */
    | IDENT '(' call_args ')'
        {
            $$ = make_expr_call($1, arg_buf, arg_count);
        }
    ;

/* ── Call argument list (fills arg_buf / arg_count) ──────────────── */
call_args
    : expr
        {
            arg_count = 0;
            arg_buf[arg_count++] = $1;
        }
    | call_args ',' expr
        {
            if (arg_count < MAX_ARGS)
                arg_buf[arg_count++] = $3;
        }
    ;

%%

/* ════════════════════════════════════════════════════════════════════
 * yyerror — called by Bison on a parse error
 * ════════════════════════════════════════════════════════════════════ */
void yyerror(const char *msg)
{
    fprintf(stderr, "[PARSE ERROR] line %d: %s\n", yylineno, msg);
}

/* ════════════════════════════════════════════════════════════════════
 * main — entry point; reads from stdin or a file argument
 * ════════════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[])
{
    extern FILE *yyin;

    printf("MML -- Mini Math Language\n");
    printf("--------------------------------------------------\n");

    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror(argv[1]);
            return 1;
        }
    }

    yyparse();

    if (argc > 1 && yyin)
        fclose(yyin);

    return 0;
}
