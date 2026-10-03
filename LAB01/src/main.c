// SPDX-License-Identifier: MIT

#include <errno.h>
#include <glib.h>
#include <stdio.h>

#include "scanner.h"

/* Print a character as C would write it: '+', '\'', '\xC5'. */
static void
print_char (int c)
{
    if (c == '\'' || c == '\\')
        printf ("'\\%c'", c);
    else if (g_ascii_isprint (c))
        printf ("'%c'", c);
    else
        printf ("'\\x%02X'", (unsigned int) c);
}

/* --tokens: scan the whole input; one line per statement. */
static void
print_tokens (void)
{
    int kind;

    do {
        kind = pb_scanner_next ();
        switch (kind) {
            case TOK_EOF:
                printf ("EOF\n");
                break;
            case TOK_NUM:
                printf ("NUM(%s)", pb_scanner_text);
                break;
            case TOK_UNDEF:
                printf ("UNDEF(");
                print_char ((unsigned char) pb_scanner_text[0]);
                printf (")");
                break;
            default:
                print_char (kind);
                break;
        }
        if (kind == ';')
            printf ("\n");
        else if (kind != TOK_EOF)
            printf (" ");
    } while (kind != TOK_EOF);
}

int
main (int    argc,
      char **argv)
{
    g_autoptr (GOptionContext) context = NULL;
    g_autoptr (GError) error = NULL;
    gboolean tokens = FALSE;
    GOptionEntry entries[] = {
        { "tokens", 0, 0, G_OPTION_ARG_NONE, &tokens,
          "Print the token stream instead of running", NULL },
        G_OPTION_ENTRY_NULL,
    };
    FILE *in = stdin;

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
    if (!tokens) {
        g_printerr ("pebble: there is no parser yet; use --tokens\n");
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
    print_tokens ();
    pb_scanner_destroy ();

    if (in != stdin)
        fclose (in);
    return 0;
}
