// SPDX-License-Identifier: MIT

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "scanner.h"

/* Make the scanner read n bytes from memory instead of a file. */
static FILE *
open_source (const char *source,
             size_t      n)
{
    FILE *in = fmemopen ((void *) source, n, "r");

    g_assert_nonnull (in);
    pb_scanner_in = in;
    return in;
}

static void
close_source (FILE *in)
{
    pb_scanner_destroy ();
    fclose (in);
}

/* The next token must have this kind and this lexeme. */
static void
expect (int         kind,
        const char *text)
{
    g_assert_cmpint (pb_scanner_next (), ==, kind);
    g_assert_cmpstr (pb_scanner_text, ==, text);
}

static void
test_empty (void)
{
    FILE *in = open_source (" \t\r\n", strlen (" \t\r\n"));

    expect (TOK_EOF, "");
    expect (TOK_EOF, "");
    close_source (in);
}

static void
test_operators (void)
{
    const char *source = "+-*/();";
    FILE *in = open_source (source, strlen (source));

    for (const char *p = source; *p != '\0'; p++) {
        char text[] = { *p, '\0' };

        expect (*p, text);
    }
    expect (TOK_EOF, "");
    close_source (in);
}

static void
test_numbers (void)
{
    const char *source = "12 007\n9";
    FILE *in = open_source (source, strlen (source));

    expect (TOK_NUM, "12");
    g_assert_cmpfloat (pb_scanner_value, ==, 12.0);
    expect (TOK_NUM, "007");
    g_assert_cmpfloat (pb_scanner_value, ==, 7.0);
    expect (TOK_NUM, "9");
    expect (TOK_EOF, "");
    close_source (in);
}

static void
test_lookahead (void)
{
    const char *source = "12+3";
    FILE *in = open_source (source, strlen (source));

    expect (TOK_NUM, "12");
    expect ('+', "+");
    expect (TOK_NUM, "3");
    expect (TOK_EOF, "");
    close_source (in);
}

static void
test_undefined (void)
{
    /* Five bytes: '@', 'a', a NUL byte, '1' and ';'. */
    FILE *in = open_source ("@a\0" "1;", 5);

    expect (TOK_UNDEF, "@");
    expect (TOK_UNDEF, "a");
    g_assert_cmpint (pb_scanner_next (), ==, TOK_UNDEF);
    expect (TOK_NUM, "1");
    expect (';', ";");
    expect (TOK_EOF, "");
    close_source (in);
}

int
main (int    argc,
      char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/scanner/empty", test_empty);
    g_test_add_func ("/scanner/operators", test_operators);
    g_test_add_func ("/scanner/numbers", test_numbers);
    g_test_add_func ("/scanner/lookahead", test_lookahead);
    g_test_add_func ("/scanner/undefined", test_undefined);

    return g_test_run ();
}
