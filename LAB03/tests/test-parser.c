// SPDX-License-Identifier: MIT

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "scanner.h"

typedef struct
{
    const char *path;
    const char *source;
    const char *output;     /* expected standard output */
    const char *error;      /* expected standard error */
} Case;

#define OPERAND "expecting number, '-' or '('\n"

static const Case cases[] = {
    { "/parser/number", "42;", "42\n", "" },
    { "/parser/precedence", "1 + 2 * 3; 2 * 3 + 4;", "7\n10\n", "" },
    { "/parser/parentheses", "(1 + 2) * 3; ((7));", "9\n7\n", "" },
    { "/parser/left-to-right", "8 - 3 - 2; 8 / 4 / 2;", "3\n1\n", "" },
    { "/parser/division", "7 / 2;", "3.5\n", "" },
    { "/parser/power-right", "2 ^ 3 ^ 2; (2 ^ 3) ^ 2;", "512\n64\n", "" },
    { "/parser/power-tighter", "2 * 3 ^ 2; 2 ^ 3 * 2;", "18\n16\n", "" },
    { "/parser/unary", "-2; --2; 1 - -2; -2 * 3;", "-2\n2\n3\n-6\n", "" },
    { "/parser/unary-power", "-2 ^ 2; (-2) ^ 2; 2 ^ -2;",
      "-4\n4\n0.25\n", "" },
    /* Not "": under ASan, fmemopen() rejects a zero-length buffer. */
    { "/parser/blank", " \n", "", "" },
    { "/parser/error-after-output", "1 + 2; 3 +;", "3\n",
      "pebble: error: unexpected ';', " OPERAND },
    { "/parser/missing-operand", "1 * ;", "",
      "pebble: error: unexpected ';', " OPERAND },
    { "/parser/missing-semicolon", "1 + 2", "",
      "pebble: error: unexpected end of input, expecting ';'\n" },
    { "/parser/unbalanced", "(1 + 2;", "",
      "pebble: error: unexpected ';', expecting ')'\n" },
    { "/parser/two-numbers", "1 2;", "",
      "pebble: error: unexpected number, expecting ';'\n" },
    { "/parser/undefined", "1 @ 2;", "",
      "pebble: error: unexpected character '@', expecting ';'\n" },
    { "/parser/power-missing", "2 ^;", "",
      "pebble: error: unexpected ';', " OPERAND },
};

/* In the child process: parse source, then exit with pb_parse()'s result. */
static void
run_child (const char *source,
           gboolean    trace)
{
    FILE *in = fmemopen ((void *) source, strlen (source), "r");
    int status;

    g_assert_nonnull (in);
    pb_scanner_in = in;
    pb_parser_trace = trace;
    status = pb_parse ();
    pb_scanner_destroy ();
    fclose (in);
    exit (status);
}

/* The parser prints to stdout, so each case runs in a child process. */
static void
test_case (gconstpointer data)
{
    const Case *c = data;

    if (g_test_subprocess ())
        run_child (c->source, FALSE);

    g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
    if (c->error[0] == '\0')
        g_test_trap_assert_passed ();
    else
        g_test_trap_assert_failed ();
    g_test_trap_assert_stdout (c->output);
    g_test_trap_assert_stderr (c->error);
}

/* --trace prints one block per rule function, with its value. */
static void
test_trace (void)
{
    if (g_test_subprocess ())
        run_child ("-7;", TRUE);

    g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
    g_test_trap_assert_passed ();
    g_test_trap_assert_stdout ("-7\n");
    g_test_trap_assert_stderr ("statement {\n"
                               "  expr {\n"
                               "    term {\n"
                               "      unary {\n"
                               "        '-'\n"
                               "        unary {\n"
                               "          power {\n"
                               "            factor {\n"
                               "              NUM(7)\n"
                               "            } = 7\n"
                               "          } = 7\n"
                               "        } = 7\n"
                               "      } = -7\n"
                               "    } = -7\n"
                               "  } = -7\n"
                               "  ';'\n"
                               "} = -7\n");
}

int
main (int    argc,
      char **argv)
{
    g_test_init (&argc, &argv, NULL);

    for (gsize i = 0; i < G_N_ELEMENTS (cases); i++)
        g_test_add_data_func (cases[i].path, &cases[i], test_case);
    g_test_add_func ("/parser/trace", test_trace);

    return g_test_run ();
}
