// SPDX-License-Identifier: MIT

#include "parser.h"

#include <glib.h>
#include <setjmp.h>
#include <stdio.h>

#include "scanner.h"

/*
 * The grammar, in the notation bison uses:
 *
 *   program
 *       : %empty
 *       | program statement
 *       ;
 *
 *   statement
 *       : expr ';'
 *       ;
 *
 *   expr
 *       : term
 *       | expr '+' term
 *       | expr '-' term
 *       ;
 *
 *   term
 *       : factor
 *       | term '*' factor
 *       | term '/' factor
 *       ;
 *
 *   factor
 *       : NUM
 *       | '(' expr ')'
 *       ;
 *
 * One function per nonterminal. Each returns the value of what it read.
 */

/* The next token: read, but not yet used. */
static int lookahead;

/* Where syntax_error() jumps back to: inside pb_parse(). */
static jmp_buf on_error;

static double parse_expr (void);

static void
advance (void)
{
    lookahead = pb_scanner_next ();
}

static G_NORETURN void
syntax_error (void)
{
    fflush (stdout);
    g_printerr ("pebble: syntax error\n");
    longjmp (on_error, 1);
}

/* The lookahead must be of this kind: use it up. */
static void
expect (int kind)
{
    if (lookahead != kind)
        syntax_error ();
    advance ();
}

/* factor : NUM | '(' expr ')' */
static double
parse_factor (void)
{
    double value;

    switch (lookahead) {
        case TOK_NUM:
            value = pb_scanner_value;
            advance ();
            return value;
        case '(':
            advance ();
            value = parse_expr ();
            expect (')');
            return value;
        default:
            syntax_error ();
    }
}

/* term : factor | term '*' factor | term '/' factor */
static double
parse_term (void)
{
    double value = parse_factor ();

    while (lookahead == '*' || lookahead == '/') {
        int op = lookahead;
        double right;

        advance ();
        right = parse_factor ();
        value = op == '*' ? value * right : value / right;
    }
    return value;
}

/* expr : term | expr '+' term | expr '-' term */
static double
parse_expr (void)
{
    double value = parse_term ();

    while (lookahead == '+' || lookahead == '-') {
        int op = lookahead;
        double right;

        advance ();
        right = parse_term ();
        value = op == '+' ? value + right : value - right;
    }
    return value;
}

/* statement : expr ';' */
static void
parse_statement (void)
{
    char text[G_ASCII_DTOSTR_BUF_SIZE];
    double value = parse_expr ();

    expect (';');
    printf ("%s\n", g_ascii_formatd (text, sizeof text, "%.10g", value));
}

/* program : %empty | program statement */
int
pb_parse (void)
{
    if (setjmp (on_error) != 0)
        return 1;

    advance ();
    while (lookahead != TOK_EOF)
        parse_statement ();
    return 0;
}
