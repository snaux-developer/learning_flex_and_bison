// SPDX-License-Identifier: MIT

#include <errno.h>
#include <glib.h>
#include <stdio.h>

#include "parser.h"
#include "scanner.h"

/* --tokens: scan the whole input; one line per statement. */
static void
print_tokens (void)
{
    g_autoptr (GString) line = g_string_new (NULL);
    int kind;

    do {
        kind = pb_scanner_next ();
        pb_append_token (line, kind);
        if (kind == ';' || kind == TOK_EOF) {
            printf ("%s\n", line->str);
            g_string_truncate (line, 0);
        } else {
            g_string_append_c (line, ' ');
        }
    } while (kind != TOK_EOF);
}

int
main (int    argc,
      char **argv)
{
    g_autoptr (GOptionContext) context = NULL;
    g_autoptr (GError) error = NULL;
    gboolean tokens = FALSE;
    gboolean trace = FALSE;
    GOptionEntry entries[] = {
        { "tokens", 0, 0, G_OPTION_ARG_NONE, &tokens,
          "Print the token stream instead of running", NULL },
        { "trace", 0, 0, G_OPTION_ARG_NONE, &trace,
          "Print the parser's calls and tokens while running", NULL },
        G_OPTION_ENTRY_NULL,
    };
    FILE *in = stdin;
    int status = 0;

    context = g_option_context_new ("[FILE]");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("pebble: %s\n", error->message);
        return 2;
    }
    if (argc > 2) {
        g_printerr ("pebble: too many arguments\n");
        return 2;
    }
    if (argc == 2) {
        in = fopen (argv[1], "r");
        if (in == NULL) {
            g_printerr ("pebble: %s: %s\n", argv[1], g_strerror (errno));
            return 2;
        }
    }

    pb_scanner_in = in;
    pb_parser_trace = trace;
    if (tokens)
        print_tokens ();
    else if (pb_parse () != 0)
        status = 2;
    pb_scanner_destroy ();

    if (in != stdin)
        fclose (in);
    return status;
}
