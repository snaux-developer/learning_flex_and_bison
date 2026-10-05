# LAB03 -- Associativity, unary minus and lookahead by hand

> You add a right-associative operator and a unary operator to the
> hand-written parser, see exactly why some rules must become loops
> while others stay recursive, and find where hand-written parsers start
> to hurt.

Prerequisites: none new.

## What changed since LAB02

- `src/scanner.c`, `src/scanner.h`: the `^` token. `pb_append_char()`
  and `pb_append_token()` format tokens; they moved here from `main.c`
  so that `--tokens` and the new `--trace` print tokens the same way.
- `src/parser.c`, `src/parser.h`: two new levels, `unary` and `power`;
  `--trace` (`pb_parser_trace`); error messages that name the unexpected
  token and what was expected.
- `src/main.c`: the `--trace` option; `--tokens` uses
  `pb_append_token()`.
- `meson.build`, `src/meson.build`: the C math library, for `pow()`.
- `tests/test-scanner.c`, `tests/test-parser.c`: cases for `^`, unary
  minus, every error message and the trace.
- `examples/power.pb` (new).
- Version `1.3.0`.

## The idea

LAB02's parser handles `+`, `-`, `*`, `/` and parentheses. This lab
adds two operators that the finished pebble language has, and each one
brings something new.

**Power, `^`.** `2 ^ 3` is 8. The question is what `2 ^ 3 ^ 2` means.
Mathematics writes it as 2 to the power 3², and computes it from the
right: `2 ^ (3 ^ 2) = 2 ^ 9 = 512`. Computing from the left would give
`(2 ^ 3) ^ 2 = 8 ^ 2 = 64`. Subtraction is the other way round:
`8 - 3 - 2` is computed from the left. So operators of the same level
can group in two directions, and the grammar has to say which.

**Negation, unary `-`.** In `-2`, the minus has only one operand. The
scanner cannot tell this minus from the one in `5 - 2`: both are the
token `'-'`. The parser must work out from the position which one it
is. And it must give `-2 ^ 2` the meaning mathematics gives `-2²`:
`-(2 ^ 2) = -4`, not `(-2) ^ 2 = 4`.

Along the way you will see why LAB02 had to turn its left-recursive
rules into loops while `^` can stay a plain recursive call, and what
"one token of lookahead" means precisely. The parser also gets two
tools: error messages that say what was found and what was expected,
and a `--trace` option that prints the parse tree as the parser builds
it. At the end of the lab, the same tools show where a hand-written
parser starts to hurt: messages that are hard to keep right, and code
that grows with every rule. Those two problems are why bison exists.

## New words

- **operand** -- a value an operator works on. In `5 - 2` the operands
  are `5` and `2`.
- **binary operator** -- an operator with two operands, one on each
  side: `5 - 2`.
- **unary operator** -- an operator with one operand. In `-2`, the
  minus is unary: it negates the `2`.
- **left-associative** -- operators of one level group from the left:
  `8 - 3 - 2` means `(8 - 3) - 2`. `+`, `-`, `*` and `/` are
  left-associative.
- **right-associative** -- operators of one level group from the right:
  `2 ^ 3 ^ 2` means `2 ^ (3 ^ 2)`. `^` is right-associative.
- **non-associative** -- an operator that may not be chained at all:
  pebble's comparisons (LAB05) will reject `1 < 2 < 3`.
- **LL(1)** -- the kind of grammar a recursive-descent parser can follow
  with one token of lookahead. The name is explained in the theory
  corner.
- **trace** -- a printed record of what a program does as it runs. In
  this lab: which parser functions start and finish, and which tokens
  they use up.
- **call tree** -- all the function calls of a run, drawn as a tree:
  each call's children are the calls it makes. For a recursive-descent
  parser, the call tree is the parse tree.
- **expected tokens** -- the tokens the grammar would accept at the
  point where a syntax error is found. A good message lists them:
  `unexpected ';', expecting ')'`.

## Theory corner

### Associativity, and how the grammar chooses it

Here is the new grammar. The two new levels are `unary` and `power`:

```yacc
expr
    : term
    | expr '+' term
    | expr '-' term
    ;

term
    : unary
    | term '*' unary
    | term '/' unary
    ;

unary
    : power
    | '-' unary
    ;

power
    : factor
    | factor '^' unary
    ;

factor
    : NUM
    | '(' expr ')'
    ;
```

(`program` and `statement` are unchanged from LAB02.)

Look at where each rule refers back to its own level.

- In `expr : expr '-' term`, the rule names `expr` on the *left* of the
  operator. That is left recursion. The left operand may contain more
  `-` operators; the right operand, a `term`, may not.
- In `power : factor '^' unary`, the right operand is a `unary`, and a
  `unary` can be a `power` again. So the rule leads back to its own
  level on the *right* of the operator. That is right recursion. The
  right operand may contain more `^` operators; the left operand, a
  `factor`, may not.

That one difference decides the grouping. Recall the rule from LAB02:
an operator lower in the tree is done earlier. Here is the parse tree
of `2 ^ 3 ^ 2`, starting from `power`:

```text
          power
     _______|________
    |       |        |
  factor   '^'     unary
    |                |
  NUM(2)           power
              _______|________
             |       |        |
           factor   '^'     unary
             |                |
           NUM(3)           power
                              |
                            factor
                              |
                            NUM(2)
```

The second `^` sits below the first one, inside its right operand. So
`3 ^ 2 = 9` is computed first, and then `2 ^ 9 = 512` at the root. The
tree cannot be built the other way round: for `(2 ^ 3)` to be the left
operand of a `^`, it would have to be a `factor`, and a factor contains
a `^` only inside parentheses.

For `8 - 3 - 2` it is the mirror image, as LAB02 showed: the right
operand of `expr '-' term` cannot contain a `-`, so the *last* `-` is at
the root, the first `-` is below it in the left operand, and
`8 - 3 = 5` is computed first.

In short: **left recursion makes an operator left-associative, and
right recursion makes it right-associative.** Earlier operators end up
deeper with left recursion, later ones with right recursion, and
deeper means sooner.

### Why left recursion loops forever and right recursion does not

LAB02 showed that `parse_term()` cannot follow `term : term '*' factor`
literally, because it would call itself before reading anything. Yet
in this lab, `parse_unary()` calls itself and `parse_power()` calls
back into `parse_unary()`, and both work. What is the difference?

The question to ask of every recursive call is: **has the function used
up at least one token before it calls itself again?**

- `expr : expr '+' term`, followed literally: `parse_expr()` would start
  by calling `parse_expr()`. Nothing has been used up, so the new call
  starts in exactly the same situation as the old one, with the same
  lookahead, and makes the same call again. Nothing ever changes, so
  the calls never stop: the C stack overflows. Exercise 1 lets you
  watch it.
- `unary : '-' unary`: `parse_unary()` first checks that the lookahead
  is `-`, uses it up, and only then calls itself. Every new call starts
  one token further along the input. A program of 100 tokens can
  therefore cause at most 100 nested calls, and the recursion must end.
- `power : factor '^' unary`: `parse_power()` reads a factor (at least
  one token) and the `^` before it recurses through `parse_unary()`.
  Again, progress is guaranteed.

So for recursive descent:

- a **left-associative** operator needs left recursion, which cannot
  be followed literally, so it becomes a **loop** (LAB02's
  `parse_term()` and `parse_expr()`);
- a **right-associative** operator uses right recursion, which can be
  written **exactly as the rule reads**, as a recursive call.

Here is `parse_power()`. It is the rule `power : factor | factor '^'
unary`, word for word:

```c
    value = parse_factor ();
    if (lookahead == '^') {
        advance ();
        value = pow (value, parse_unary ());
    }
```

One limit remains. "The recursion must end" is not the same as "the
recursion is small". Each level of nesting in the input costs a few C
stack frames: a million opening parentheses in a row crash this parser
with a stack overflow (a thousand are no problem). bison's parsers keep
their own stack, outside the C stack, and report "memory exhausted"
instead of crashing (LAB20, LAB29).

### Unary minus and its precedence

The levels are now, from loosest to tightest:

| Level    | Operators          | Its operands are            |
|----------|--------------------|-----------------------------|
| `expr`   | `+` `-` (binary)   | `term`s                     |
| `term`   | `*` `/`            | `unary`s                    |
| `unary`  | `-` (unary)        | a `unary`                   |
| `power`  | `^`                | a `factor`, then a `unary`  |
| `factor` | (none)             | `NUM`, or `( expr )`        |

Four inputs show what this order does.

**`-2 ^ 2` is -4.** The left operand of `^` is a `factor`, and a factor
cannot begin with `-` (unless the minus is inside parentheses). So the
minus cannot belong to the left operand of `^`; it must apply to the
whole `2 ^ 2`. The tree is `'-' unary` on top, with the `power` for
`2 ^ 2` below it: `2 ^ 2 = 4` first, then `-4`. If you mean the other
grouping, write `(-2) ^ 2`, which is 4.

**`2 ^ -2` is 0.25.** The right operand of `^` is a `unary`, which may
begin with `-`. That is why the rule is `factor '^' unary` and not
`factor '^' power`: with `power` on the right, `2 ^ -2` would be a
syntax error. Python and Lua accept it, and so does pebble.

**`-2 * 3` is -6, and so is `2 * -3`.** `unary` is tighter than `term`,
so each operand of `*` may carry its own minus.

**`1 - -2` is 3.** The same token, `'-'`, plays two roles here, and the
parser tells them apart by *where* it meets them:

- The first `-` arrives after a complete operand, `1`. `parse_expr()`
  is in its loop, looking for `+` or `-` after a term. There, a `-`
  means subtraction.
- The second `-` arrives where an operand must begin. `parse_unary()`
  is the function looking at it. There, a `-` means negation.

The scanner never needs to know the difference. This is a general
pattern: the scanner names *what* a token is, and the grammar decides
*what it does* from where it stands. bison grammars meet the same
question in LAB18, where unary minus gets a precedence of its own.

### One token of lookahead: LL(1)

Every decision in `parser.c` looks at exactly one token, the lookahead.
Here is the complete list:

| Function            | Lookahead       | Decision                     |
|---------------------|-----------------|------------------------------|
| `pb_parse()`        | end of input    | stop                         |
|                     | anything else   | one more statement           |
| `parse_expr()` loop | `+` or `-`      | one more term                |
|                     | anything else   | the expr is complete         |
| `parse_term()` loop | `*` or `/`      | one more unary               |
|                     | anything else   | the term is complete         |
| `parse_unary()`     | `-`             | `'-' unary`                  |
|                     | anything else   | `power`                      |
| `parse_power()`     | `^`             | `factor '^' unary`           |
|                     | anything else   | `factor` alone               |
| `parse_factor()`    | `NUM`           | `NUM`                        |
|                     | `(`             | `'(' expr ')'`               |
|                     | anything else   | syntax error                 |

A grammar that a parser can follow like this, choosing at every step
from one token, is called **LL(1)**:

- the first **L**: the input is read from **L**eft to right, one token
  at a time;
- the second **L**: the parser builds a **L**eftmost derivation, top
  down, as LAB02 showed;
- **(1)**: one token of lookahead is enough for every decision.

What does a grammar need to be LL(1)? At every choice, the alternatives
must begin with *different* tokens, so that one token can tell them
apart. `factor` is fine: `NUM` starts one alternative and `(` the other.
Two things break it:

- **Left recursion.** In `term : unary | term '*' unary`, every
  alternative begins with a `unary`, so no single token can choose.
  LAB02's loops are the repair.
- **Alternatives with a common beginning.** In LAB21, pebble gets
  assignments. `x = 1;` is an assignment and `x + 1;` is an expression,
  and both begin with the name `x`. One token cannot choose; the parser
  has to see the `=` or the `+` after it. A recursive-descent parser
  must then restructure the grammar or peek further ahead. bison
  handles both cases without help, because it postpones every decision
  until it has read a whole right-hand side (LAB16).

Notice the many "anything else" rows. The parser does not check
"anything else" on the spot. It simply takes the default and leaves the
error, if there is one, to be found later, often by a function several
calls higher up. That is the root of the next problem.

### Where hand-written parsers start to hurt

The parser works, and its messages are much better than LAB02's
`syntax error`:

```text
pebble: error: unexpected ';', expecting number, '-' or '('
```

Every message is assembled from two parts. The "unexpected" part comes
from the lookahead. The "expecting" part is a string *typed by hand* at
each place that calls `syntax_error()`. Look closely at the results:

- **Messages can be incomplete.** For `(1 2;`, the parser says
  `unexpected number, expecting ')'`. But after `(1`, an operator would
  have been fine too: `(1 + 2)` is valid. The message comes from
  `expect (')')` in `parse_factor()`, which knows only what *it*
  wanted. The functions that would have taken a `+` or a `*`
  (`parse_expr()`, `parse_term()`) have already given up and returned,
  under "anything else".
- **The knowledge is spread across functions.** `parse_factor()` lists
  `'-'` as expected, although it never accepts a `-` itself. It does so
  because it is always called through `parse_unary()`, which would have
  taken one. That is an assumption about the *callers*, written down in
  a string, and nothing checks it. Exercise 2 changes the callers and
  makes the message lie.
- **Growth.** LAB02 had five rule functions; this lab has seven. Each
  new precedence level costs a function, changes to the functions
  around it, the grammar comment, the expected lists and the tests.
  Exercise 3 counts the cost of one more level. The finished pebble has
  about a dozen levels and many kinds of statement. Written by hand,
  that is a large program that must agree, line by line, with a grammar
  that exists only as a comment.

A parser generator turns this around: you write the grammar, and the
tool writes the parser. The grammar can no longer drift from the code,
because it *is* the input, and the tool computes the expected tokens at
every point from the grammar itself (LAB27, LAB30). That is the step
LAB15 takes.

## The new code, line by line

### `src/scanner.c`

```c
        case ';':
        case '^':
            return TRUE;
```

`^` becomes a one-character token, so the scanner returns its character
code, 94, as its kind (LAB01's convention).

```c
void
pb_append_char (GString *out,
                int      c)
...
void
pb_append_token (GString *out,
                 int      kind)
```

These two functions are LAB01's `print_char()` and the body of
`print_tokens()`, moved out of `main.c` and changed to append to a
`GString` instead of printing. `pb_append_token()` writes a token the
way `--tokens` shows it: `NUM(12)`, `'+'`, `UNDEF('@')`, `EOF`. For
`NUM` and `UNDEF` it reads `pb_scanner_text`, so it can only describe
the token that `pb_scanner_next()` returned last. That is always the
case where it is used: in `--tokens`, and for the lookahead in
`--trace`.

### `src/parser.c`: the two new levels

```c
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
```

- Both alternatives begin with `factor`, so the function reads the
  factor first and decides afterwards, like LAB02's loops. But it uses
  `if`, not `while`: after `factor '^' unary`, any further `^` belongs
  to the right operand and has already been read by the recursive call.
- `pow()` comes from `<math.h>`. On Linux it lives in a separate
  library, `libm`, which is why the build gained `m_dep` (below).
- `trace_enter()` and `trace_leave()` are explained in the next part.
  Ignore them for now; every rule function begins and ends with them.

```c
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
```

Here the alternatives *do* begin with different tokens: `-` starts
`'-' unary`, and anything else must be a `power`. So the function
decides at once, before reading anything, exactly as `parse_factor()`
does. `--2` takes the first branch twice and gives `-(-2) = 2`.

`parse_term()` now calls `parse_unary()` where it called
`parse_factor()`, because the operands of `*` and `/` are now `unary`s.
That one change slots the two new levels in between `term` and
`factor`.

```c
        default:
            /* parse_unary() would have taken a '-', so it is valid too. */
            syntax_error ("number, '-' or '('");
```

`parse_factor()`'s error case. The comment records the assumption
discussed in the theory corner.

### `src/parser.c`: messages

```c
static void
append_lookahead (GString *out)
```

This function names the lookahead the way the messages do: `number`,
`end of input`, `character '@'`, or the quoted character for
operators. These names differ from the `--tokens` names on purpose:
messages are for people. bison builds its messages in the same way,
from names you give the tokens (LAB27).

```c
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
```

- The message is built in a `GString`, then printed in one piece.
- It is freed by hand, just before `longjmp()`. The obvious GLib way,
  `g_autoptr (GString) message`, would leak here: `g_autoptr` frees a
  variable when the function *returns*, through a compiler cleanup
  attribute, and `longjmp()` leaves the function without returning, so
  no cleanup runs. valgrind confirms that the version above leaks
  nothing.
- The format, `error: unexpected X, expecting Y`, is the one pebble
  keeps for the rest of the course. LAB08 replaces `pebble:` with the
  file, line and column.

```c
static void
expect (int c)
{
    char expected[] = { '\'', (char) c, '\'', '\0' };

    if (lookahead != c)
        syntax_error (expected);
    advance ();
}
```

`expect()` is only ever used for one-character tokens (`')'` and
`';'`), so it can build the "expecting" text, such as `')'`, in a small
array on the stack. No memory to free, which matters because
`syntax_error()` never returns.

### `src/parser.c`: the trace

```c
gboolean pb_parser_trace = FALSE;

static int depth;
```

`main.c` sets `pb_parser_trace` for `--trace`. `depth` counts how many
rule functions are active; the trace indents by it.

```c
static void
trace_enter (const char *rule)
{
    if (pb_parser_trace)
        g_printerr ("%*s%s {\n", 2 * depth, "", rule);
    depth++;
}

static double
trace_leave (double value)
{
    char text[G_ASCII_DTOSTR_BUF_SIZE];

    depth--;
    if (pb_parser_trace)
        g_printerr ("%*s} = %s\n", 2 * depth, "", number_text (text, value));
    return value;
}
```

- `%*s` with the arguments `2 * depth, ""` prints an empty string
  padded to `2 * depth` spaces: a cheap way to indent.
- A rule function prints `name {` when it starts and `} = value` when it
  returns, like a block of C code. Everything printed in between
  happened inside that rule.
- `trace_leave()` returns its argument, so a rule function can end with
  `return trace_leave (value);`.
- The trace goes to stderr, like the error messages, so it never mixes
  with the program's output on stdout.
- After a syntax error, `longjmp()` skips the `trace_leave()` calls, so
  `pb_parse()` resets `depth` to 0 before each parse.

```c
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
```

`advance()` uses up the lookahead, so this is where the trace prints
each token: at the moment it is used, inside the rule that uses it.
`g_autoptr` is safe here, because `advance()` never jumps away.

```c
    lookahead = pb_scanner_next ();
    while (lookahead != TOK_EOF)
        parse_statement ();
```

`pb_parse()` used to call `advance()` to read the first token. Now
`advance()` means "use up the lookahead and print it", and at the start
there is nothing to use up yet, so `pb_parse()` calls the scanner
directly.

### `src/main.c` and the build

- A second `GOptionEntry`, `--trace`, sets `pb_parser_trace`.
- `print_tokens()` builds each line in a `GString` with
  `pb_append_token()` and prints it after a `;` or at the end. The
  output is the same as before.
- `meson.build` finds the math library:
  `m_dep = meson.get_compiler('c').find_library('m', required: false)`.
  `required: false` because on some systems `pow()` is in the C library
  itself and there is no separate `libm`. `libpebble` and everything
  that links it depend on `m_dep`.

## New syntax

No new flex or bison syntax: the grammar uses LAB02's notation.

## Walkthrough

From `LAB03/`:

```sh
meson setup _build && meson compile -C _build
meson test -C _build --print-errorlogs
./_build/src/pebble examples/power.pb
```

```text
1/2 pebble:scanner OK              0.01s   6 subtests passed
2/2 pebble:parser  OK              0.03s   18 subtests passed
...
512
-4
4
0.25
-6
3
2
```

`power.pb` holds `2 ^ 3 ^ 2;`, `-2 ^ 2;`, `(-2) ^ 2;`, `2 ^ -2;`,
`-2 * 3;`, `1 - -2;` and `--2;`: one line for each case in the theory
corner.

Watch the parser build the tree for `-2 ^ 2`:

```sh
echo '-2 ^ 2;' | ./_build/src/pebble --trace
```

```text
statement {
  expr {
    term {
      unary {
        '-'
        unary {
          power {
            factor {
              NUM(2)
            } = 2
            '^'
            unary {
              power {
                factor {
                  NUM(2)
                } = 2
              } = 2
            } = 2
          } = 4
        } = 4
      } = -4
    } = -4
  } = -4
  ';'
} = -4
-4
```

Read it as a tree turned on its side, root at the top left. The outer
`unary` uses up the `'-'`, then calls `unary` again for its operand.
That inner `unary` holds the whole `power`, `2 ^ 2`, which returns 4,
and only then is the minus applied: `-4`. Each `} = value` line is a
function returning; the values climb towards the root exactly as in
LAB02's hand-computed trees.

The error messages:

```sh
for s in '1 + ;' '(1 + 2;' '1 2;' '1 + 2' '1 @ 2;'; do
    echo "$s" | ./_build/src/pebble
done
```

```text
pebble: error: unexpected ';', expecting number, '-' or '('
pebble: error: unexpected ';', expecting ')'
pebble: error: unexpected number, expecting ';'
pebble: error: unexpected end of input, expecting ';'
pebble: error: unexpected character '@', expecting ';'
```

Compare them with LAB02's five identical `pebble: syntax error` lines.
`--tokens` shows the new token:

```sh
./_build/src/pebble --tokens examples/power.pb | head -2
```

```text
NUM(2) '^' NUM(3) '^' NUM(2) ';'
'-' NUM(2) '^' NUM(2) ';'
```

The second line shows the scanner's view: a plain `'-'`, with nothing
to say whether it is negation or subtraction.

## Exercises

1. **Break it: left recursion, literally.** Replace `parse_expr()` with
   a version that follows `expr : expr '+' term` word for word: call
   `parse_expr()`, then `expect ('+')`, then `parse_term()`. Rebuild and
   run `echo '1 + 2;' | ./_build/src/pebble`. Then run it with
   `--trace 2>&1 | head` and look at the indentation.
2. **Break it: `^` from the left.** Make `^` left-associative by
   mistake: turn the `if` in `parse_power()` into a `while`, and read
   the right operand with `parse_factor()`. Compare `2 ^ 3 ^ 2;` before
   and after. Then run `2 ^ -2;` and read the error message word by
   word.
3. **Write it: comparisons.** Add `<` and `>` as a new lowest level,
   below `expr`: `comparison : expr | expr '<' expr | expr '>' expr`,
   giving 1 for true and 0 for false. A statement and the inside of
   parentheses become a `comparison`. Count the functions and lines you
   add or change, then run `meson test -C _build --print-errorlogs` and
   explain the failure. What does `1 < 2 < 3;` do? (LAB05 adds the full
   set, `<=` and `==` included, with flex.)
4. **Predict, then verify.** Predict the output of
   `echo '2 * -3 ^ 2; - - 2 ^ 2; 2 ^ 3 * 2; -(2 ^ 2) ^ 2;' |
   ./_build/src/pebble`, then run it.
5. **Inspect: associativity in the trace.** Keep only the token lines of
   two traces:

   ```sh
   echo '8 - 3 - 2;' | ./_build/src/pebble --trace 2>&1 >/dev/null |
       grep -v -E '\{$|\} = '
   echo '2 ^ 3 ^ 2;' | ./_build/src/pebble --trace 2>&1 >/dev/null |
       grep -v -E '\{$|\} = '
   ```

   Compare the indentation of the operators. Then count the trace lines
   for `7;` and explain the number.
6. **Write it: a fuller message.** For `1 2;` the parser says
   `expecting ';'`, but an operator would also have been accepted. Make
   `parse_statement()` report every token that could follow a complete
   expression. Then try `(1 2;`. Where else would you need the same
   list, and what happens to it if you also did exercise 3?

## Check your understanding

1. `unary : '-' unary` refers to itself. Why does `parse_unary()` not
   call itself forever?
2. Why is `-2 ^ 2` equal to -4, not 4, with this grammar?
3. In `1 - -2`, how does the parser know that the first `-` subtracts
   and the second one negates?
4. What does each part of "LL(1)" stand for? Which rule of LAB02's
   grammar is not LL(1) as written?
5. Why does `syntax_error()` free its message by hand instead of using
   `g_autoptr`?

## Compare with

- **Python** writes the same levels with other names. In CPython's
  grammar file (`Grammar/python.gram`), simplified:

  ```text
  factor: '+' factor | '-' factor | '~' factor | power
  power:  await_primary '**' factor | await_primary
  ```

  Python's `factor` is this lab's `unary`, and its `**` is our `^`.
  The right operand of `**` is a `factor`, which may start with a
  minus; the left one is a primary, which may not. So in Python, too,
  `-2 ** 2` is -4 and `2 ** -2` is 0.25.
- **Lua's** hand-written parser (`lparser.c`, function `subexpr()`)
  handles all binary levels in one function, with a table of priorities
  instead of one function per level. In that table, `^` has a higher
  priority on its left than on its right, which makes it
  right-associative, and unary operators sit just below `^`, so `-2^2`
  is -4 there as well. This technique, precedence climbing, is listed
  under "Look up later" in LAB02.
- **Hand-written front ends are still common.** GCC's C parser
  (`c-parser.c`, `c-parser.cc` in recent releases) and Clang's are
  hand-written recursive descent, chosen for control over error
  messages and recovery. gawk and bash pair hand-written *scanners*
  with yacc grammars (LAB47).

## Look up later

- LL(k) and ANTLR -- top-down parsing with more lookahead, and a
  parser generator built on it. This course uses bison's bottom-up
  parsers instead.
- Left factoring -- rewriting `a : x y | x z` as `a : x rest` with
  `rest : y | z`, so that one token can choose again.
- PEG and packrat parsing -- the technique behind Python's parser since
  3.9.

## Answers

**1.** The replacement:

```c
/* expr : expr '+' term, followed literally */
static double
parse_expr (void)
{
    double left;

    trace_enter ("expr");
    left = parse_expr ();
    expect ('+');
    return trace_leave (left + parse_term ());
}
```

The run ends with `Segmentation fault (core dumped)` and exit status
139. The trace shows why:

```text
statement {
  expr {
    expr {
      expr {
        expr {
```

Every line is a new `parse_expr()` call, one level deeper, and no token
is ever used up between them. With a sanitizer build
(`-Db_sanitize=address`), the crash is reported by name:
`AddressSanitizer: stack-overflow ... in parse_expr`.

**2.** The changed function:

```c
/* power : factor | power '^' factor  (left-associative, by mistake) */
static double
parse_power (void)
{
    double value;

    trace_enter ("power");
    value = parse_factor ();
    while (lookahead == '^') {
        advance ();
        value = pow (value, parse_factor ());
    }
    return trace_leave (value);
}
```

`2 ^ 3 ^ 2` becomes `(2 ^ 3) ^ 2 = 64` instead of 512. And `2 ^ -2;`
now fails with:

```text
pebble: error: unexpected '-', expecting number, '-' or '('
```

It rejects a `-` while listing `-` as acceptable. The right operand of
`^` is now read by `parse_factor()` directly, but the message in
`parse_factor()` still assumes that `parse_unary()` has just turned
down a `-`. The change broke that assumption, and nothing warned you:
not the compiler, not the existing tests. That is the "knowledge spread
across functions" problem from the theory corner, caught in the act.

**3.** In `scanner.c`, two new operators:

```c
        case '^':
        case '<':
        case '>':
            return TRUE;
```

In `parser.c`, a forward declaration
`static double parse_comparison (void);`, the new function, and the two
callers switched from `parse_expr()` to `parse_comparison()` (in the
`'('` case of `parse_factor()` and in `parse_statement()`):

```c
/* comparison : expr | expr '<' expr | expr '>' expr */
static double
parse_comparison (void)
{
    double value;

    trace_enter ("comparison");
    value = parse_expr ();
    if (lookahead == '<' || lookahead == '>') {
        int op = lookahead;
        double right;

        advance ();
        right = parse_expr ();
        value = op == '<' ? value < right : value > right;
    }
    return trace_leave (value);
}
```

That is one new function and about 21 added and 2 changed lines in two
files, plus four lines of grammar comment. `1 < 2;` prints 1,
`1 + 2 < 2 * 2;` prints 1 (both sides are `expr`s, computed first), and
`(1 < 2) + 1;` prints 2.

`meson test` fails in `/parser/trace`: every trace now has one more
level, `comparison { ... }`, so the expected trace of `-7;` no longer
matches. One more level changes every trace, and every test that pins
one down.

`1 < 2 < 3;` gives `unexpected '<', expecting ';'`. The rule uses an
`if`, not a `while`, and both operands are `expr`s, which cannot
contain a `<`. So a second `<` can only be rejected: the comparison is
non-associative, for free.

**4.** `-18`, `4`, `16`, `-16`.

- `2 * -3 ^ 2`: the operand of `*` is a `unary`; the minus applies to
  `3 ^ 2 = 9`, giving -9, and `2 * -9 = -18`.
- `- - 2 ^ 2`: two nested `unary`s around `2 ^ 2 = 4`: `-(-4) = 4`.
- `2 ^ 3 * 2`: `^` binds tighter than `*`: `8 * 2 = 16`.
- `-(2 ^ 2) ^ 2`: the parentheses make `(2 ^ 2)` a factor, the left
  operand of the second `^`: `4 ^ 2 = 16`, then the minus: -16.

**5.** The token lines only:

```text
            NUM(8)                  NUM(2)
    '-'                           '^'
            NUM(3)                      NUM(3)
    '-'                               '^'
            NUM(2)                          NUM(2)
  ';'                     ';'
```

(Shown side by side here; each command prints one column.) Both `'-'`
tokens stand in the same column: the loop in `parse_expr()` handles
them at one depth, one after the other. Each `'^'` stands further to
the right than the one before: each is handled one recursive call
deeper, inside the right operand of the previous one. Same depth means
left to right; deeper means inside.

The trace of `7;` has 14 lines: `statement`, `expr`, `term`, `unary`,
`power` and `factor` each print an opening and a closing line (12),
plus `NUM(7)` and `';'`. A lone number passes through every level: the
price of layering, now one level higher than in LAB02.

**6.** In `parse_statement()`:

```c
    value = parse_expr ();
    if (lookahead != ';')
        syntax_error ("'+', '-', '*', '/', '^' or ';'");
    advance ();
```

`1 2;` now says
`unexpected number, expecting '+', '-', '*', '/', '^' or ';'`. But
`(1 2;` still says `expecting ')'`: the same list would be needed in
the `'('` case of `parse_factor()`. And after exercise 3, both lists
would be wrong until you added `'<'` and `'>'` to them by hand. Nothing
connects these strings to the grammar, so nothing tells you.

**Questions.**

1. `parse_unary()` uses up the `-` before it calls itself, so each call
   starts one token further on. The input is finite, so the calls must
   end.
2. The left operand of `^` is a `factor`, which cannot begin with `-`.
   So the minus must apply to the whole `2 ^ 2`: `-(2 ^ 2)`.
3. By position. After a complete operand, `parse_expr()`'s loop sees the
   `-` and treats it as subtraction. Where an operand must begin,
   `parse_unary()` sees it and treats it as negation.
4. Left to right, Leftmost derivation, 1 token of lookahead. Any
   left-recursive rule, such as `expr : expr '+' term`, is not LL(1) as
   written: all of its alternatives begin the same way.
5. `g_autoptr` frees a variable when the function returns, and
   `syntax_error()` never returns: `longjmp()` leaves it without running
   any cleanup. The message would leak.
