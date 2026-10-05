// SPDX-License-Identifier: MIT

#pragma once

#include <glib.h>
#include <stdio.h>

/*
 * Token kinds, numbered the way yacc and bison number them:
 *
 *   0        end of input
 *   1-255    a one-character token is its own character code ('+' is 43)
 *   256      reserved for bison's "error" token
 *   257      a character that starts no token ("invalid token")
 *   258...   named tokens
 */
typedef enum
{
    TOK_EOF = 0,
    TOK_UNDEF = 257,
    TOK_NUM = 258,
} PbTokenKind;

/* Where tokens are read from; NULL means stdin. */
extern FILE       *pb_scanner_in;

/* The characters of the last token (its lexeme). */
extern const char *pb_scanner_text;

/* The value of the last TOK_NUM. */
extern double      pb_scanner_value;

int  pb_scanner_next    (void);
void pb_scanner_destroy (void);

void pb_append_char     (GString *out,
                         int      c);
void pb_append_token    (GString *out,
                         int      kind);
