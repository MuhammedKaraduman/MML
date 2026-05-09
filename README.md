# MML — Mini Math Language

A domain-specific scripting language for mathematical computation.
Implemented with Lex (Flex) + Yacc (Bison) + C.

## Build & Run

### Docker (recommended)
```bash
docker build -t mml .
docker run --rm mml                              # run all 5 tests
docker run --rm mml sh -c "./mml samples/test1.mml"
docker run --rm -it mml ./mml                   # interactive REPL
```

### Manual (Linux / WSL)
```bash
cd mml
make        # build
make test   # run all samples
make clean  # remove generated files
```

## Language Features

| Statement | Syntax |
|-----------|--------|
| Variable  | `SET x = expr` |
| Print     | `PRINT expr [, expr ...]` |
| If/Else   | `IF cond THEN ... [ELSE ...] END` |
| While     | `WHILE cond DO ... END` |
| For       | `FOR i FROM expr TO expr [STEP expr] DO ... END` |
| Function  | `DEF f(a, b) = expr` |

Built-ins: `SIN` `COS` `TAN` `SQRT` `ABS` `LOG` `EXP` `FLOOR` `CEIL`

Operators: `+` `-` `*` `/` `%` `^` `==` `!=` `<` `<=` `>` `>=` `AND` `OR` `NOT`

## Sample Programs

| File | Content |
|------|---------|
| test1.mml | Arithmetic & variables |
| test2.mml | IF/ELSE & boolean conditions |
| test3.mml | WHILE loops (Fibonacci, Collatz) |
| test4.mml | FOR loops & built-in math functions |
| test5.mml | User-defined functions (geometry, physics) |
