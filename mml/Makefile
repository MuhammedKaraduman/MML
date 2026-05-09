# Makefile — Mini Math Language
# Usage:  make        → build mml binary
#         make test   → build and run all sample programs
#         make clean  → remove generated files

CC      = gcc
CFLAGS  = -Wall -Wextra -g
LIBS    = -lm

# Generated source files
PARSER  = y.tab.c
LEXER   = lex.yy.c
HEADER  = y.tab.h

TARGET  = mml
SRCS    = $(PARSER) $(LEXER) executor.c

SAMPLES = samples/test1.mml \
          samples/test2.mml \
          samples/test3.mml \
          samples/test4.mml \
          samples/test5.mml

# ── Default target ────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(SRCS) mml.h
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

# Step 1: generate C parser + token header from Yacc grammar
$(PARSER) $(HEADER): mml.y
	yacc -d mml.y

# Step 2: generate C lexer from Lex specification
$(LEXER): mml.l $(HEADER)
	lex mml.l

# ── Test target: run all five sample programs ─────────────────────
test: $(TARGET)
	@for f in $(SAMPLES); do \
		echo ""; \
		echo "══════════════════════════════════════════"; \
		echo " Running: $$f"; \
		echo "══════════════════════════════════════════"; \
		./$(TARGET) $$f; \
	done

# ── Clean target ─────────────────────────────────────────────────
clean:
	rm -f $(PARSER) $(HEADER) $(LEXER) $(TARGET)

.PHONY: all test clean
