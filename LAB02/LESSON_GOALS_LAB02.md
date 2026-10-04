# LAB02 -- Grammars and a recursive-descent parser

> You write pebble's syntax down as a grammar, in the notation bison
> reads, and turn that grammar into a parser by hand: one C function per
> rule.

Prerequisites: gdb (optional, for exercise 7). Nothing else is new.

## What changed since LAB01

- `src/parser.c`, `src/parser.h` (new): a recursive-descent parser that
  computes each statement's value while it parses; `pb_parse()`.
- `src/main.c`: without `--tokens`, `pebble` now runs the program. Exit
  code 2 after a syntax error.
- `src/meson.build`: `parser.c` joins `libpebble`.
- `tests/test-parser.c` (new) and `tests/meson.build`: parser tests.
- `examples/precedence.pb`, `examples/tree.pb` (new).
- `meson.build`: version `1.2.0`.

## The idea

LAB01's scanner turns characters into tokens, but it accepts anything:
`1 + ;` and `) 2 (` come out as tokens just as happily as `1 + 2;`.
Two questions are still open:

- **Which token sequences are programs?** `1 + 2;` is one, `1 + ;` is
  not.
- **What is their structure?** In `1 + 2 * 3`, the `*` must be done
  first: the structure is `1 + (2 * 3)`, not `(1 + 2) * 3`.

Both answers come from a **grammar**: a short, exact description of
the syntax. You have read many of them. The syntax boxes in Oracle's
SQL Language Reference and the synopsis lines in PostgreSQL's
documentation are grammars, drawn or written for people. This lab
writes pebble's grammar in the notation bison reads (bison takes it
almost unchanged in LAB15):

```yacc
program
    : %empty
    | program statement
    ;

statement
    : expr ';'
    ;

expr
    : term
    | expr '+' term
    | expr '-' term
    ;

term
    : factor
    | term '*' factor
    | term '/' factor
    ;

factor
    : NUM
    | '(' expr ')'
    ;
```

Read aloud: "A program is nothing, or a program followed by a statement.
A statement is an expr and a semicolon. An expr is a term, or an expr,
a plus and a term..."

Then you turn the grammar into a **parser**, the second stage of the
big picture from LAB01. The technique is called *recursive descent*:
every name on the left of a rule becomes a C function, and every name
on the right becomes a call to that function. The functions return the
value of what they read, so the parser computes `7` for `1 + 2 * 3;`
while it parses, and prints it.

## New words

- **grammar** -- a set of rules that says which token sequences are
  programs and what their structure is.
- **terminal** -- a symbol that appears in the input: a token kind.
  `NUM`, `'+'` and `';'` are terminals. The name says that a
  derivation (below) *terminates* there.
- **nonterminal** -- a name for a piece of structure: `program`,
  `statement`, `expr`, `term`, `factor`. It never appears in the input;
  it stands for a sequence of symbols described by its rules.
- **rule** (or **production**) -- one way to build a nonterminal:
  `statement : expr ';'`. The nonterminal on the left is the rule's
  **left-hand side**; what follows the `:` is its **right-hand side**.
- **alternative** -- one of several right-hand sides of the same
  nonterminal, separated by `|`. `factor` has two: `NUM` and
  `'(' expr ')'`.
- **start symbol** -- the nonterminal that stands for a whole input:
  `program`. It is the first rule in the grammar.
- **BNF** (Backus-Naur Form) -- the notation for writing rules. The
  course uses bison's flavour of it, shown above.
- **context-free grammar** -- a grammar whose rules all have a single
  nonterminal on the left. A rule can then be used wherever that
  nonterminal appears, whatever surrounds it (its *context*). bison
  reads context-free grammars.
- **derivation** -- a sequence of steps that starts with the start
  symbol and, in each step, replaces one nonterminal with the
  right-hand side of one of its rules, until only terminals are left.
  If a derivation reaches a token sequence, that sequence is valid.
- **leftmost derivation** -- a derivation that always replaces the
  leftmost nonterminal first.
- **parse tree** -- a derivation drawn as a tree: each nonterminal is a
  node whose children are the symbols of the rule used; the leaves,
  read left to right, are the tokens. The **root**, the node at the top
  of the drawing, is the symbol the derivation started from: `program`
  for a whole program.
- **ambiguous grammar** -- a grammar that allows two different parse
  trees for the same input. Two trees mean two meanings.
- **precedence** -- which operator groups first. `*` has a higher
  precedence than `+`: in `1 + 2 * 3` the `2 * 3` is grouped first.
- **recursive descent** -- a parser written as one function per
  nonterminal; the functions call each other as the rules say.
- **top-down parsing** -- building the parse tree from the root down to
  the leaves, as recursive descent does. bison works the other way,
  bottom-up (LAB16).
- **syntax error** -- input that the grammar does not allow, such as
  `1 + ;`.

## Theory corner

### The notation

- A rule's left-hand side stands alone at column 0. `:` means "can be
  made of", `|` means "or", and `;` ends the rule.
- `'+'` in quotes is a terminal: the one-character token whose code is
  the character (LAB01). `NUM` in capitals is a named terminal; in C it
  is `TOK_NUM`, and from LAB15 bison adds the `TOK_` prefix for you.
- Lowercase names are nonterminals.
- `%empty` is an alternative that matches no tokens at all. The section
  on lists below shows what it is for.

### A derivation and its parse tree

Here is the leftmost derivation of `2 * (3 + 4)` from `expr` (the `NUM`s
are 2, 3 and 4). Each line replaces the leftmost nonterminal:

```text
     what we have so far                  rule applied
 0   expr
 1   term                                 expr   : term
 2   term '*' factor                      term   : term '*' factor
 3   factor '*' factor                    term   : factor
 4   NUM '*' factor                       factor : NUM
 5   NUM '*' '(' expr ')'                 factor : '(' expr ')'
 6   NUM '*' '(' expr '+' term ')'        expr   : expr '+' term
 7   NUM '*' '(' term '+' term ')'        expr   : term
 8   NUM '*' '(' factor '+' term ')'      term   : factor
 9   NUM '*' '(' NUM '+' term ')'         factor : NUM
10   NUM '*' '(' NUM '+' factor ')'       term   : factor
11   NUM '*' '(' NUM '+' NUM ')'          factor : NUM
```

Line 11 holds only terminals, and they are exactly the tokens of
`2 * (3 + 4)`, so the expression is valid. Drawn as a tree, with each
step's rule becoming a node and its children:

```text
                  expr
                   |
                  term
         __________|__________
        |          |          |
       term       '*'       factor
        |              _______|_______
      factor          |       |       |
        |            '('     expr    ')'
      NUM(2)          ________|________
                     |        |        |
                    expr     '+'      term
                     |                 |
                    term             factor
                     |                 |
                   factor            NUM(4)
                     |
                   NUM(3)
```

The tree shows the structure: the `+` sits below the `*`, so it is
done first. Computing bottom-up, the subtree `3 + 4` gives 7, and the
root gives `2 * 7 = 14`. That is exactly what the parser in this lab
does.

A word on direction. Trees in this course are drawn the way computer
scientists draw them: upside down, with the **root** at the top and the
**leaves**, the tokens, at the bottom. "Down" in a tree means towards
the tokens, and "up" means towards the root. So "computing bottom-up"
means starting at the tokens and finishing at the root.

This tree's root is `expr`, because the derivation started from `expr`:
it shows one expression, not a whole program. The tree of the complete
program `2 * (3 + 4);` has the start symbol `program` at its root, and
the tree above hangs below it:

```text
              program
             /       \
        program     statement
           |        /       \
        %empty    expr      ';'
                    |
           (the expr tree above)
```

The left `program` used the `%empty` alternative, so it covers no
tokens at all. The trees for single expressions in the rest of this
lesson leave the top part out in the same way.

### Lists: `%empty` and a rule that refers to itself

A pebble program is a list of statements: none, one, or any number.
The first rule of the grammar says so in two alternatives:

```yacc
program
    : %empty
    | program statement
    ;
```

- `%empty`: a program can be nothing at all. An empty file is a valid
  program.
- `program statement`: a program can be a shorter program with one more
  statement after it.

The second alternative names `program` itself, so it can be applied
again to the `program` it produces, and again. Each application adds
one statement, and `%empty` ends the repetition. Here is the derivation
of a program with three statements:

```text
     what we have so far                      rule applied
 0   program
 1   program statement                        program : program statement
 2   program statement statement              program : program statement
 3   program statement statement statement    program : program statement
 4   statement statement statement            program : %empty
```

Applying the second alternative *k* times gives *k* statements. Zero
statements is the shortest derivation of all: `program` becomes
`%empty` in the first step.

**Why not just write "statement, repeated"?** Plain BNF has only two
tools: a *sequence* (symbols one after another) and a *choice* (`|`).
It has no "repeat". Repetition comes from a rule that refers to itself,
which is called **recursion**. Extended BNF (EBNF) adds shorthands: `x*`
for "zero or more `x`", `x+` for "one or more", `x?` for "optional".
Many books and language references use them, and would write this rule
as `program : statement*`. It means exactly the same. bison reads plain
BNF only, so in a bison grammar every list is a recursive rule: with a
`%empty` alternative for "zero or more", or with a single item for "one
or more" (exercise 4).

**Left or right?** `program : program statement` refers to itself at
the *left* end of the alternative: **left recursion**. The mirror image,
`program : statement program`, is **right recursion** and describes the
same programs. bison handles left recursion better, and LAB20 shows
why. A recursive-descent parser cannot follow left recursion literally
(LAB03 shows what happens if it tries), so it turns the rule into a
loop. In `pb_parse()`:

```c
    while (lookahead != TOK_EOF)
        parse_statement ();
```

Zero iterations is the `%empty` alternative, and each iteration is one
more application of `program statement`.

**Why a keyword for nothing?** Older grammars leave the alternative
blank (`program : | program statement ;`) or write a comment there
(`/* empty */`). Both are legal, but a blank alternative is easy to
overlook when reading and easy to create by accident with a stray `|`.
`%empty` makes the intent visible, and bison checks it. With `-Wall`,
which the course uses from LAB15, a blank alternative draws the warning
`empty rule without %empty [-Wempty-rule]`, and `%empty` followed by a
symbol is an error (`%empty on non-empty rule`).

### Ambiguity, and layering as the cure

A parse tree is more than proof that the input is valid: it *is* the
meaning. Values are computed from the bottom of the tree upwards, so an
operator low in the tree is done first and the operator at the root is
done last. A grammar for a programming language must therefore give
every valid input exactly one tree. If it allows two, the program has
two meanings, and the grammar does not say which one is right.

Here is a shorter grammar for pebble's expressions, with a single
nonterminal `e` for every kind of expression:

```yacc
e
    : e '+' e
    | e '*' e
    | NUM
    ;
```

Read aloud: "an `e` is an `e` plus an `e`, or an `e` times an `e`, or a
number". It accepts exactly the right inputs: every expression made of
numbers, `+` and `*`, and nothing else. Its problem is not *what* it
accepts but *how*. Here are two leftmost derivations of `1 + 2 * 3`.
Both are legal, and both end in the same tokens:

```text
     derivation A                  rule applied
 0   e
 1   e '+' e                       e : e '+' e
 2   NUM '+' e                     e : NUM
 3   NUM '+' e '*' e               e : e '*' e
 4   NUM '+' NUM '*' e             e : NUM
 5   NUM '+' NUM '*' NUM           e : NUM

     derivation B                  rule applied
 0   e
 1   e '*' e                       e : e '*' e
 2   e '+' e '*' e                 e : e '+' e
 3   NUM '+' e '*' e               e : NUM
 4   NUM '+' NUM '*' e             e : NUM
 5   NUM '+' NUM '*' NUM           e : NUM
```

They part ways in step 1, which picks the rule for the root of the
tree. To see what each tree means, remember one rule: an operator can
only be computed once both of its operands have values. A number is
already a value. A subtree is not: it must be computed first. That is
why an operator lower in the tree is done earlier.

- In A the root is the `+`. Its left operand is the number `1`, and its
  right operand is the whole subtree `2 * 3`. Before the `+` can be
  done, that subtree must be computed: `2 * 3 = 6`. Only then is the
  `+` at the root done: `1 + 6 = 7`.
- In B the root is the `*`. Its left operand is the whole subtree
  `1 + 2`, and its right operand is the number `3`. Now the addition is
  the one below, so it is computed first: `1 + 2 = 3`. Then the `*` at
  the root: `3 * 3 = 9`.

Same tokens, two trees, two answers. A grammar that allows this is
**ambiguous**. Exercise 1 turns the two derivations into drawings.

The trouble is not limited to mixing operators. Add `-` in the same
style (`e : e '-' e`), and `8 - 3 - 2` also has two trees: one means
`(8 - 3) - 2`, which is 3, and the other `8 - (3 - 2)`, which is 7.

There are two cures:

1. Rewrite the grammar so that only one tree is possible. That is
   *layering*, described next, and it is what this lab does.
2. Keep the short grammar and tell the parser generator which tree to
   prefer. bison can do that with precedence declarations (LAB18).

**Layering** gives each precedence level a nonterminal of its own, and
builds each level only from the level directly below it:

| Level    | Operators  | Its operands are         | Done      |
|----------|------------|--------------------------|-----------|
| `expr`   | `+` `-`    | `term`s                  | last      |
| `term`   | `*` `/`    | `factor`s                |           |
| `factor` | (none)     | `NUM`, or `( expr )`     | first     |

The consequence is that a level cannot contain the operators of the
levels above it, except inside parentheses. A `term` is factors joined
by `*` and `/`, and a `factor` is a number or a parenthesised `expr`.
Nowhere in a term or a factor can a bare `+` appear. So every bare `+`
or `-` comes from an `expr` rule, high in the tree, and is done after
everything below it.

Follow `1 + 2 * 3` from the top and watch the choices disappear:

1. The statement rule wants an `expr`. An `expr` is a single `term`, or
   `expr '+' term`, or `expr '-' term`.
2. Could the whole input be a single `term`? No: it contains a bare
   `+`, and no term can contain one. So it is `expr '+' term`, and the
   `+` of the rule is the only `+` in the input.
3. Then everything before the `+`, the `1`, must be an `expr`, and
   everything after it, `2 * 3`, must be a `term`. Each can be built in
   exactly one way: `1` as expr, term, factor, `NUM`; `2 * 3` as
   `term '*' factor`.
4. Could the `*` be at the root instead, as in tree B? Then `1 + 2`
   would have to be the left operand of `*`, which is a `term`, and a
   term cannot contain a bare `+`. Tree B cannot be built at all.

Parentheses are the way back to the top: `factor : '(' expr ')'`.
Inside the parentheses a complete `expr` is allowed again, with any
operators. From outside, the whole group counts as one `factor`, the
level that is done first. That is how `(1 + 2) * 3` gets the grouping
of tree B: `(1 + 2)` is now a factor, and a factor may be an operand of
`*`.

The same grammar also settles operators of the same level. Take
`8 - 3 - 2` and the rule `expr : expr '-' term`. Its left operand is an
`expr`, which may contain a `-`; its right operand is a `term`, which
may not. So the root must be the *last* `-`: its right operand is `2`,
and its left operand is `8 - 3`. The tree means `(8 - 3) - 2`, which
is 3. Had the rule been written `expr : term '-' expr`, the roles would
swap and the tree would mean `8 - (3 - 2)`, which is 7. The way
operators of the same level group is called **associativity**; LAB03
covers it.

Layering has a price. Every precedence level costs a nonterminal, and
in a hand-written parser a function. Even a plain `7` passes through
all of them: `expr`, `term`, `factor`, `NUM`. pebble will gain more
levels, `^` in LAB03 and comparisons in LAB05, and the chain grows with
each one. That is one reason LAB18 replaces layering with bison's
precedence declarations.

### Top-down: how recursive descent builds the tree

There are two ways to build a parse tree while reading tokens:

- **Top-down**: start at the root, with the start symbol, and work
  down towards the tokens. The parser *predicts* what must come next
  ("a program is statements, a statement starts with an `expr`, an
  `expr` starts with a `term`...") and checks each prediction against
  the tokens as they arrive.
- **Bottom-up**: start at the leaves, with the tokens, and work up
  towards the root. The parser recognises a piece only after it has
  read all of it ("`2` is a `NUM`, so a `factor`; `2 * 7` is a
  `term`...").

Recursive descent is top-down. bison's parsers are bottom-up (LAB16).

Here is this lab's parser at work on `2 * (3 + 4);`, the expression
from the derivation above. Each row is one moment. The middle column
is the call stack, indented by depth: a row further to the right means
the parser has gone one level deeper into the tree, and a row further
to the left means it has returned from a finished subtree. The first
column is the lookahead, the token the parser is looking at, at that
moment.

```text
lookahead  call stack                    what happens
---------  ----------------------------  ------------------------------
2          pb_parse                      not end of input: a statement
2            parse_statement             statement : expr ';'
2              parse_expr                an expr starts with a term
2                parse_term              a term starts with a factor
2                  parse_factor          NUM: use it; return 2
*                parse_term              '*': read one more factor
(                  parse_factor          '(': use it; then an expr
3                    parse_expr          an expr starts with a term
3                      parse_term        a term starts with a factor
3                        parse_factor    NUM: use it; return 3
+                      parse_term        no '*' or '/': return 3
+                    parse_expr          '+': read one more term
4                      parse_term        a term starts with a factor
4                        parse_factor    NUM: use it; return 4
)                      parse_term        no '*' or '/': return 4
)                    parse_expr          no '+' or '-': return 3 + 4
)                  parse_factor          expect ')': use it; return 7
;                parse_term              2 * 7; no '*' or '/': return 14
;              parse_expr                no '+' or '-': return 14
;            parse_statement             expect ';': use it; print 14
end          pb_parse                    end of input: return 0
```

There are five things to see in this table.

**Calls are predictions.** The first four calls, `parse_statement()`,
`parse_expr()`, `parse_term()` and `parse_factor()`, all happen while
the lookahead is still `2`, before the parser has used a single token.
Each call is a claim about the input: "what comes next is a
statement", "... is an expr", "... is a term", "... is a factor". The
parser can make these claims without looking, because the grammar
leaves no choice: a statement always begins with an `expr`, an `expr`
with a `term`, a `term` with a `factor`. Each call opens a node of the
tree, from the root downwards, in nearly the same order as the leftmost
derivation (the exception is the last point below).

**Decisions look at one token.** Where the grammar does offer a choice,
the parser looks at the lookahead and at nothing else. There are only
three such places in `parser.c`:

- in `parse_factor()`, `NUM` or `(` picks the alternative;
- in `parse_term()` and `parse_expr()`, an operator or its absence
  decides whether the loop goes round again;
- in `pb_parse()`, end of input decides whether another statement
  follows.

For this grammar one token is always enough. LAB03 gives that property
its name and shows grammars where it fails.

**Tokens are used once, from left to right.** Read the lookahead column
from top to bottom: `2 * ( 3 + 4 ) ;` and the end, each token in turn.
The parser never returns to an earlier token. That is why the scanner
can hand over one token at a time and forget it.

**Nodes are opened top-down but finished bottom-up.** A function
returns only when its whole piece of the input has been read, and it
returns that piece's value. So the values flow upwards: first `3` and
`4`, then `3 + 4 = 7` in the inner `parse_expr()`, then the
parenthesised factor `7`, and last `2 * 7 = 14` in the outer
`parse_term()`. The innermost subtree is finished first, exactly as
when you compute a tree by hand from the bottom up.

**The loops decide late.** This is the subtlest point of the lab, so
here it is in small steps.

*Step 1: what `parse_term()` has to decide.* The rule for `term` has
three alternatives:

```yacc
term
    : factor
    | term '*' factor
    | term '/' factor
    ;
```

When `parse_term()` is called, the parser has already predicted that a
term comes next, but not which of the three kinds of term.
`parse_factor()` faces the same kind of question for its own two
alternatives, and answers it at once from the lookahead: a `NUM` can
only start the first alternative, a `(` only the second.

*Step 2: why one token cannot decide it.* Try the same trick in the
outer `parse_term()` for `2 * (3 + 4)`. The lookahead is `2`. Which of
the three alternatives can begin with a number?

- `factor`: yes, a factor can be a `NUM`.
- `term '*' factor`: it begins with a term, a term begins with a
  factor, and a factor can be a `NUM`. So yes.
- `term '/' factor`: yes, for the same reason.

All three can begin with `2`, so the lookahead does not help.

*Step 3: what would decide it.* The three alternatives differ in what
comes *after* their first factor: a `*` for the second, a `/` for the
third, anything else for the first. In `2 * (3 + 4)` that is the second
token, the `*`. The parser can only see it after it has read the `2`.

*Step 4: with more operators, it gets worse.* Here is the leftmost
derivation of the term `2 * 3 * 4`:

```text
 0   term
 1   term '*' factor                      term   : term '*' factor
 2   term '*' factor '*' factor           term   : term '*' factor
 3   factor '*' factor '*' factor         term   : factor
 4   NUM '*' factor '*' factor            factor : NUM
 5   NUM '*' NUM '*' factor               factor : NUM
 6   NUM '*' NUM '*' NUM                  factor : NUM
```

Before it reaches the first token, the derivation has applied
`term '*' factor` twice: once for each `*` in the input. To decide
early, a parser would have to know how many `*` are coming before it
has read any of them. A parser that reads one token at a time, from
left to right, cannot know that.

*Step 5: how the loop gets round it.* `parse_term()` does not choose an
alternative at the start. It first reads the part that all three
alternatives share, a factor, and makes the decision afterwards, once
for every operator that follows:

```c
    double value = parse_factor ();

    while (lookahead == '*' || lookahead == '/') {
        int op = lookahead;
        double right;

        advance ();
        right = parse_factor ();
        value = op == '*' ? value * right : value / right;
    }
    return value;
```

Follow it on `2 * 3 * 4;`:

1. `parse_factor()` reads the `2`. Whatever the term turns out to be,
   it starts with this factor, so reading it commits the parser to
   nothing. `value` is 2, and the lookahead is now the first `*`.
2. The `while` condition sees `*`. Only now does the parser know that
   what it has read so far is not the whole term: it is the left operand
   of a multiplication. It uses up the `*`, reads the right operand, the
   factor `3`, and multiplies. `value` is 6, the value of `2 * 3`, and
   the lookahead is the second `*`.
3. The condition sees `*` again. Everything read so far, `2 * 3`,
   becomes the left operand of another multiplication. The parser reads
   the factor `4` and multiplies: `value` is 24.
4. The lookahead is now `;`, which is neither `*` nor `/`. The term is
   finished, and `parse_term()` returns 24.

Each round of the loop takes everything read so far and puts it under
a new `term '*' factor` node, as that node's left operand. The tree
grows upwards, one level per round, and ends up exactly as the
left-recursive rule describes it:

```text
after reading the 2:

     term
      |
    factor
      |
    NUM(2)

after round 1 (2 * 3):

            term
       ______|_______
      |      |       |
     term   '*'    factor
      |              |
    factor         NUM(3)
      |
    NUM(2)

after round 2 (2 * 3 * 4):

                     term
              ________|________
             |        |        |
            term     '*'     factor
       ______|_______          |
      |      |       |       NUM(4)
     term   '*'    factor
      |              |
    factor         NUM(3)
      |
    NUM(2)
```

The first piece read, the `2`, ends up deepest, at the bottom left.
That is why `2 * 3` is computed before the `* 4`: the multiplications
group from the left.

*Step 6: why not follow the rule literally?* Written the way the rule
reads (simplified to the `*` alternative), `term : term '*' factor`
would become:

```c
static double
parse_term (void)
{
    double left = parse_term ();        /* term   */

    expect ('*');                       /* '*'    */
    return left * parse_factor ();      /* factor */
}
```

The first line calls `parse_term()` again, before using a single token.
The new call sees the same lookahead, so it makes the same call, and so
on. Nothing changes from one call to the next, so the calls never stop,
until the C stack overflows. This is "deciding early" taken literally:
the function commits to `term '*' factor` before it can know whether a
`*` will ever come. LAB03 lets you run it.

Bottom-up parsers do not have this problem. They recognise
`term '*' factor` only after reading all of it, so a rule that refers
to itself on the left suits them. That is why bison accepts this lab's
grammar exactly as it is written (LAB15).

## The new code, line by line

All of it is in `src/parser.c`. The grammar sits in a comment at the
top; each function repeats its own rule above it.

| Grammar                          | C in `parser.c`                    |
|----------------------------------|------------------------------------|
| nonterminal `term`               | function `parse_term ()`           |
| a nonterminal on the right       | a call to its function             |
| a terminal on the right          | check the lookahead, `advance ()`  |
| choosing an alternative          | `switch` or `if` on the lookahead  |
| `term : term '*' factor`         | a `while` loop                     |
| the value of what was read       | the function's return value        |

### The lookahead and the helpers

```c
static int lookahead;

static void
advance (void)
{
    lookahead = pb_scanner_next ();
}
```

The parser keeps exactly one token that it has read but not yet used:
the **lookahead** (LAB01 had one *character* of lookahead; this is one
*token*). Every decision is made by looking at it. `advance()` uses it
up and reads the next one.

```c
static jmp_buf on_error;

static G_NORETURN void
syntax_error (void)
{
    fflush (stdout);
    g_printerr ("pebble: syntax error\n");
    longjmp (on_error, 1);
}
```

When the lookahead does not fit the grammar, the parser gives up. The
error can be found ten calls deep, so `longjmp()` jumps straight back
to the `setjmp()` in `pb_parse()`, unwinding all of them at once. This
is safe here because the parser holds no memory that would need
freeing. `G_NORETURN` tells the compiler that the function never
returns, so a function that ends with `syntax_error ();` needs no
`return`. `fflush (stdout)` makes earlier output appear before the
message even when both go into one pipe.

`syntax error` is deliberately poor: it says neither where nor what.
It is also exactly the message a bison parser prints by default.
LAB03, LAB08 and LAB27 improve it step by step.

```c
static void
expect (int kind)
{
    if (lookahead != kind)
        syntax_error ();
    advance ();
}
```

A terminal in a rule: the lookahead must be that token. `expect (';')`
reads "here comes a `;`, or it is an error".

### `parse_factor()`: choosing an alternative

```c
/* factor : NUM | '(' expr ')' */
static double
parse_factor (void)
{
    double value;

    switch (lookahead) {
        case TOK_NUM:
            value = pb_scanner_value;
            advance ();
            return value;
        case '(':
            advance ();
            value = parse_expr ();
            expect (')');
            return value;
        default:
            syntax_error ();
    }
}
```

`factor` has two alternatives, and the lookahead decides which one
applies: `NUM` starts the first, `(` the second, and anything else is
an error. One token is always enough to decide, here and everywhere in
this grammar. LAB03 names that property.

The `NUM` case copies `pb_scanner_value` *before* calling `advance()`.
The next call to the scanner may overwrite it with the next number.
bison has the same problem and solves it by copying every token's
value onto its own stack (LAB16).

The `'('` case follows its alternative symbol by symbol: use up the
`(`, read an `expr` by calling `parse_expr()`, then expect the `)`.
This call back up to the top level is the *recursive* in recursive
descent.

### `parse_term()` and `parse_expr()`: the loops

```c
/* term : factor | term '*' factor | term '/' factor */
static double
parse_term (void)
{
    double value = parse_factor ();

    while (lookahead == '*' || lookahead == '/') {
        int op = lookahead;
        double right;

        advance ();
        right = parse_factor ();
        value = op == '*' ? value * right : value / right;
    }
    return value;
}
```

Read the rules for `term` from the bottom up and they say: "a term is
a factor, followed by any number of `* factor` or `/ factor`". That is
what the function does. The first factor is the value so far; each
time the lookahead is `*` or `/`, it reads one more factor and combines
it with the value so far. Combining from the left gives `8 / 4 / 2 =
(8 / 4) / 2 = 1`.

Why not follow the rule literally, `term : term '*' factor`, by calling
`parse_term()` first? Because `parse_term()` would call itself before
reading any token, and again, and again, until the stack overflows.
LAB03 lets you watch that happen. `parse_expr()` is the same function
one level up, with `+` and `-`, calling `parse_term()`.

### `parse_statement()` and `pb_parse()`

```c
/* statement : expr ';' */
static void
parse_statement (void)
{
    char text[G_ASCII_DTOSTR_BUF_SIZE];
    double value = parse_expr ();

    expect (';');
    printf ("%s\n", g_ascii_formatd (text, sizeof text, "%.10g", value));
}
```

The statement is printed only after its `;` has been seen. `%.10g`
prints up to ten significant digits (`7.428571429`) and drops a useless
`.0`. bison's own `calc` example prints the same way.
`g_ascii_formatd()` always writes a `.`, whatever the locale, just as
`g_ascii_strtod()` always reads one.

```c
/* program : %empty | program statement */
int
pb_parse (void)
{
    if (setjmp (on_error) != 0)
        return 1;

    advance ();
    while (lookahead != TOK_EOF)
        parse_statement ();
    return 0;
}
```

- `setjmp()` returns 0 when it is called. If `syntax_error()` jumps
  back later, `setjmp()` "returns" a second time, with 1, and
  `pb_parse()` returns 1.
- `advance()` reads the first token, so that a lookahead exists before
  any decision is made.
- The loop is the `program` rule: zero or more statements. Another
  statement follows exactly when the lookahead is not end of input.
- The result is 0 for success and 1 after a syntax error, the same
  contract as bison's `yyparse()`. In LAB15, where `main.c` now calls
  `pb_parse()` it will call `yyparse()`, and the check of the result
  stays as it is.

### Support code

- `src/main.c` calls `pb_parse()` unless `--tokens` is given, and turns
  a syntax error into exit code 2.
- `tests/test-parser.c` checks the printed output and the result of
  ten small programs. The parser prints to stdout, so each case runs in
  a child process: `g_test_trap_subprocess()` starts it and captures its
  output, and `g_test_trap_assert_stdout()` compares it.

## New syntax

No tool runs yet, but the grammar is written in bison's notation:

| Construct            | Meaning                                  | Tool  |
|----------------------|------------------------------------------|-------|
| `name : ... ;`       | a rule for the nonterminal `name`        | bison |
| `\|`                 | separates alternatives                   | bison |
| `%empty`             | an alternative with nothing in it        | bison |
| `'+'` in a rule      | terminal: a one-character token          | bison |
| `NUM` in a rule      | terminal: a named token kind             | bison |

## Walkthrough

From `LAB02/`:

```sh
meson setup _build && meson compile -C _build
meson test -C _build --print-errorlogs
```

```text
1/2 pebble:scanner OK              0.01s   5 subtests passed
2/2 pebble:parser  OK              0.02s   10 subtests passed
```

Running programs:

```sh
./_build/src/pebble examples/hello.pb
./_build/src/pebble examples/precedence.pb
```

```text
3
7
9
3
14
3.5
```

`precedence.pb` contains `1 + 2 * 3;`, `(1 + 2) * 3;`, `8 - 3 - 2;`,
`2 * (3 + 4);` and `7 / 2;`. The LAB01 examples run too:

```sh
./_build/src/pebble examples/tokens.pb
./_build/src/pebble examples/unknown.pb; echo "exit $?"
```

```text
15
7.428571429
1000000
pebble: syntax error
exit 2
```

`unknown.pb` is `12abc @ 3;`. The scanner returns `UNDEF('a')` after
the `12`, and the parser rejects it: a number must be followed by an
operator, `)` or `;`.

The parser computes while it parses, so statements before an error
have already been printed:

```sh
echo '1; 2; 3 +; 4;' | ./_build/src/pebble
```

```text
1
2
pebble: syntax error
```

LAB22 changes that: the whole program will be parsed before anything
runs. `--tokens` still works as in LAB01.

## Exercises

1. **Draw it: ambiguity.** Draw both parse trees of `1 + 2 * 3` for the
   ambiguous grammar `e : e '+' e | e '*' e | NUM ;`. Compute the value
   at the root of each.
2. **Predict, then verify.** Predict the output, then run it:
   `echo '8 - 3 - 2; 2 * 3 + 4 * 5; (((7))); 1 - (2 - 3);' |
   ./_build/src/pebble`.
3. **Write it: empty statements.** Allow a lone `;` as a statement that
   does nothing. Change the grammar comment first, then
   `parse_statement()`. `printf ';;1;;\n;' | ./_build/src/pebble` must
   print `1`.
4. **Write it, on paper: lists.** Write BNF rules for a list of numbers
   separated by commas, such as `1, 2, 3`, with at least one number.
   Then write a second version that also allows an empty list.
5. **Break it: swap the levels.** Make `parse_expr()` handle `*` and
   `/`, and `parse_term()` handle `+` and `-` (swap the two loop
   conditions and the two calculations). Predict `1 + 2 * 3;` and
   `2 * 3 + 1;`, then run them, and run
   `meson test -C _build --print-errorlogs`.
6. **Break it: bad input.** Feed `1 + ;`, `(1 + 2;`, `1 2;` and `1 + 2`
   (no `;`). Read the messages. For each, write down what you would
   want the message to say.
7. **Inspect: the call stack is the tree** (optional, needs gdb; LAB03's
   `--trace` shows the same without it). Stop the parser when it
   reaches the `4` in `examples/tree.pb` (`2 * (3 + 4);`):

   ```sh
   gdb -q -batch \
       -ex 'break parse_factor if lookahead == 258 && pb_scanner_value == 4' \
       -ex run -ex bt --args ./_build/src/pebble examples/tree.pb
   ```

   Read the backtrace from the bottom up and compare it with the path
   from the root of the parse tree to `NUM(4)`.

## Check your understanding

1. In `statement : expr ';'`, which symbols are terminals and which are
   nonterminals?
2. Why does `1 + 2 * 3` have only one parse tree in the layered
   grammar?
3. Why must `parse_factor()` copy `pb_scanner_value` before it calls
   `advance()`?
4. How does `pb_parse()` know whether another statement follows?
5. Why does `pb_parse()` call `advance()` before parsing anything?

## Compare with

- bison's `examples/c/calc/calc.y` (under
  `~/opt/gtk/4.24.0/install/share/doc/bison/`). Its grammar section has
  the same three levels, `expr`, `term` and `fact`, written exactly in
  this lab's notation. Statements end with a newline instead of `;`,
  the program rule is called `input`, and it prints with `%.10g`. The
  `{ ... }` after some alternatives are *actions*, C code that bison
  runs; LAB15 explains them, and ports this lab's grammar to bison.

## Look up later

- ISO/IEC 14977 -- the standardised EBNF, which writes repetition as
  `{ ... }` and options as `[ ... ]` instead of `*` and `?`.
- Precedence climbing and Pratt parsing -- hand-written alternatives
  to layering that handle many precedence levels in one function.
  Common in hand-written compilers.
- Railroad diagrams -- grammars drawn as tracks, as in Oracle's SQL
  Language Reference.

## Answers

**1.** Leaves are `NUM` tokens, shown by their values:

```text
    e   (= 7)                         e   (= 9)
  / | \                             / | \
 e '+' e                           e '*' e
 |   / | \                       / | \   |
 1  e '*' e                     e '+' e  3
    |     |                     |     |
    2     3                     1     2
```

**2.** `3`, `26`, `7`, `2`. Subtraction groups from the left
(`(8 - 3) - 2`), `*` binds tighter than `+` (`6 + 20`), parentheses
only group, and `1 - (2 - 3)` is `1 - (-1)`.

**3.** Grammar:

```yacc
statement
    : ';'
    | expr ';'
    ;
```

The lookahead decides: a `;` starts the first alternative, anything
else must be an `expr`.

```c
/* statement : ';' | expr ';' */
static void
parse_statement (void)
{
    char text[G_ASCII_DTOSTR_BUF_SIZE];
    double value;

    if (lookahead == ';') {
        advance ();
        return;
    }
    value = parse_expr ();
    expect (';');
    printf ("%s\n", g_ascii_formatd (text, sizeof text, "%.10g", value));
}
```

**4.** At least one number, then an empty list allowed:

```yacc
list
    : NUM
    | list ',' NUM
    ;

list_opt
    : %empty
    | list
    ;
```

`list` refers to itself on the left (`list ',' NUM`), like `program`.
`list_opt` adds the empty case without allowing a stray comma: `, 1`
and `1,` are still invalid. LAB20 builds pebble's lists this way.

**5.** `9` and `8`: now `+` binds tighter, so `1 + 2 * 3` is
`(1 + 2) * 3` and `2 * 3 + 1` is `2 * (3 + 1)`. The tree's shape, not
the names of the functions, decides the precedence. `meson test` fails
in `/parser/precedence`.

**6.** All four print only `pebble: syntax error`. Useful would be:
which token was wrong (`;`, `;`, `2`, end of input), where it is (line
and column), and what would have been accepted ("expected a number or
`(`", "expected `)`", "expected an operator or `;`", "expected `;`").
LAB03 adds the "expected" part, LAB08 the position, LAB27 both, in
bison.

**7.** Abridged, read from the bottom:

```text
#0  parse_factor ()      NUM(4)
#1  parse_term ()        term   : factor
#2  parse_expr ()        expr   : expr '+' term   (in the + loop)
#3  parse_factor ()      factor : '(' expr ')'
#4  parse_term ()        term   : term '*' factor (in the * loop)
#5  parse_expr ()        expr   : term
#6  parse_statement ()
#7  pb_parse ()
#8  main ()
```

From `parse_expr` (#5) up to `parse_factor` (#0) it is the path in the
tree from the root `expr` down to `NUM(4)`: expr, term, factor, expr,
term, factor. In a recursive-descent parser, the call stack at any
moment is the path from the root to the node being read.

**Questions.**

1. `';'` is a terminal; `expr` is a nonterminal.
2. The operands of `*` are factors, and a factor cannot contain an
   unparenthesised `+`. So `2 * 3` must be a term inside the `+`, and
   no other tree fits.
3. `advance()` calls the scanner, and the next `NUM` overwrites
   `pb_scanner_value`. Even when the next token is not a number, the
   parser should not rely on the value staying put.
4. It looks at the lookahead. Every statement starts with something
   other than end of input, so `lookahead != TOK_EOF` means "a statement
   follows".
5. Every decision looks at the lookahead, so there must be one before
   the first decision. That is why the scanner always runs one token
   ahead of the parser.
