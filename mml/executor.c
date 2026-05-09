/*
 * executor.c — Mini Math Language Runtime
 *
 * Responsibilities:
 *   1. AST constructor functions (called from mml.y semantic actions)
 *   2. eval_expr  — recursively evaluates an ExprNode tree → double
 *   3. eval_cond  — recursively evaluates a CondNode tree  → int (0/1)
 *   4. exec_stmt  — executes a StmtNode (SET, PRINT, IF, WHILE, FOR, DEF)
 *   5. exec_program — walks the top-level statement list
 *   6. Symbol table and function table helpers
 */

#include "mml.h"

/* ═══════════════════════════════════════════════════════════════════
 * UTILITY
 * ═══════════════════════════════════════════════════════════════════ */

char *xstrdup(const char *s)
{
    char *copy = malloc(strlen(s) + 1);
    if (!copy) { perror("malloc"); exit(1); }
    strcpy(copy, s);
    return copy;
}

void mml_error(const char *msg)
{
    fprintf(stderr, "[RUNTIME ERROR] %s\n", msg);
    exit(1);
}

/* ═══════════════════════════════════════════════════════════════════
 * SYMBOL TABLE
 * ═══════════════════════════════════════════════════════════════════ */

/* Returns the value of a variable; exits with error if not found. */
double sym_get(SymbolTable *t, const char *name)
{
    for (int i = 0; i < t->count; i++) {
        if (strcmp(t->entries[i].name, name) == 0)
            return t->entries[i].value;
    }
    char msg[128];
    snprintf(msg, sizeof(msg), "Undefined variable: '%s'", name);
    mml_error(msg);
    return 0.0;
}

/* Sets (or creates) a variable in the symbol table. */
void sym_set(SymbolTable *t, const char *name, double value)
{
    for (int i = 0; i < t->count; i++) {
        if (strcmp(t->entries[i].name, name) == 0) {
            t->entries[i].value = value;
            return;
        }
    }
    if (t->count >= MAX_SYMBOLS)
        mml_error("Symbol table full");
    strncpy(t->entries[t->count].name, name, MAX_IDENT - 1);
    t->entries[t->count].value = value;
    t->count++;
}

/* ═══════════════════════════════════════════════════════════════════
 * FUNCTION TABLE
 * ═══════════════════════════════════════════════════════════════════ */

UserFunc *func_get(FuncTable *t, const char *name)
{
    for (int i = 0; i < t->count; i++) {
        if (strcmp(t->entries[i].name, name) == 0)
            return &t->entries[i];
    }
    return NULL;
}

void func_set(FuncTable *t, const char *name,
              char params[][MAX_IDENT], int pc, ExprNode *body)
{
    /* Overwrite if already defined */
    for (int i = 0; i < t->count; i++) {
        if (strcmp(t->entries[i].name, name) == 0) {
            memcpy(t->entries[i].params, params,
                   pc * MAX_IDENT);
            t->entries[i].param_count = pc;
            t->entries[i].body = body;
            return;
        }
    }
    if (t->count >= MAX_FUNCS)
        mml_error("Function table full");
    strncpy(t->entries[t->count].name, name, MAX_IDENT - 1);
    memcpy(t->entries[t->count].params, params, pc * MAX_IDENT);
    t->entries[t->count].param_count = pc;
    t->entries[t->count].body = body;
    t->count++;
}

/* ═══════════════════════════════════════════════════════════════════
 * AST CONSTRUCTORS — ExprNode
 * ═══════════════════════════════════════════════════════════════════ */

static ExprNode *new_expr(ExprKind kind)
{
    ExprNode *n = calloc(1, sizeof(ExprNode));
    if (!n) { perror("calloc"); exit(1); }
    n->kind = kind;
    return n;
}

ExprNode *make_expr_num(double num)
{
    ExprNode *n = new_expr(EXPR_NUM);
    n->num = num;
    return n;
}

ExprNode *make_expr_ident(char *name)
{
    ExprNode *n = new_expr(EXPR_IDENT);
    n->str = name;   /* already xstrdup'd by the lexer */
    return n;
}

ExprNode *make_expr_binop(BinOp op, ExprNode *l, ExprNode *r)
{
    ExprNode *n = new_expr(EXPR_BINOP);
    n->op    = op;
    n->left  = l;
    n->right = r;
    return n;
}

ExprNode *make_expr_unop(ExprNode *operand)
{
    ExprNode *n = new_expr(EXPR_UNOP);
    n->left = operand;
    return n;
}

ExprNode *make_expr_builtin(BuiltinFunc f, ExprNode *arg)
{
    ExprNode *n = new_expr(EXPR_BUILTIN);
    n->bfunc = f;
    n->left  = arg;
    return n;
}

ExprNode *make_expr_call(char *name, ExprNode **args, int argc)
{
    ExprNode *n = new_expr(EXPR_CALL);
    n->str  = name;  /* already xstrdup'd by the lexer */
    n->argc = argc;
    for (int i = 0; i < argc; i++)
        n->args[i] = args[i];
    return n;
}

/* ═══════════════════════════════════════════════════════════════════
 * AST CONSTRUCTORS — CondNode
 * ═══════════════════════════════════════════════════════════════════ */

static CondNode *new_cond(CondKind kind)
{
    CondNode *n = calloc(1, sizeof(CondNode));
    if (!n) { perror("calloc"); exit(1); }
    n->kind = kind;
    return n;
}

CondNode *make_cond_cmp(CmpOp op, ExprNode *l, ExprNode *r)
{
    CondNode *n = new_cond(COND_CMP);
    n->cmp        = op;
    n->left_expr  = l;
    n->right_expr = r;
    return n;
}

CondNode *make_cond_and(CondNode *l, CondNode *r)
{
    CondNode *n = new_cond(COND_AND);
    n->left  = l;
    n->right = r;
    return n;
}

CondNode *make_cond_or(CondNode *l, CondNode *r)
{
    CondNode *n = new_cond(COND_OR);
    n->left  = l;
    n->right = r;
    return n;
}

CondNode *make_cond_not(CondNode *c)
{
    CondNode *n = new_cond(COND_NOT);
    n->left = c;
    return n;
}

/* ═══════════════════════════════════════════════════════════════════
 * AST CONSTRUCTORS — StmtNode
 * ═══════════════════════════════════════════════════════════════════ */

static StmtNode *new_stmt(StmtKind kind)
{
    StmtNode *n = calloc(1, sizeof(StmtNode));
    if (!n) { perror("calloc"); exit(1); }
    n->kind = kind;
    n->next = NULL;
    return n;
}

StmtNode *make_stmt_set(char *name, ExprNode *expr)
{
    StmtNode *n = new_stmt(STMT_SET);
    strncpy(n->name, name, MAX_IDENT - 1);
    n->expr = expr;
    free(name);
    return n;
}

StmtNode *make_stmt_print(PrintItem *list)
{
    StmtNode *n = new_stmt(STMT_PRINT);
    n->print_list = list;
    return n;
}

StmtNode *make_stmt_if(CondNode *cond, StmtNode *then_b, StmtNode *else_b)
{
    StmtNode *n = new_stmt(STMT_IF);
    n->cond      = cond;
    n->then_body = then_b;
    n->else_body = else_b;
    return n;
}

StmtNode *make_stmt_while(CondNode *cond, StmtNode *body)
{
    StmtNode *n = new_stmt(STMT_WHILE);
    n->cond      = cond;
    n->then_body = body;
    return n;
}

StmtNode *make_stmt_for(char *var, ExprNode *from, ExprNode *to,
                         ExprNode *step, StmtNode *body)
{
    StmtNode *n = new_stmt(STMT_FOR);
    strncpy(n->name, var, MAX_IDENT - 1);
    n->from_expr = from;
    n->to_expr   = to;
    n->step_expr = step;   /* NULL → default +1 */
    n->then_body = body;
    free(var);
    return n;
}

StmtNode *make_stmt_def(char *name, char params[][MAX_IDENT],
                         int param_count, ExprNode *body)
{
    StmtNode *n = new_stmt(STMT_DEF);
    strncpy(n->name, name, MAX_IDENT - 1);
    n->param_count = param_count;
    memcpy(n->params, params, param_count * MAX_IDENT);
    n->expr = body;
    free(name);
    return n;
}

/* Append node to the end of a linked statement list. */
StmtNode *stmt_append(StmtNode *list, StmtNode *node)
{
    StmtNode *cur = list;
    while (cur->next) cur = cur->next;
    cur->next = node;
    return list;
}

/* ═══════════════════════════════════════════════════════════════════
 * AST CONSTRUCTORS — PrintItem
 * ═══════════════════════════════════════════════════════════════════ */

PrintItem *make_pitem_expr(ExprNode *expr)
{
    PrintItem *p = calloc(1, sizeof(PrintItem));
    p->kind = PITEM_EXPR;
    p->expr = expr;
    p->next = NULL;
    return p;
}

PrintItem *make_pitem_str(char *str)
{
    PrintItem *p = calloc(1, sizeof(PrintItem));
    p->kind = PITEM_STR;
    p->str  = str;
    p->next = NULL;
    return p;
}

PrintItem *pitem_append(PrintItem *list, PrintItem *item)
{
    PrintItem *cur = list;
    while (cur->next) cur = cur->next;
    cur->next = item;
    return list;
}

/* ═══════════════════════════════════════════════════════════════════
 * EXPRESSION EVALUATOR
 * ═══════════════════════════════════════════════════════════════════ */

double eval_expr(Env *env, ExprNode *e)
{
    if (!e) mml_error("NULL expression node");

    switch (e->kind) {

    case EXPR_NUM:
        return e->num;

    case EXPR_IDENT:
        return sym_get(&env->symbols, e->str);

    case EXPR_UNOP:
        return -eval_expr(env, e->left);

    case EXPR_BINOP: {
        double l = eval_expr(env, e->left);
        double r = eval_expr(env, e->right);
        switch (e->op) {
            case OP_ADD: return l + r;
            case OP_SUB: return l - r;
            case OP_MUL: return l * r;
            case OP_DIV:
                if (r == 0.0) mml_error("Division by zero");
                return l / r;
            case OP_MOD:
                if (r == 0.0) mml_error("Modulo by zero");
                return fmod(l, r);
            case OP_POW: return pow(l, r);
        }
        break;
    }

    case EXPR_BUILTIN: {
        double v = eval_expr(env, e->left);
        switch (e->bfunc) {
            case FUNC_SIN:   return sin(v);
            case FUNC_COS:   return cos(v);
            case FUNC_TAN:   return tan(v);
            case FUNC_SQRT:
                if (v < 0) mml_error("SQRT of negative number");
                return sqrt(v);
            case FUNC_ABS:   return fabs(v);
            case FUNC_LOG:
                if (v <= 0) mml_error("LOG of non-positive number");
                return log(v);
            case FUNC_EXP:   return exp(v);
            case FUNC_FLOOR: return floor(v);
            case FUNC_CEIL:  return ceil(v);
        }
        break;
    }

    case EXPR_CALL: {
        /* Look up user-defined function */
        UserFunc *f = func_get(&env->funcs, e->str);
        if (!f) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "Undefined function: '%s'", e->str);
            mml_error(msg);
        }
        if (e->argc != f->param_count) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "Function '%s' expects %d args, got %d",
                     e->str, f->param_count, e->argc);
            mml_error(msg);
        }

        /* Create a local environment for the call (copy globals,
           then bind parameters) */
        Env local = *env;
        for (int i = 0; i < f->param_count; i++) {
            double val = eval_expr(env, e->args[i]);
            sym_set(&local.symbols, f->params[i], val);
        }
        return eval_expr(&local, f->body);
    }

    } /* switch */

    mml_error("Unknown expression kind");
    return 0.0;
}

/* ═══════════════════════════════════════════════════════════════════
 * CONDITION EVALUATOR
 * ═══════════════════════════════════════════════════════════════════ */

int eval_cond(Env *env, CondNode *c)
{
    if (!c) mml_error("NULL condition node");

    switch (c->kind) {
    case COND_CMP: {
        double l = eval_expr(env, c->left_expr);
        double r = eval_expr(env, c->right_expr);
        switch (c->cmp) {
            case CMP_EQ:  return l == r;
            case CMP_NEQ: return l != r;
            case CMP_LT:  return l <  r;
            case CMP_LTE: return l <= r;
            case CMP_GT:  return l >  r;
            case CMP_GTE: return l >= r;
        }
        break;
    }
    case COND_AND: return eval_cond(env, c->left) && eval_cond(env, c->right);
    case COND_OR:  return eval_cond(env, c->left) || eval_cond(env, c->right);
    case COND_NOT: return !eval_cond(env, c->left);
    }

    mml_error("Unknown condition kind");
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * STATEMENT EXECUTOR
 * ═══════════════════════════════════════════════════════════════════ */

/* Helper: print a single double — integers without decimal point */
static void print_num(double v)
{
    if (v == (long long)v)
        printf("%lld", (long long)v);
    else
        printf("%g", v);
}

void exec_stmt(Env *env, StmtNode *s)
{
    if (!s) return;

    switch (s->kind) {

    /* ── SET x = expr ── */
    case STMT_SET: {
        double val = eval_expr(env, s->expr);
        sym_set(&env->symbols, s->name, val);
        break;
    }

    /* ── PRINT args ── */
    case STMT_PRINT: {
        PrintItem *item = s->print_list;
        int first = 1;
        while (item) {
            if (!first) printf(" ");
            first = 0;
            if (item->kind == PITEM_STR)
                printf("%s", item->str);
            else
                print_num(eval_expr(env, item->expr));
            item = item->next;
        }
        printf("\n");
        break;
    }

    /* ── IF cond THEN body [ELSE body] END ── */
    case STMT_IF: {
        if (eval_cond(env, s->cond))
            exec_stmt(env, s->then_body);
        else if (s->else_body)
            exec_stmt(env, s->else_body);
        break;
    }

    /* ── WHILE cond DO body END ── */
    case STMT_WHILE: {
        while (eval_cond(env, s->cond))
            exec_stmt(env, s->then_body);
        break;
    }

    /* ── FOR var FROM expr TO expr [STEP expr] DO body END ── */
    case STMT_FOR: {
        double from = eval_expr(env, s->from_expr);
        double to   = eval_expr(env, s->to_expr);
        double step = s->step_expr ? eval_expr(env, s->step_expr) : 1.0;

        if (step == 0.0) mml_error("FOR step cannot be zero");

        /* Guard against infinite loops */
        if (step > 0) {
            for (double i = from; i <= to; i += step) {
                sym_set(&env->symbols, s->name, i);
                exec_stmt(env, s->then_body);
            }
        } else {
            for (double i = from; i >= to; i += step) {
                sym_set(&env->symbols, s->name, i);
                exec_stmt(env, s->then_body);
            }
        }
        break;
    }

    /* ── DEF name(params) = expr ── */
    case STMT_DEF: {
        func_set(&env->funcs, s->name,
                 s->params, s->param_count, s->expr);
        printf("[DEF] Function '%s' defined with %d param(s)\n",
               s->name, s->param_count);
        break;
    }

    } /* switch */

    /* Walk the linked list of statements */
    if (s->next)
        exec_stmt(env, s->next);
}

/* ═══════════════════════════════════════════════════════════════════
 * PROGRAM ENTRY POINT
 * Called once by mml.y after a successful parse.
 * ═══════════════════════════════════════════════════════════════════ */

void exec_program(StmtNode *program)
{
    Env env;
    memset(&env, 0, sizeof(env));
    exec_stmt(&env, program);
}
