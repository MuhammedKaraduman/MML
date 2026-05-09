/*
 * mml.h — Mini Math Language
 * Shared type definitions, AST node structs, symbol/function tables.
 */

#ifndef MML_H
#define MML_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Limits ────────────────────────────────────────────────────────── */
#define MAX_IDENT   64
#define MAX_SYMBOLS 256
#define MAX_FUNCS   64
#define MAX_PARAMS  16
#define MAX_ARGS    16

/* ═══════════════════════════════════════════════════════════════════
 * ENUMS
 * ═══════════════════════════════════════════════════════════════════ */

/* Binary operators */
typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_POW
} BinOp;

/* Comparison operators (carried in yylval.cmp) */
typedef enum {
    CMP_EQ, CMP_NEQ, CMP_LT, CMP_LTE, CMP_GT, CMP_GTE
} CmpOp;

/* Built-in math functions */
typedef enum {
    FUNC_SIN, FUNC_COS, FUNC_TAN,
    FUNC_SQRT, FUNC_ABS,
    FUNC_LOG,  FUNC_EXP,
    FUNC_FLOOR, FUNC_CEIL
} BuiltinFunc;

/* ═══════════════════════════════════════════════════════════════════
 * EXPRESSION AST
 * ═══════════════════════════════════════════════════════════════════ */

typedef enum {
    EXPR_NUM,       /* numeric literal  → num */
    EXPR_IDENT,     /* variable ref     → str */
    EXPR_BINOP,     /* a OP b           → op, left, right */
    EXPR_UNOP,      /* -a               → left */
    EXPR_BUILTIN,   /* SIN(a)           → bfunc, left */
    EXPR_CALL       /* f(a, b, ...)     → str, args, argc */
} ExprKind;

typedef struct ExprNode ExprNode;

struct ExprNode {
    ExprKind kind;
    union {
        double          num;              /* EXPR_NUM */
        char           *str;              /* EXPR_IDENT, EXPR_CALL name */
    };
    BinOp       op;                       /* EXPR_BINOP */
    BuiltinFunc bfunc;                    /* EXPR_BUILTIN */
    ExprNode   *left;                     /* EXPR_BINOP left, EXPR_UNOP, EXPR_BUILTIN arg */
    ExprNode   *right;                    /* EXPR_BINOP right */
    ExprNode   *args[MAX_ARGS];           /* EXPR_CALL arguments */
    int         argc;                     /* EXPR_CALL argument count */
};

/* ═══════════════════════════════════════════════════════════════════
 * CONDITION AST
 * ═══════════════════════════════════════════════════════════════════ */

typedef enum {
    COND_CMP,   /* expr CMP expr */
    COND_AND,
    COND_OR,
    COND_NOT
} CondKind;

typedef struct CondNode CondNode;

struct CondNode {
    CondKind    kind;
    CmpOp       cmp;            /* COND_CMP */
    ExprNode   *left_expr;      /* COND_CMP */
    ExprNode   *right_expr;     /* COND_CMP */
    CondNode   *left;           /* COND_AND, COND_OR, COND_NOT */
    CondNode   *right;          /* COND_AND, COND_OR */
};

/* ═══════════════════════════════════════════════════════════════════
 * PRINT ARGUMENT LIST
 * ═══════════════════════════════════════════════════════════════════ */

typedef enum { PITEM_EXPR, PITEM_STR } PrintItemKind;

typedef struct PrintItem PrintItem;
struct PrintItem {
    PrintItemKind kind;
    ExprNode     *expr;   /* PITEM_EXPR */
    char         *str;    /* PITEM_STR */
    PrintItem    *next;
};

/* ═══════════════════════════════════════════════════════════════════
 * STATEMENT AST
 * ═══════════════════════════════════════════════════════════════════ */

typedef enum {
    STMT_SET,
    STMT_PRINT,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_DEF
} StmtKind;

typedef struct StmtNode StmtNode;

struct StmtNode {
    StmtKind    kind;
    StmtNode   *next;          /* linked list of statements */

    /* STMT_SET */
    char        name[MAX_IDENT];
    ExprNode   *expr;

    /* STMT_PRINT */
    PrintItem  *print_list;

    /* STMT_IF */
    CondNode   *cond;
    StmtNode   *then_body;
    StmtNode   *else_body;     /* NULL if no ELSE */

    /* STMT_WHILE */
    /* cond reused; body in then_body */

    /* STMT_FOR */
    ExprNode   *from_expr;
    ExprNode   *to_expr;
    ExprNode   *step_expr;     /* NULL → default +1 */

    /* STMT_DEF */
    char        params[MAX_PARAMS][MAX_IDENT];
    int         param_count;
    /* body in expr */
};

/* ═══════════════════════════════════════════════════════════════════
 * SYMBOL TABLE  (variables)
 * ═══════════════════════════════════════════════════════════════════ */

typedef struct {
    char   name[MAX_IDENT];
    double value;
} Symbol;

typedef struct {
    Symbol entries[MAX_SYMBOLS];
    int    count;
} SymbolTable;

/* ═══════════════════════════════════════════════════════════════════
 * FUNCTION TABLE  (user-defined)
 * ═══════════════════════════════════════════════════════════════════ */

typedef struct {
    char      name[MAX_IDENT];
    char      params[MAX_PARAMS][MAX_IDENT];
    int       param_count;
    ExprNode *body;
} UserFunc;

typedef struct {
    UserFunc entries[MAX_FUNCS];
    int      count;
} FuncTable;

/* ═══════════════════════════════════════════════════════════════════
 * ENVIRONMENT  (passed through executor)
 * ═══════════════════════════════════════════════════════════════════ */

typedef struct {
    SymbolTable symbols;
    FuncTable   funcs;
} Env;

/* ═══════════════════════════════════════════════════════════════════
 * FUNCTION PROTOTYPES
 * ═══════════════════════════════════════════════════════════════════ */

/* --- AST constructors (called from yacc actions) --- */
ExprNode  *make_expr_num   (double num);
ExprNode  *make_expr_ident (char *name);
ExprNode  *make_expr_binop (BinOp op, ExprNode *l, ExprNode *r);
ExprNode  *make_expr_unop  (ExprNode *operand);
ExprNode  *make_expr_builtin(BuiltinFunc f, ExprNode *arg);
ExprNode  *make_expr_call  (char *name, ExprNode **args, int argc);

CondNode  *make_cond_cmp   (CmpOp op, ExprNode *l, ExprNode *r);
CondNode  *make_cond_and   (CondNode *l, CondNode *r);
CondNode  *make_cond_or    (CondNode *l, CondNode *r);
CondNode  *make_cond_not   (CondNode *c);

StmtNode  *make_stmt_set   (char *name, ExprNode *expr);
StmtNode  *make_stmt_print (PrintItem *list);
StmtNode  *make_stmt_if    (CondNode *cond, StmtNode *then_b, StmtNode *else_b);
StmtNode  *make_stmt_while (CondNode *cond, StmtNode *body);
StmtNode  *make_stmt_for   (char *var, ExprNode *from, ExprNode *to,
                             ExprNode *step, StmtNode *body);
StmtNode  *make_stmt_def   (char *name, char params[][MAX_IDENT],
                             int param_count, ExprNode *body);
StmtNode  *stmt_append     (StmtNode *list, StmtNode *node);

PrintItem *make_pitem_expr (ExprNode *expr);
PrintItem *make_pitem_str  (char *str);
PrintItem *pitem_append    (PrintItem *list, PrintItem *item);

/* --- Executor --- */
void   exec_program (StmtNode *program);
void   exec_stmt    (Env *env, StmtNode *stmt);
double eval_expr    (Env *env, ExprNode *expr);
int    eval_cond    (Env *env, CondNode *cond);

/* --- Symbol table --- */
double sym_get (SymbolTable *t, const char *name);
void   sym_set (SymbolTable *t, const char *name, double value);

/* --- Function table --- */
UserFunc *func_get (FuncTable *t, const char *name);
void      func_set (FuncTable *t, const char *name,
                    char params[][MAX_IDENT], int pc, ExprNode *body);

/* --- Utility --- */
char *xstrdup (const char *s);
void  mml_error (const char *msg);

#endif /* MML_H */
