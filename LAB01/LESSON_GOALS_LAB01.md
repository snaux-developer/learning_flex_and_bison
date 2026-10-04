# LAB01 -- Tokens by hand

> You write a scanner by hand, so that you know exactly what flex writes
> for you from LAB04 on.

Prerequisites: GCC, Meson, Ninja and GLib 2.90 (the course baseline).
flex and bison are not used yet.

## What changed since the start of the course

Everything is new:

- `meson.build`, `src/meson.build`, `tests/meson.build`, `meson.format`:
  the build.
- `src/scanner.h`, `src/scanner.c`: the hand-written scanner.
- `src/main.c`: the `pebble` command and its `--tokens` option.
- `tests/test-scanner.c`: one GTest program.
- `examples/hello.pb`, `examples/tokens.pb`, `examples/unknown.pb`.
- `../.gitignore` (course root): ignores every lab's `_build/`.

## The idea

A C compiler, the PostgreSQL server and every interpreter do the same
thing with a program text: they never look at it as a whole. They work
in stages, and each stage hands a simpler kind of data to the next.
Here is the pebble statement `1 + 2;` on its way through them:

```text
  "1 + 2;"           the program: just characters
     |               '1' ' ' '+' ' ' '2' ';'
     v
+-----------+
|  scanner  |        groups characters into tokens   (flex, LAB04)
+-----------+
     |               NUM(1)  '+'  NUM(2)  ';'  EOF
     v
+-----------+
|  parser   |        finds the structure             (bison, LAB15)
+-----------+
     |                     statement
     |                    /         \
     |                 expr         ';'
     |               /  |   \
     |           NUM(1) '+'  NUM(2)
     v
+-----------+
|  actions  |        your C code gives it meaning    (you, always)
+-----------+
     |
     v
     3
```

The scanner knows nothing about statements or precedence. It answers
one question, over and over: "what is the next token?" The parser asks
it each time it needs one; it never receives a list. flex generates
scanners, bison generates parsers, and the meaning (computing `3`)
is always C code you write yourself.

In this lab you write the first stage by hand, in about 80 lines of C.
In LAB04 flex will write it for you from a list of patterns. Having
written one yourself, you will recognise every part of what flex
produces.

## New words

- **token** -- the smallest unit of meaning in a program, like a word
  in a sentence. `12 + 3;` has four tokens: `12`, `+`, `3`, `;`.
- **token kind** -- which sort of token it is. `12` and `3` have the
  same kind (`TOK_NUM`); `+` has the kind `'+'`. The parser decides
  almost everything from kinds alone.
- **lexeme** -- the exact characters of one token in the input. In
  `007;` the first token's lexeme is `007`.
- **pattern** -- the rule that says which lexemes belong to a kind.
  For `TOK_NUM` the pattern is "one or more digits". flex will write
  it as `[0-9]+`: `[0-9]` is any digit, `+` means "one or more"
  (LAB05).
- **semantic value** -- the data a token carries to the parser. The
  semantic value of `007` is the number 7. `+` needs none: its kind
  says everything.
- **scanner** -- the code that turns characters into tokens. Also
  called *lexer*, *lexical analyser* or *tokenizer*.
- **lexical analysis** -- the scanner's job: splitting characters into
  tokens.
- **whitespace** -- spaces, tabs and newlines between tokens. They
  separate tokens (`1 2` is two numbers, `12` is one) but are not tokens
  themselves.
- **lookahead** -- input that has been read but not yet used. The
  scanner knows that `12` is finished only after it has read the `+`
  in `12+3`. That `+` is one character of lookahead.
- **end of input** -- a token the scanner returns when the input is
  exhausted, so the parser knows nothing more is coming. Its kind is
  `TOK_EOF`, and its code is 0.
- **character code** -- the number a character is stored as: `'+'` is
  43 in ASCII. A C `char` constant such as `'+'` *is* that number.
- **parser** -- the stage after the scanner. It checks that the tokens
  form a valid program and finds its structure (the tree in the
  picture). The rules it follows are the language's *grammar*
  (LAB02).

One statement, taken apart:

| Lexeme | Kind      | Pattern (in words)  | Semantic value |
|--------|-----------|---------------------|----------------|
| `12`   | `TOK_NUM` | one or more digits  | 12             |
| `+`    | `'+'`     | the character `+`   | --             |
| `3`    | `TOK_NUM` | one or more digits  | 3              |
| `;`    | `';'`     | the character `;`   | --             |
|        | `TOK_EOF` | end of the input    | --             |

## Theory corner: a scanner is a finite automaton

A **finite automaton** is a machine with a fixed number of **states**.
It reads one character at a time, and each character moves it along an
arrow (a **transition**) to the next state. Some states are
**accepting**: being there means "a complete token has been read".
Here is the automaton for LAB01 (`(( ))` marks accepting states;
"blank" means space, tab, carriage return or newline):

```text
     blank
     .----.
     |    v
--> ( START ) ---- digit ----------> (( NUM )) ---.
     |  |  |                             ^        | digit
     |  |  |                             '--------'
     |  |  '------ + - * / ( ) ; ----> (( OP ))
     |  |
     |  '--------- end of input -----> (( EOF ))
     |
     '------------ anything else ----> (( UNDEF ))
```

The scanner starts in START and follows arrows. When the next character
has no arrow out of the current state, and the state is accepting, the
token is complete. That character is not part of the token, so it is
*given back* and becomes the first character of the next token.

Follow `12+3` through it:

```text
step  state  reads  what happens
----  -----  -----  ---------------------------------------------
 1    START  '1'    digit: go to NUM
 2    NUM    '2'    digit: stay in NUM
 3    NUM    '+'    no arrow; NUM accepts: token NUM "12".
                    Give '+' back.
 4    START  '+'    go to OP: token '+'
 5    START  '3'    digit: go to NUM
 6    NUM    end    no arrow; NUM accepts: token NUM "3"
 7    START  end    go to EOF: token EOF
```

Step 3 is the lookahead: the `+` is read once as the end of `12` and
once more as a token of its own.

### The state is the position in the code

An automaton has to remember exactly one thing between characters:
which state it is in. A program can keep that in a variable, or let
the program counter keep it, so that the state is *whichever line is
running*. `scanner.c` does the second; it has no variable called
`state`:

| Automaton                  | `scanner.c`                               |
|----------------------------|-------------------------------------------|
| in START                   | inside the `do`/`while` blank loop        |
| in NUM                     | inside `while (g_ascii_isdigit (c))`      |
| START --blank--> START     | the blank loop going round again          |
| START --digit--> NUM       | taking `else if (g_ascii_isdigit (c))`    |
| NUM --digit--> NUM         | the digit loop going round again          |
| no arrow out of NUM        | falling out of the digit loop             |
| OP, UNDEF, EOF (accepting) | the branches that set `kind`, then return |

A transition is plain control flow: an `if` taken, or a loop going
round again. To know which state the scanner is in, stop it in gdb
and look at the line number.

Here is the same automaton with the state in a variable instead:

```c
enum { S_START, S_NUM } state = S_START;

for (;;) {
    c = getc (in);
    switch (state) {
        case S_START:
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                break;
            if (c == EOF)
                return TOK_EOF;
            g_string_append_c (lexeme, c);
            if (!g_ascii_isdigit (c))
                return is_operator (c) ? c : TOK_UNDEF;
            state = S_NUM;
            break;
        case S_NUM:
            if (!g_ascii_isdigit (c)) {
                ungetc (c, in);
                return TOK_NUM;
            }
            g_string_append_c (lexeme, c);
            break;
    }
}
```

Apart from setting `pb_scanner_text` and `pb_scanner_value`, left out
here, it behaves exactly like `scanner.c`. But now the state is data.
Replace the `switch` with a table lookup, `state = next[state][c]`, and
you have the shape of what flex generates. This loop is the heart of
every flex scanner (abridged; you will find it in `_build/src/scanner.c`
from LAB04 on):

```c
do {
    YY_CHAR yy_c = yy_ec[YY_SC_TO_UI(*yy_cp)];
    if (yy_accept[yy_current_state]) {
        yy_last_accepting_state = yy_current_state;
        yy_last_accepting_cpos = yy_cp;
    }
    ...
    yy_current_state = yy_nxt[yy_base[yy_current_state] + yy_c];
    ++yy_cp;
} while (yy_base[yy_current_state] != 6);
```

- `yy_current_state` is an `int`: the state, held in a variable.
- `yy_cp` points at the next character in flex's own input buffer;
  flex does not call `getc()` for each character.
- `yy_nxt`, `yy_base` and their companions are the arrows, stored as
  compressed tables (the hidden lines deal with the compression).
- `yy_accept` marks the accepting states. On the way, flex remembers
  the last accepting state it passed and where in the input that was.
- The loop stops when there is no arrow for the next character. flex
  models that as a dead state; the `6` belongs to one particular
  scanner.

This code is the same in every flex scanner. Adding a rule to a `.l`
file changes the tables, not the loop. LAB07 prints the tables.

Why it matters which one you choose:

- **Size.** For a handful of states, code shaped like the drawing is
  the clearest thing you can write. For the hundreds of states flex
  builds from a few dozen patterns, hand-written control flow becomes
  unmanageable. Tables scale, and producing them from patterns is
  mechanical: exactly the job flex does for you.
- **Giving back.** In exercise 5 (`1.x`) the scanner must return to the
  last accepting state and give back two characters. By hand you write
  that recovery yourself, case by case, and `ungetc()` only guarantees
  one character. flex does it once for every scanner: it jumps back to
  `yy_last_accepting_state` and `yy_last_accepting_cpos` in its buffer.
- **Pausing.** A state kept in the program counter exists only while
  the function runs. LAB01's scanner cannot stop halfway through a
  number, return "no more input yet", and continue later: returning
  loses the state, so `getc()` has to wait for input. A state kept in a
  variable can be stored in a struct and resumed from there, which is
  what you need when data arrives in chunks, as from a `GInputStream`
  in a GTK main loop.

Parsers make the same choice. The parser you write by hand in LAB02
keeps its state in code positions and in the C call stack: which
functions are active, and where each one is. LAB03's `--trace` prints
exactly that. bison's parser keeps a stack of state numbers as data
(LAB16, LAB17). That is why it can throw states away to recover from a
syntax error (LAB28) and pause between tokens to wait for input
(LAB38).

## The new code, line by line

### `src/scanner.h`: the scanner's interface

```c
typedef enum
{
    TOK_EOF = 0,
    TOK_UNDEF = 257,
    TOK_NUM = 258,
} PbTokenKind;
```

These numbers follow a convention that yacc started in the 1970s and
bison still uses. The parser you build in LAB15 relies on it, so it is
fixed here, before any tool appears:

- **0 is end of input.** A parser stops when the scanner returns 0, so
  no real token may use 0.
- **1-255: a one-character token is its own character code.** The
  scanner returns `'+'` (43) for a plus sign, and the grammar writes
  `'+'` too. No names, no enum entries. That is also why
  `pb_scanner_next()` returns `int`: most token kinds are not members
  of the enum.
- **256 is reserved** for bison's `error` token (LAB28).
- **257 is "invalid token".** pebble returns it for a character that
  starts no token, such as `@`. bison calls it `YYUNDEF`; in LAB15 the
  generated header names it `TOK_YYUNDEF`.
- **258 and up are named tokens.** `TOK_NUM` is the first. Every
  later one (`TOK_IDENT`, `TOK_PRINT`, ...) takes the next number, and
  none can ever collide with a character code.

The `TOK_` prefix is the course's naming rule (bison adds it for us
from LAB15). It also avoids a clash: `EOF` alone is already a macro in
`<stdio.h>`.

```c
extern FILE       *pb_scanner_in;
extern const char *pb_scanner_text;
extern double      pb_scanner_value;

int  pb_scanner_next    (void);
void pb_scanner_destroy (void);
```

- `pb_scanner_next()` reads and returns the next token's kind.
- `pb_scanner_in` is where it reads from; `NULL` means standard input.
- `pb_scanner_text` is the lexeme of the token just returned.
- `pb_scanner_value` is the semantic value of the last `TOK_NUM`. It
  is a `double` although only integers exist so far: LAB06 adds `1.5`,
  and the bison parser of LAB15 computes in `double`, so the type
  never has to change.
- `pb_scanner_destroy()` frees the scanner's memory at the end.

This is the classic lex interface, under pebble's names. Learn the
mapping now; it is exactly what flex and bison give you:

| LAB01                   | flex / bison (LAB04, LAB15)    |
|-------------------------|--------------------------------|
| `pb_scanner_next ()`    | `yylex ()`                     |
| `pb_scanner_in`         | `yyin`                         |
| `pb_scanner_text`       | `yytext`                       |
| `pb_scanner_value`      | `yylval`                       |
| `pb_scanner_destroy ()` | `yylex_destroy ()` (LAB14)     |

Yes, these are globals. That is how lex and yacc have always worked.
LAB31-LAB33 remove every one of them.

### `src/scanner.c`: the scanner

```c
static GString *lexeme = NULL;
```

The characters of the current token are collected in a `GString`, so
a 500-digit number cannot overflow a fixed buffer. It is reused for
every token.

```c
static gboolean
is_operator (int c)
{
    switch (c) {
        case '+':
        ...
        case ';':
            return TRUE;
```

The set of one-character tokens. Adding `%` later means adding one
`case`. Why not `strchr ("+-*/();", c)`? Because `strchr()` also
finds the string's terminating `'\0'`: a NUL byte in the input would
count as an operator and be returned as token 0, which means "end of
input". The `switch` cannot make that mistake.

```c
    if (pb_scanner_in == NULL)
        pb_scanner_in = stdin;
    if (lexeme == NULL)
        lexeme = g_string_new (NULL);
    g_string_truncate (lexeme, 0);
```

Defaults on the first call (flex does the same with `yyin`), then an
empty lexeme for the new token.

```c
    do {
        c = getc (pb_scanner_in);
    } while (c == ' ' || c == '\t' || c == '\r' || c == '\n');
```

The START state's blank loop. Afterwards `c` holds the first character
of the token, or the C macro `EOF`. Do not confuse the two EOFs: `EOF`
(-1) is what `getc()` returns when the file is exhausted; `TOK_EOF`
(0) is the token the scanner returns because of that.

```c
    if (c == EOF) {
        kind = TOK_EOF;
    } else if (g_ascii_isdigit (c)) {
        while (g_ascii_isdigit (c)) {
            g_string_append_c (lexeme, c);
            c = getc (pb_scanner_in);
        }
        ungetc (c, pb_scanner_in);
        pb_scanner_value = g_ascii_strtod (lexeme->str, NULL);
        kind = TOK_NUM;
```

The NUM state. The loop always reads one character too many: the
first non-digit. `ungetc()` pushes it back into the stream, so the
next call to `getc()` returns it again. That is step 3 of the trace.
C guarantees one character of push-back per stream, which is exactly
the one character of lookahead this scanner needs. If the number ends
at the end of the file, `c` is `EOF` and `ungetc (EOF, ...)` does
nothing, which is also what we want.

`g_ascii_strtod()` turns the lexeme into the semantic value. Unlike
`strtod()` it ignores the locale: under `pl_PL.UTF-8`, `strtod()`
expects `1,5`, not `1.5`. It is harmless for integers and essential
from LAB06 on.

```c
    } else {
        g_string_append_c (lexeme, c);
        kind = is_operator (c) ? c : TOK_UNDEF;
    }

    pb_scanner_text = lexeme->str;
    return kind;
```

Any other character is a token by itself: its own code if it is an
operator, `TOK_UNDEF` if not. `pb_scanner_text` is set last, because
appending may have moved the `GString`'s buffer.

`pb_scanner_destroy()` frees the `GString` and resets the globals, so
valgrind finds nothing left over.

### `src/main.c`: the command (support code)

`GOptionContext` parses `--tokens` and writes `--help` for free.
`print_tokens()` calls `pb_scanner_next()` until `TOK_EOF` and prints
each token: `NUM(lexeme)`, `UNDEF(char)`, or the character in quotes.
It starts a new line after each `;`, so the output has one line per
statement. `print_char()` writes non-printable bytes as `'\xC5'`.
Exit codes: 0 for success, 2 for usage errors. Without `--tokens`,
`pebble` refuses to run: there is no parser until LAB02.

### `tests/test-scanner.c`

`fmemopen()` turns a string into a `FILE *`, so a test can scan a
string as if it were a file. `expect (kind, text)` checks the next
token's kind and lexeme. One test per situation: blanks only, every
operator, numbers, `12+3` without spaces (the lookahead case) and
undefined characters, including a NUL byte.

### The build

You know Meson from the other course, so only what matters here:

- `project()` sets version `1.1.0` (`1.<lab>.0` in every lab),
  `c_std=gnu17` and `warning_level=2` (`-Wall -Wextra`). With
  `gnu17`, `<stdio.h>` also declares POSIX functions such as
  `fmemopen()`, and later `fileno()`, which flex's output calls.
- The `GLIB_VERSION_*` defines make the compiler warn if the code uses
  anything newer than GLib 2.90 or deprecated in it.
- `src/meson.build` builds everything except `main.c` into the static
  library `libpebble`. The `pebble` executable and the tests both link
  it through `libpebble_dep`.
- `tests/meson.build` registers the test program with
  `protocol: 'tap'`. A GTest program reports in TAP, the Test Anything
  Protocol: one `ok N /scanner/...` or `not ok` line per test case.
  With `protocol: 'tap'` Meson reads those lines, so it counts each
  case as a subtest and names the one that failed, instead of only
  checking the program's exit status. `meson test -v` lists them all.
- `meson.format` holds the settings for `meson format`.

## New syntax

No flex or bison syntax yet, but the token numbering already belongs
to bison:

| Construct         | Meaning                                     | Tool  |
|-------------------|---------------------------------------------|-------|
| token code 0      | end of input                                | bison |
| `'+'` (43)        | a one-character token is its own code       | bison |
| token code 256    | reserved for the `error` token              | bison |
| token code 257    | "invalid token" (`YYUNDEF`)                 | bison |
| 258 and up        | named tokens (`TOK_NUM`, ...)               | bison |

## Walkthrough

From `LAB01/`:

```sh
meson setup _build
meson compile -C _build
meson test -C _build --print-errorlogs
```

```text
1/1 pebble:scanner OK              0.01s   5 subtests passed
Ok:                1
```

The statement from the big picture:

```sh
./_build/src/pebble --tokens examples/hello.pb
```

```text
NUM(1) '+' NUM(2) ';'
EOF
```

A file with spaces, a tab, no spaces and parentheses:

```sh
cat examples/tokens.pb
./_build/src/pebble --tokens examples/tokens.pb
```

```text
NUM(12) '+' NUM(3) ';'
NUM(2) '*' '(' NUM(30) '-' NUM(4) ')' '/' NUM(7) ';'
NUM(1000000) ';'
EOF
```

`12+3` gives the same tokens as `12 + 3` would. The blanks are gone,
and `1000000` is one token, not seven.

With no file, `pebble` reads standard input:

```sh
echo '7*(8-5);' | ./_build/src/pebble --tokens
```

```text
NUM(7) '*' '(' NUM(8) '-' NUM(5) ')' ';'
EOF
```

Characters that start no token:

```sh
./_build/src/pebble --tokens examples/unknown.pb
```

```text
NUM(12) UNDEF('a') UNDEF('b') UNDEF('c') UNDEF('@') NUM(3) ';'
EOF
```

And there is nothing to run yet:

```sh
./_build/src/pebble examples/hello.pb; echo "exit $?"
```

```text
pebble: there is no parser yet; use --tokens
exit 2
```

To see the raw characters the scanner reads, try
`od -c examples/tokens.pb`.

## Exercises

1. **Write it: the `%` operator.** pebble gets a modulo operator in
   LAB17. Make the scanner return `'%'` as a token, and add `%` to the
   string in `test_operators()`. First predict what
   `printf '7 %% 2;' | ./_build/src/pebble --tokens` prints today.
   (`printf` needs `%%` to print one `%`.)
2. **Predict, then decide: `12abc`.** Predict the tokens for the first
   word of `examples/unknown.pb`, then check. Now decide what *should*
   happen. Find at least two sensible answers and one consequence of
   each. No code, just a decision.
3. **Write it: `#` comments.** A `#` starts a comment that runs to the
   end of the line. Like blanks, comments are not tokens. Make
   `printf '1 # one\n+ 2; # end' | ./_build/src/pebble --tokens` print
   `NUM(1) '+' NUM(2) ';'` and `EOF`. Count the lines you added: LAB09
   does this in flex with one line.
4. **Break it: forget to give back.** Comment out the `ungetc()` call.
   Predict the tokens for `12+3;`, then run it. Rebuild, run
   `meson test -C _build --print-errorlogs`, and read which test fails
   and what the numbers in the message mean. (`--print-errorlogs`
   prints a failing test's output in the terminal; without it, the
   message is only in `_build/meson-logs/testlog.txt`.)
5. **Draw it: decimal fractions.** Extend the automaton from the
   theory corner so that `12.5` is one `NUM`. Decide whether `12.` and
   `.5` are numbers. Then follow `1.x` through your drawing: what must
   the scanner give back, and why is that a problem for `ungetc()`?
6. **Inspect: bytes, not letters.** Run
   `printf 'zażółć;' | ./_build/src/pebble --tokens` and
   `printf 'zażółć;' | od -An -tx1`. How many `UNDEF` tokens are there,
   and why more than the six letters?

## Check your understanding

1. Why does the scanner call `ungetc()` after a number, but not after
   `+`?
2. What are `EOF` and `TOK_EOF`, and what are their values?
3. Why do named tokens start at 258, and not at 1?
4. For the input `007;`, what are the kind, the lexeme and the semantic
   value of the first token?
5. Where in `scanner.c` is the NUM state of the automaton?
6. LAB01's scanner keeps its state in the program counter. Name two
   things that become possible once the state is kept in a variable.

## Compare with

- bison's `examples/c/rpcalc/rpcalc.y`, the `yylex()` function at the
  end. On your machine: `~/opt/gtk/4.24.0/install/share/doc/bison/`.
  It is the same idea in 20 lines: skip blanks, return `YYEOF` (bison's
  name for token 0) at end of input, return any other character as its
  own code. It differs in two ways. It has no "invalid token": every
  character becomes a token, and the parser rejects the bad ones. And
  after `ungetc()` it lets `scanf ("%lf", &yylval)` read the whole
  number, so the C library does the digit loop.

## Look up later

- `getc_unlocked()` -- `getc()` without stream locking. flex reads in
  large blocks instead (`YY_INPUT`, LAB12).
- `open_memstream()` -- the writing counterpart of `fmemopen()`.
- More than one character of push-back -- not portable with `ungetc()`.
  flex keeps its own buffer and can back up any distance (LAB43).

## Answers

**1.** Before: `NUM(7) UNDEF('%') NUM(2) ';'`. In `is_operator()`:

```c
        case ';':
        case '%':
            return TRUE;
```

and in `test_operators()`:

```c
    const char *source = "+-*/();%";
```

Afterwards: `NUM(7) '%' NUM(2) ';'`.

**2.** Today: `NUM(12) UNDEF('a') UNDEF('b') UNDEF('c')`. Choices:

- Reject it in the scanner as a malformed number. gcc does this:
  `invalid suffix "abc" on integer constant`. Clear message, but the
  scanner must look past the digits.
- Scan it as `NUM(12)` followed by a name `abc` (names arrive in LAB07)
  and let the parser reject two values in a row. The scanner stays
  simple; the message ("unexpected identifier") is less precise.
- Keep today's behaviour. Simple, but four tokens for one mistake make
  for four confusing messages later.

flex's longest-match rule (LAB07) gives you the second choice for
free.

**3.** Replace the blank loop:

```c
    /* Skip whitespace and comments; neither is a token. */
    for (;;) {
        c = getc (pb_scanner_in);
        if (c == '#') {
            /* A comment runs to the end of the line. */
            while (c != '\n' && c != EOF)
                c = getc (pb_scanner_in);
        }
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            break;
    }
```

After a comment `c` is `'\n'` (a blank, so the loop goes on) or `EOF`
(which ends it). About ten lines, a nested loop, and the `EOF` case to
remember. In flex (LAB09) the same job is one short rule.

**4.** `NUM(12) NUM(3) EOF`. The character after each number is
swallowed: here the `+` and the `;`. `/scanner/lookahead` fails with
`assertion failed (pb_scanner_next () == kind): (258 == 43)`: the
scanner returned 258 (`TOK_NUM`, the `3`) where the test expected 43
(`'+'`).

**5.** One answer, where `12.` is *not* a number:

```text
--> ( START ) -digit-> (( INT )) -'.'-> ( DOT ) -digit-> (( FRAC ))
                        ^     |                           ^      |
                        '-----' digit                     '------' digit
```

For `.5`, add an arrow `START -'.'-> DOT`. Now `1.x`: after `1`, `.`
and `x` the scanner is in DOT, which does not accept, and there is no
arrow for `x`. It must fall back to the last accepting state (INT,
after `1`) and give back *two* characters, `.` and `x`. `ungetc()`
only guarantees one. If `12.` counts as a number (as in C), DOT becomes
accepting and the problem goes away, but `1..5` (LAB11) then scans as
`1.` followed by `.5`. flex handles both cases with its own buffer;
LAB11 and LAB43 return to this.

**6.** Ten `UNDEF` tokens. `z` and `a` are one byte each, but `ż`,
`ó`, `ł` and `ć` take two bytes each in UTF-8 (`c5 bc`, `c3 b3`,
`c5 82`, `c4 87`). `getc()` reads bytes, not letters. LAB13 teaches
the scanner UTF-8.

**Questions.**

1. After a number the scanner has read one character too many: the
   first non-digit, which belongs to the next token. After `+` it has
   read nothing extra: a one-character token is complete as soon as it
   is seen.
2. `EOF` is a C macro, -1, returned by `getc()` when the file is
   exhausted. `TOK_EOF` is a token kind, 0, returned by the scanner to
   tell the parser that the input has ended.
3. Codes 1-255 are taken by one-character tokens (their character
   codes), 256 by `error` and 257 by "invalid token". Starting at 258
   means a named token can never be mistaken for a character.
4. Kind `TOK_NUM`, lexeme `007`, semantic value 7.
5. The `while (g_ascii_isdigit (c))` loop. Being inside that loop *is*
   being in NUM.
6. Any two of: pausing when input runs out and resuming later (the
   state is saved in a struct); driving any number of states from one
   loop and a table generated from patterns (flex); remembering the last
   accepting state, so that giving back characters works the same way
   for every rule.
