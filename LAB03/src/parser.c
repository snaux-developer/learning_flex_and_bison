// SPDX-License-Identifier: MIT

#include "parser.h"

#include <math.h>
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
 *       : unary
 *       | term '*' unary
 *       | term '/' unary
 *       ;
 *
 *   unary
 *       : power
 *       | '-' unary
 *       ;
 *
 *   power
 *       : factor
 *       | factor '^' unary
 *       ;
 *
 *   factor
 *       : NUM
 *       | '(' expr ')'
 *       ;
 *
 * One function per nonterminal. Each returns the value of what it read.
 */

gboolean pb_parser_trace = FALSE;

/* The next token: read, but not yet used. */
static int lookahead;

/* Where syntax_error() jumps back to: inside pb_parse(). */
static jmp_buf on_error;

/* How many rule functions are active, for --trace. */
static int depth;

static double parse_expr (void);
static double parse_unary (void);

static const char *
number_text (char   text[G_ASCII_DTOSTR_BUF_SIZE],
             double value)
{
    return g_ascii_formatd (text, G_ASCII_DTOSTR_BUF_SIZE, "%.10g", value);
}

/* --trace: a rule's function starts. */
static void
trace_enter (const char *rule)
{
    if (pb_parser_trace)
        g_printerr ("%*s%s {\n", 2 * depth, "", rule);
    depth++;
}

/* --trace: a rule's function returns value. */
static double
trace_leave (double value)
{
    char text[G_ASCII_DTOSTR_BUF_SIZE];

    depth--;
    if (pb_parser_trace)
        g_printerr ("%*s} = %s\n", 2 * depth, "", number_text (text, value));
    return value;
}

/* Use up the lookahead and read the next token. */
static void
advance (void)
{
    if (pb_parser_trace) {
        g_autoptr (GString) token = g_string_new (NULL);

        pb_append_token (token, lookahead);
        g_printerr ("%*s%s\n", 2 * depth, "", token->str);
    }
    lookahead = pb_scanner_next ();
}

/* Append the lookahead as error messages name it. */
static void
append_lookahead (GString *out)
{
    switch (lookahead) {
        case TOK_EOF:
            g_string_append (out, "end of input");
            break;
        case TOK_NUM:
            g_string_append (out, "number");
            break;
        case TOK_UNDEF:
            g_string_append (out, "character ");
            pb_append_char (out, (guchar) pb_scanner_text[0]);
            break;
        default:
            pb_append_char (out, lookahead);
            break;
    }
}

/*
 * Report the lookahead as unexpected and give up. expected names what
 * the grammar allows here. longjmp() skips g_autoptr cleanups, so the
 * message is freed by hand.
 */
static G_NORETURN void
syntax_error (const char *expected)
{
    GString *message = g_string_new ("pebble: error: unexpected ");

    append_lookahead (message);
    g_string_append_printf (message, ", expecting %s", expected);
    fflush (stdout);
    g_printerr ("%s\n", message->str);
    g_string_free (message, TRUE);
    longjmp (on_error, 1);
}

/* The lookahead must be the one-character token c: use it up. */
static void
expect (int c)
{
    char expected[] = { '\'', (char) c, '\'', '\0' };

    if (lookahead != c)
        syntax_error (expected);
    advance ();
}

/* factor : NUM | '(' expr ')' */
static double
parse_factor (void)
{
    double value;

    trace_enter ("factor");
    switch (lookahead) {
        case TOK_NUM:
            value = pb_scanner_value;
            advance ();
            break;
        case '(':
            advance ();
            value = parse_expr ();
            expect (')');
            break;
        default:
            /* parse_unary() would have taken a '-', so it is valid too. */
            syntax_error ("number, '-' or '('");
    }
    return trace_leave (value);
}

/* power : factor | factor '^' unary */
static double
parse_power (void)
{
    double value;

    trace_enter ("power");
    value = parse_factor ();
    if (lookahead == '^') {
        advance ();
        value = pow (value, parse_unary ());
    }
    return trace_leave (value);
}

/* unary : power | '-' unary */
static double
parse_unary (void)
{
    double value;

    trace_enter ("unary");
    if (lookahead == '-') {
        advance ();
        value = -parse_unary ();
    } else {
        value = parse_power ();
    }
    return trace_leave (value);
}

/* term : unary | term '*' unary | term '/' unary */
static double
parse_term (void)
{
    double value;

    trace_enter ("term");
    value = parse_unary ();
    while (lookahead == '*' || lookahead == '/') {
        int op = lookahead;
        double right;

        advance ();
        right = parse_unary ();
        value = op == '*' ? value * right : value / right;
    }
    return trace_leave (value);
}

/* expr : term | expr '+' term | expr '-' term */
static double
parse_expr (void)
{
    double value;

    trace_enter ("expr");
    value = parse_term ();
    while (lookahead == '+' || lookahead == '-') {
        int op = lookahead;
        double right;

        advance ();
        right = parse_term ();
        value = op == '+' ? value + right : value - right;
    }
    return trace_leave (value);
}

/* statement : expr ';' */
static void
parse_statement (void)
{
    char text[G_ASCII_DTOSTR_BUF_SIZE];
    double value;

    trace_enter ("statement");
    value = parse_expr ();
    expect (';');
    trace_leave (value);
    printf ("%s\n", number_text (text, value));
}

/* program : %empty | program statement */
int
pb_parse (void)
{
    depth = 0;
    if (setjmp (on_error) != 0)
        return 1;

    lookahead = pb_scanner_next ();
    while (lookahead != TOK_EOF)
        parse_statement ();
    return 0;
}
