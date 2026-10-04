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
    int         status;     /* expected pb_parse() result */
} Case;

static const Case cases[] = {
    { "/parser/number", "42;", "42\n", 0 },
    { "/parser/precedence", "1 + 2 * 3; 2 * 3 + 4;", "7\n10\n", 0 },
    { "/parser/parentheses", "(1 + 2) * 3; ((7));", "9\n7\n", 0 },
    { "/parser/left-to-right", "8 - 3 - 2; 8 / 4 / 2;", "3\n1\n", 0 },
    { "/parser/division", "7 / 2;", "3.5\n", 0 },
    /* Not "": under ASan, fmemopen() rejects a zero-length buffer. */
    { "/parser/blank", " \n", "", 0 },
    { "/parser/error-after-output", "1 + 2; 3 +;", "3\n", 1 },
    { "/parser/missing-semicolon", "1 + 2", "", 1 },
    { "/parser/unbalanced", "(1 + 2;", "", 1 },
    { "/parser/undefined", "1 @ 2;", "", 1 },
};

/* The parser prints to stdout, so each case runs in a child process. */
static void
test_case (gconstpointer data)
{
    const Case *c = data;

    if (g_test_subprocess ()) {
        FILE *in = fmemopen ((void *) c->source, strlen (c->source), "r");
        int status;

        g_assert_nonnull (in);
        pb_scanner_in = in;
        status = pb_parse ();
        pb_scanner_destroy ();
        fclose (in);
        exit (status);
    }

    g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
    if (c->status == 0) {
        g_test_trap_assert_passed ();
        g_test_trap_assert_stderr ("");
    } else {
        g_test_trap_assert_failed ();
        g_test_trap_assert_stderr ("pebble: syntax error\n");
    }
    g_test_trap_assert_stdout (c->output);
}

int
main (int    argc,
      char **argv)
{
    g_test_init (&argc, &argv, NULL);

    for (gsize i = 0; i < G_N_ELEMENTS (cases); i++)
        g_test_add_data_func (cases[i].path, &cases[i], test_case);

    return g_test_run ();
}
