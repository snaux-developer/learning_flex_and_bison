// SPDX-License-Identifier: MIT

#include "scanner.h"

#include <glib.h>

FILE       *pb_scanner_in = NULL;
const char *pb_scanner_text = "";
double      pb_scanner_value = 0.0;

/* Holds the lexeme; reused for every token. */
static GString *lexeme = NULL;

static gboolean
is_operator (int c)
{
    switch (c) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '(':
        case ')':
        case ';':
        case '^':
            return TRUE;
        default:
            return FALSE;
    }
}

int
pb_scanner_next (void)
{
    int kind;
    int c;

    if (pb_scanner_in == NULL)
        pb_scanner_in = stdin;
    if (lexeme == NULL)
        lexeme = g_string_new (NULL);
    g_string_truncate (lexeme, 0);

    /* Whitespace separates tokens but is not a token: skip it. */
    do {
        c = getc (pb_scanner_in);
    } while (c == ' ' || c == '\t' || c == '\r' || c == '\n');

    if (c == EOF) {
        kind = TOK_EOF;
    } else if (g_ascii_isdigit (c)) {
        /* A number: take digits for as long as they come. */
        while (g_ascii_isdigit (c)) {
            g_string_append_c (lexeme, c);
            c = getc (pb_scanner_in);
        }
        /* c is the first character after the number: give it back. */
        ungetc (c, pb_scanner_in);
        pb_scanner_value = g_ascii_strtod (lexeme->str, NULL);
        kind = TOK_NUM;
    } else {
        /* Any other character is a token on its own. */
        g_string_append_c (lexeme, c);
        kind = is_operator (c) ? c : TOK_UNDEF;
    }

    pb_scanner_text = lexeme->str;
    return kind;
}

void
pb_scanner_destroy (void)
{
    if (lexeme != NULL) {
        g_string_free (lexeme, TRUE);
        lexeme = NULL;
    }
    pb_scanner_in = NULL;
    pb_scanner_text = "";
}

/* Append c as C writes a character constant: '+', '\'', '\xC5'. */
void
pb_append_char (GString *out,
                int      c)
{
    if (c == '\'' || c == '\\')
        g_string_append_printf (out, "'\\%c'", c);
    else if (g_ascii_isprint (c))
        g_string_append_printf (out, "'%c'", c);
    else
        g_string_append_printf (out, "'\\x%02X'", (unsigned int) c);
}

/*
 * Append the token that pb_scanner_next() returned last, the way
 * --tokens prints it: NUM(12), '+', UNDEF('@'), EOF.
 */
void
pb_append_token (GString *out,
                 int      kind)
{
    switch (kind) {
        case TOK_EOF:
            g_string_append (out, "EOF");
            break;
        case TOK_NUM:
            g_string_append_printf (out, "NUM(%s)", pb_scanner_text);
            break;
        case TOK_UNDEF:
            g_string_append (out, "UNDEF(");
            pb_append_char (out, (guchar) pb_scanner_text[0]);
            g_string_append_c (out, ')');
            break;
        default:
            pb_append_char (out, kind);
            break;
    }
}
