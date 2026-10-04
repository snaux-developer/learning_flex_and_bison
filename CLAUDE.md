# CLAUDE.md -- Parsing by Doing: flex and bison from first principles

This file is the master plan for a hands-on course on flex and bison.
Claude Code reads it before generating every lab. It fixes the ground rules,
the running example, the coding style and the content of all 47 labs (the
cap is 50). Labs are generated one at a time, on request, after the
developer approves this plan.

Contents

1. Purpose
2. Ground rules
3. Repository layout
4. How to generate a lab
5. Environment and tools
6. Coding style
7. The running example: `pebble`
8. The lab plan (LAB01-LAB47)
9. Coverage checklists
10. Deliberately out of scope
11. Tool version notes
12. Plan change log


## 1. Purpose

The developer this course is written for is a C programmer and Oracle
DBA who builds GTK 4 applications with GObject and is learning Meson in
a separate course. They have always been fascinated by flex and bison
but have no background in compiler construction.

The goal: after LAB47 the developer is comfortable with flex and bison,
and can:

- design a scanner and a grammar for a small language from scratch;
- read and fix conflicts by reading bison's reports, without guessing;
- build reentrant scanners and pure, push and multi-parser programs;
- report errors with exact locations, and recover from them;
- read real-world scanners and grammars, such as PostgreSQL's `scan.l`
  and `gram.y`, without trouble.

The method: 47 labs, each a self-contained project built with Meson. Each
lab copies the previous one and adds a few concepts by growing one running
example, a tiny programming language called **pebble**. It starts as a
four-function calculator and ends as a small scripting language with a
REPL, include files, string interpolation and a JSON loader.

The focus is flex and bison. C is the vehicle. Meson and GLib are support
tools and are never the subject of a lab.


## 2. Ground rules

These rules come from the developer. They are not negotiable.

1. **flex and bison first.** Every lab exists to teach flex or bison (or
   the theory needed to use them well). Interpreter work (evaluation,
   scopes, values) is kept as small as possible and never becomes the
   topic of a lab.
2. **First principles, slowly.** LAB01-LAB03 build a scanner and a parser
   by hand, so the developer sees what the tools automate. Difficulty
   rises gently: 2-3 concepts per lab early on, at most 4 later.
3. **Cumulative and self-contained.** `LABnn` is a full copy of `LAB(nn-1)`
   plus the changes planned for `LABnn`. Every lab configures, builds and
   tests on its own; nothing refers to another lab directory.
4. **Never remove a language feature.** pebble features are never removed.
   Scaffolding that a tool replaces *is* removed (the hand-written scanner
   in LAB04, the hand-written parser in LAB15), and the lab's
   `LESSON_GOALS_LABnn.md` says what replaced what.
5. **Explain every piece of syntax.** The developer is a beginner with
   these tools. The first time any flex or bison construct appears (a
   pattern operator, a `%` directive, `yytext`, `$$`, `@1`, a macro, an
   option), the lab explains what it means, what it does to the
   generated code, and why it is written that way. Every new or changed
   line of a `.l` or `.y` file is covered in "The new code, line by
   line" (section 4.2). Nothing in a scanner or grammar is left
   unexplained, and nothing is used before it has been explained.
6. **Theory: just enough, from zero.** The developer has no compiler
   background. Define every term in plain language the first time it
   is used (token, lexeme, grammar, nonterminal, derivation, shift,
   reduce, state, lookahead, conflict...), always with a concrete
   pebble example and, where it helps, an ASCII picture. Introduce theory when
   a lab needs it: tokens, grammars, derivations, ambiguity,
   regex-to-automaton intuition, how LR parsing shifts and reduces,
   conflicts, LALR versus IELR, GLR. No proofs, no FIRST/FOLLOW tables,
   no hand-built automata beyond small sketches.
7. **Meaningful labs, not build exercises.** Meson is plumbing. It is
   explained once (LAB01, LAB04, LAB15) and never takes more than a few
   lines of a `LESSON_GOALS_LABnn.md` after that. Every lab asks the
   developer to *write* flex or bison code themselves, not only to run,
   inspect or break the lab's code.
8. **C, with GLib helpers.** Scanners, grammars and their actions are C.
   Support code may use GLib (`GHashTable`, `GString`, `GPtrArray`,
   `g_autofree`, GTest). Keep grammar actions short: one call into a
   helper function where possible.
9. **Other tools only when they help.** Graphviz (pictures of automata),
   xsltproc (HTML reports), valgrind/ASan (memory), gcov/gcovr (coverage)
   appear only in the labs where they make a concept easier to see, and
   are always optional.
10. **Host builds.** Never mention jhbuild, Flatpak, toolbox or
    containers.
11. **80 columns** for all code, grammars, scanners, build files and
    Markdown, including this file.
12. **Style** as in section 6: GTK-like C layout, 1TBS braces, 4-space
    indentation, and fixed layouts for `.l` and `.y` files.
13. **No personal names.** The repository is public. Files refer to the
    person taking the course as "the developer" (lessons say "you"),
    never by name, and use "they" for pronouns.


## 3. Repository layout

```
<course root>/
|-- CLAUDE.md            this plan
|-- .gitignore           created together with LAB01 (contains `_build/`)
|-- LAB01/
|   |-- LESSON_GOALS_LAB01.md  what this lab teaches, exercises, answers
|   |-- REFERENCE.md           every construct and term learned so far
|   |-- meson.build
|   |-- src/                   pebble sources (.c, .h, .l, .y)
|   |-- tests/                 GTest programs and test scripts
|   `-- examples/        *.pb programs used in the walkthrough
|-- LAB02/
|-- ...
`-- LAB47/
```

- Lab directories are zero-padded (`LAB01`...`LAB47`).
- Each lab builds into its own `_build/` directory.
- Generated files (`scanner.c`, `parser.c`, `parser.h`, `parser.output`)
  live only in `_build/src/`. The developer is encouraged to read
  them.
- Compare consecutive labs with `git diff --no-index LAB16 LAB17`.
- `REFERENCE.md` is cumulative. Each lab copies it forward and adds
  what it introduced, so the latest lab always holds the complete
  reference (format in section 4.3).


## 4. How to generate a lab

The developer asks for one lab at a time, e.g. "Generate LAB07 per
CLAUDE.md".

### 4.1 Procedure

1. Read this file, the plan entry for `LABnn` (section 8), and the previous
   lab's `LESSON_GOALS_LAB(nn-1).md`.
2. `cp -a LAB(nn-1) LABnn`, then delete `LABnn/_build` and the copied
   `LABnn/LESSON_GOALS_LAB(nn-1).md`.
3. Bump the project version to `1.nn.0` (no leading zero).
4. Make the planned changes, and only those. If the plan turns out to be
   wrong (tool behaviour, a better minimal design), choose the simplest
   correct fix, update section 8, and record it in section 12.
5. Write `LABnn/LESSON_GOALS_LABnn.md` from the template in 4.2, and extend
   `LABnn/REFERENCE.md` (4.3).
6. Verify against the definition of done (4.4).
7. Report: concepts covered, files changed, any deviation from the plan.

### 4.2 LESSON_GOALS_LABnn.md template

Each lab's lesson file is named after the lab, with the two-digit
number: `LAB07/LESSON_GOALS_LAB07.md`.

Under 1000 lines is typical, not a cap: a lesson is as long as its
content needs, and is never trimmed just to save lines. The line-by-line
section makes lessons longer than in the Meson course, on purpose.
Write for a reader who knows C very well and has never studied
compilers: define every term, explain every symbol, prefer a small
example over an abstract sentence.

The developer wants clear, detailed explanations, not summaries. This
matters most in "The idea", the theory corner and the line-by-line
section:

- Explain each point in full sentences. Prefer several plain paragraphs
  to one dense one; never compress an explanation to save space.
- Work through a concrete pebble input step by step: a derivation, a
  trace or a table with one row per moment. Then say what to notice in
  it, point by point.
- When a sentence makes a claim ("the parser decides late"), show the
  moment in the example where it happens.
- Check every trace and table against the real program before writing
  it down.

```markdown
# LABnn -- <Title>

> One sentence: what flex/bison skill this lab adds.

Prerequisites: tools this lab needs beyond earlier labs (if any).

## What changed since LAB(nn-1)

- `src/parser.y`: ...      (short list: added/modified/removed files)

## The idea

Plain-language explanation of the problem this lab solves and the
idea behind the solution, *before* any code (10-30 lines). Start
from what the developer already knows. Use a concrete pebble input.

## New words

- **lookahead** -- the next token, which the parser has read but not
  yet used. In `1 + 2 * 3`, after reading `2` the lookahead is `*`.

(Every term introduced in this lab, defined in plain language with
a pebble example. Omit the section if the lab adds no term.)

## Theory corner          (in labs that introduce theory)

A concrete explanation with one worked example, step by step, with a
small table or an ASCII sketch. No proofs.

## The new code, line by line

Every new or changed line of `scanner.l` and `parser.y` (and the C
glue it touches), in order. Quote the code, then explain each part:

    %left '+' '-'
    %left '*' '/'

- `%left` declares *left-associative* operators: `1 - 2 - 3` means
  `(1 - 2) - 3`.
- Each `%left` line is one precedence level. Later lines bind
  *tighter*, so `*` binds tighter than `+`.
- Effect on the generated parser: ... (what bison does with it).

When a construct appears for the first time in the course, also
show what it becomes in the generated C (`_build/src/*.c`) if that
helps, e.g. the `case` an action turns into.

## New syntax

| Construct     | Meaning                          | Tool  |
|---------------|----------------------------------|-------|
| `%left t...`  | left-assoc. operators, one level | bison |

(Narrow table: only constructs new in this lab.)

## Walkthrough

Copy-paste commands from a clean lab directory, with abridged
expected output. Build commands stay short; the point is pebble's
behaviour, the tokens, the traces and the reports. Tests always run
as `meson test -C _build --print-errorlogs`, here and in exercises:

    meson setup _build && meson compile -C _build
    meson test -C _build --print-errorlogs
    ./_build/src/pebble --tokens examples/hello.pb

## Exercises

1. **Write it.** Add a scanner rule or grammar rule yourself, from a
   short specification (required in every lab).
2. **Predict, then verify.** ...
3. **Break it.** Introduce a specific mistake (a misordered rule, a
   missing precedence declaration), read the tool's message, fix it.
4. **Inspect.** Read the generated C, the `.output` report, a trace.

(4-8 exercises; at least one "write it", one "break it" and one
"inspect".)

## Check your understanding

3-5 short questions answerable without a computer ("Why must the
keyword rule come before the identifier rule?").

## Compare with

- bison `examples/c/lexcalc/` -- the same technique in the manual's
  own example. (Also flex manual examples, PostgreSQL, jq, ...)

## Look up later

- `%option yyclass` -- C++ scanners only.

## Answers

Answers to the exercises (with the code for every "write it") and to
the questions.
```

### 4.3 REFERENCE.md

A cumulative, alphabetical reference that grows with the course. Each
lab copies the previous `REFERENCE.md` and adds its own entries, so
the developer never has to search old labs to remember what `yyless`
or `%prec` means. Three sections:

```markdown
# pebble course reference (as of LABnn)

## flex

- `yyless(n)` -- keep the first n characters of the match; give the
  rest back to the input. *(LAB11)*

## bison

- `%prec TOKEN` -- give this rule the precedence of TOKEN. Used for
  unary minus. *(LAB18)*

## Terms

- **shift** -- push the lookahead token onto the parser's stack and
  read the next one. *(LAB16)*
```

One to three lines per entry, plain language, and the lab that
introduced it.

### 4.4 Definition of done

From the lab directory, starting with no `_build/`:

```sh
meson setup _build
meson compile -C _build
meson test -C _build --print-errorlogs
```

- bison reports **no** warnings and no conflicts, except where the lab's
  final grammar deliberately keeps them with a justified `%expect`.
  Conflicts that a lab *demonstrates* live in exercises or scratch
  files, not in the final grammar.
- flex reports no warnings (`%option warn nodefault` from LAB07 on).
- The C compiler reports no warnings, generated code included. Prefer
  flex/bison options (`nounput`, `noinput`, ...) over `-Wno-...` flags.
- From LAB29 on, `meson test` also passes in a build configured with
  `-Db_sanitize=address,undefined`.
- Every exercise in `LESSON_GOALS_LABnn.md` has been tried, where
  feasible.
- Every new or changed line of a `.l` or `.y` file is explained in "The
  new code, line by line". Every term used is defined in this lab or an
  earlier one. Every new construct and term is in `REFERENCE.md`.
- At least one exercise asks the developer to write flex or bison code,
  and the answer contains that code. (LAB01-LAB03 come before the tools: there,
  "write it" means hand-written scanner or parser code in C.)
- Each line in `*.c *.h *.l *.y *.build *.md *.pb` is 80 columns or
  fewer.
- Every pebble example in `examples/` runs and matches the walkthrough.

### 4.5 Deciding what to teach

- Teach a directive, option or macro when pebble needs it, **or** when
  real scanners and grammars use it commonly.
- Otherwise list it under "Look up later" with a few words.
- Show, don't tell: traces (`--trace`, `%option debug`), `.output`
  reports, `--tokens` and `--ast` dumps, and the generated C.
- The installed tools beat memory: check uncertain details against
  `info bison`, `info flex` and the tools' behaviour before writing.


## 5. Environment and tools

- Oracle Linux 9 host, GCC, Meson >= 1.12, Ninja, pkg-config.
- Minimum versions, checked in `meson.build`:

  | Dependency       | Minimum | Checked with                  |
  |------------------|---------|-------------------------------|
  | glib-2.0         | 2.90    | `dependency(..., version:)`   |
  | flex             | 2.6.4   | `find_program(..., version:)` |
  | bison            | 3.8.2   | `find_program(..., version:)` |

- C code sets `GLIB_VERSION_MIN_REQUIRED` and `GLIB_VERSION_MAX_ALLOWED`
  to `GLIB_VERSION_2_90` (one `add_project_arguments()` line).
- **flex 2.6.4** (the current release; OL9 ships it).
- **bison 3.8.2** (the current release). OL9 ships bison 3.7.4, which
  lacks `%header`, `--html` and location printing in traces, so the
  developer installs 3.8.2 in their usual prefix. Every grammar starts
  with `%require "3.8.2"`.
- Optional tools, with the lab that first uses them:
  - gdb (LAB02; the call stack of a hand-written parser)
  - Graphviz `dot` (LAB17; pictures of the LR automaton)
  - valgrind (LAB14), ASan/UBSan through Meson (LAB29)
  - xsltproc (LAB30; `bison --html`)
  - gcovr (LAB42; grammar coverage), clang (LAB42; optional fuzzing)
- For the reading labs (LAB45-LAB47): a PostgreSQL source checkout
  (the developer works on a PostgreSQL migration), and optionally jq
  and Mesa.


## 6. Coding style

### 6.1 C

The same as the developer's Meson course:

- Layout follows GTK 4 and Valent; braces follow the One True Brace Style
  (1TBS); 4-space indentation, never tabs; 80 columns.
- Put a space before the parenthesis of calls, macros and keywords:
  `g_hash_table_lookup (table, name)`, `if (x)`, `sizeof (int)`.
- Function definitions: return type on its own line, name at column 0,
  wrapped parameters aligned.
- One SPDX line per file: `// SPDX-License-Identifier: MIT` (in `.l` and
  `.y` files: `/* SPDX-License-Identifier: MIT */`).
- Prefixes: `Pb` (types), `pb_` (functions), `PB_` (macros and enum
  values). Token kinds use `TOK_` (section 7.2).
- Modern GLib in support code: `g_autofree`, `g_autoptr`,
  `g_clear_pointer`, `GString`, `GPtrArray`, `GHashTable`.
- `switch`: `case` and `default` labels are indented 4 spaces inside the
  `switch`, and their statements 4 more.
- Structs: no blank line after the first member. In a GObject instance
  struct that means no blank line after `parent_instance`.
- Enums: the opening and closing braces go on their own lines, like a
  struct's (including `typedef enum`).

```c
typedef enum
{
    PB_NODE_NUM,
    PB_NODE_ADD,
    PB_NODE_MUL,
} PbNodeKind;

struct _PbNode
{
    PbNodeKind  kind;
    PbLocation  loc;
    PbNode     *left;
    PbNode     *right;
};

static double
pb_eval_binary (PbNode *node)
{
    double a = pb_eval (node->left);
    double b = pb_eval (node->right);

    switch (node->kind) {
        case PB_NODE_ADD:
            return a + b;
        case PB_NODE_MUL:
            return a * b;
        default:
            g_assert_not_reached ();
    }
}
```

### 6.2 flex files (`.l`)

```lex
/* SPDX-License-Identifier: MIT */

%top{
#include "config.h"
}

%option warn nodefault noyywrap nounput noinput

%{
#include "parser.h"
%}

digit       [0-9]
ident       [A-Za-z_][A-Za-z0-9_]*

%%

{digit}+                { yylval.num = strtod (yytext, NULL);
                          return TOK_NUM; }
"print"                 { return TOK_PRINT; }
{ident}                 { yylval.str = g_strdup (yytext);
                          return TOK_IDENT; }
[ \t\r\n]+              { /* skip */ }
.                       { return pb_scan_error (yytext); }

%%
```

- This is the shape from LAB07 on; LAB04-LAB06 start with fewer options.
- `%option` lines first, then `%{ %}` code, then named definitions.
  Definition names are lowercase (`digit`, `ident`), as in PostgreSQL.
- Patterns start at column 0. Actions start at column 25 when the pattern
  fits, otherwise on the next line indented 4. Multi-line actions use
  braces, continuation lines aligned with the first statement.
- Start-condition blocks `<COMMENT>{ ... }` indent their rules by 4.
- Start condition names are UPPER_CASE (`COMMENT`, `STRING`).

### 6.3 bison files (`.y`)

```yacc
/* SPDX-License-Identifier: MIT */

%require "3.8.2"
%header
%define api.token.prefix {TOK_}
%define parse.trace

%code requires {
#include "pebble.h"
}

%token NUM IDENT
%left '+' '-'
%left '*' '/'

%%

program
    : %empty
    | program statement
    ;

expr
    : expr '+' expr             { $$ = $1 + $3; }
    | expr '*' expr             { $$ = $1 * $3; }
    | NUM
    ;

%%
```

- Order of the declarations section: `%require`, output options
  (`%header`), `%define`s, `%code` blocks, `%union` or value types,
  tokens, precedence (lowest first), `%nterm`, `%destructor` and
  `%printer`.
- A rule's left-hand side sits alone at column 0. Alternatives are
  indented 4 and start with `:` or `|`. The closing `;` stands alone,
  indented 4. Separate rules with one blank line.
- Single-line actions start at column 33 when they fit. Longer actions go
  on the next line, indented 8, with braces on their own lines:

  ```yacc
      | IDENT '=' expr
          {
              $$ = pb_node_assign (@$, $1, $3);
          }
  ```

- Use `%empty` for empty alternatives, never a bare blank alternative.
- Single-character operators are character literals (`'+'`). From LAB27,
  multi-character tokens and keywords get string aliases
  (`%token LE "<="`, `%token PRINT "print"`).

### 6.4 meson.build

The Meson side stays small and identical in shape across labs. Meson
itself is taught in the developer's other course; here it is
explained once (LAB04 for flex, LAB15 for bison) and then reused.

```meson
glib_dep = dependency('glib-2.0', version: '>= 2.90')
flex_prog = find_program('flex', version: '>= 2.6.4')
bison_prog = find_program('bison', version: '>= 3.8.2')

scanner_c = custom_target(
    'scanner',
    input: 'scanner.l',
    output: 'scanner.c',
    command: [flex_prog, '-o', '@OUTPUT@', '@INPUT@'],
)

parser_ch = custom_target(
    'parser',
    input: 'parser.y',
    output: ['parser.c', 'parser.h', 'parser.output'],
    command: [
        bison_prog, '-Wall', '--report=all',
        '--report-file=@OUTPUT2@', '-o', '@OUTPUT0@', '@INPUT@',
    ],
)
```

- `%header` in the grammar makes bison write `parser.h` next to
  `parser.c`; the custom target declares it as its second output.
- The pebble sources (everything except `main.c`) build into a static
  library `libpebble` used by the `pebble` executable and by the tests.
- `meson format` style: 4 spaces, 80 columns (`meson.format` in each lab).
- GTest programs are registered with `protocol: 'tap'`, so Meson reports
  each test case as a subtest (GLib 2.90 prints TAP by default).
- One project option, from LAB23: `cex` (boolean, default `false`) adds
  `-Wcounterexamples` to the bison command.

### 6.5 Markdown

- 80 columns, ATX headings, fenced code blocks with a language tag
  (`c`, `lex`, `yacc`, `meson`, `sh`, `text`).
- Tables must stay within 80 columns. Prefer lists when they would not.


## 7. The running example: `pebble`

### 7.1 The language at the end of the course

```text
# pebble -- a tiny scripting language        /* block comments nest */
include "lib.pb";

let greeting = "witaj";                      # UTF-8 everywhere
fn fact(n) {
    if (n <= 1) return 1; else return n * fact(n - 1);
}
let double = x => x * 2;                     # arrow functions
let xs = [1, 2, 3,];                         # lists, trailing comma
for i in 0..3 {
    print "${greeting} #${i}: ${fact(xs[i % 3] + 2)}"
}
let cfg = load_json("config.json");          # second parser
print cfg["name"], double(21), 2 ^ 3 ^ 2, -2 ^ 2
```

- Statements end with `;`. From LAB35 a newline also ends a statement
  when the line could end there (automatic semicolons).
- Values: `nil`, booleans, numbers (double), strings, lists, functions,
  and maps (from JSON only).
- Operators, lowest to highest precedence: `=` (right), `or`, `and`,
  `not`, comparisons (non-associative), `+ -`, `* / %`, unary `-`, `^`
  (right), postfix `()` and `[]`.
- Built-ins: `len`, `str`, `eval` (LAB39), `load_json` (LAB41).

### 7.2 Name registry

Every lab uses these names, so labs generated in different sessions agree.

| Item                    | Value                                     |
|-------------------------|-------------------------------------------|
| Meson project           | `pebble`, license `MIT`                   |
| Project version         | `1.<lab>.0` (LAB07 is `1.7.0`)            |
| Executable              | `pebble`                                  |
| Support library         | `libpebble` (static, internal)            |
| Source file extension   | `.pb`                                     |
| C prefixes              | `Pb` / `pb_` / `PB_`                      |
| Token kinds             | `TOK_NUM`, `TOK_IDENT`, `TOK_PRINT`, ...  |
| Single-char tokens      | the character code (`'+'`)                |
| Scanner / parser files  | `src/scanner.l`, `src/parser.y`           |
| Support files           | `src/pebble.h`, `src/diag.[ch]` (LAB08)   |
|                         | `src/value.[ch]` (LAB19)                  |
|                         | `src/ast.[ch]`, `src/eval.[ch]` (LAB22)   |
| JSON scanner / parser   | `src/json-scanner.l`, `src/json-parser.y` |
| JSON prefixes           | `json_` (flex and bison)                  |
| GLR side parser         | `src/glr/arrow.y` (LAB40)                 |
| Include search path     | `PEBBLE_PATH` (LAB34)                     |
| Build directory         | `_build/`                                 |

Command line (with the lab that adds each option):

```text
pebble [OPTION...] [FILE...]
  --tokens     print the token stream instead of running      (LAB01)
  -e CODE      run CODE instead of a file                     (LAB12)
  --trace      bison parse trace                              (LAB16)
  --ast        print the syntax tree instead of running       (LAB22)
  --check      parse only; report all syntax errors           (LAB28)
  (no FILE on a terminal: interactive REPL)                   (LAB38)
```

Exit codes: `0` success, `1` runtime error, `2` syntax or usage error.

Diagnostics (from LAB08, file name from LAB34):

```text
lib.pb:3:9: error: unexpected ')', expecting number or identifier
    3 | let x = );
      |         ^
```

### 7.3 Final directory tree (after LAB47)

```
LAB47/
|-- LESSON_GOALS_LAB47.md  REFERENCE.md
|-- meson.build  meson.options  meson.format
|-- examples/        *.pb, config.json
|-- src/
|   |-- main.c       command line, REPL driver
|   |-- pebble.h     shared types (PbLocation, PbParser context)
|   |-- scanner.l    reentrant flex scanner
|   |-- parser.y     pure, push/pull bison parser
|   |-- filter.c     token filter: automatic semicolons
|   |-- diag.[ch]    diagnostics with source line and caret
|   |-- value.[ch]   runtime values
|   |-- ast.[ch]     syntax tree, --ast printer
|   |-- eval.[ch]    tree-walking evaluator, scopes, built-ins
|   |-- json-scanner.l  json-parser.y
|   `-- glr/arrow.y  GLR side parser for arrow functions
`-- tests/           GTest programs, cases/*.pb with expected output
```

### 7.4 Feature timeline

| Lab | What pebble gains                                          |
|-----|------------------------------------------------------------|
| 01  | hand-written scanner, `--tokens`                           |
| 02  | hand-written recursive-descent calculator                  |
| 03  | `^`, unary minus, hand parser error messages               |
| 04  | flex scanner replaces the hand-written one                 |
| 05  | comparison operators                                       |
| 06  | floating-point and hex numbers                             |
| 07  | identifiers, `print` keyword, unknown-character errors     |
| 08  | line and column tracking, caret diagnostics                |
| 09  | `#` and `/* */` comments                                   |
| 10  | string literals with escapes                               |
| 11  | nested comments, `..` token                                |
| 12  | several input files, `-e CODE`                             |
| 13  | UTF-8 identifiers and strings                              |
| 14  | scanner unit tests, clean scanner shutdown                 |
| 15  | bison parser replaces the hand-written one                 |
| 16  | `--trace`                                                  |
| 17  | modulo operator `%`                                        |
| 18  | precedence declarations replace the layered grammar        |
| 19  | typed values: strings and numbers                          |
| 20  | print lists, list literals, indexing                       |
| 21  | `let`, assignment, variables                               |
| 22  | syntax tree, `--ast`, parse-then-run                       |
| 23  | `if/else`, `while`, blocks, booleans, `and/or/not`         |
| 24  | functions, calls, `return`                                 |
| 25  | function scopes opened by a mid-rule action                |
| 26  | locations on every node and runtime error                  |
| 27  | precise syntax error messages                              |
| 28  | error recovery, `--check`                                  |
| 29  | leak-free error paths, sanitizer test runs                 |
| 30  | LAC: exact "expecting ..." lists                           |
| 31  | parser context struct, organised `%code` sections          |
| 32  | reentrant scanner                                          |
| 33  | pure parser, no globals left                               |
| 34  | `include "file.pb";`, file names in diagnostics            |
| 35  | automatic semicolons (token filter)                        |
| 36  | `for x in a..b`, contextual keyword `in`                   |
| 37  | string interpolation `"${expr}"`                           |
| 38  | interactive REPL (push parser)                             |
| 39  | `eval()` built-in, expression start symbol                 |
| 40  | arrow functions; GLR side parser                           |
| 41  | `load_json()`: a second scanner and parser                 |
| 42  | golden-file tests, grammar coverage                        |
| 43  | faster scanner tables, no backing up                       |
| 44  | (no new feature) legacy-style grammar, modernised          |
| 45  | (no new feature) reading PostgreSQL's scanner              |
| 46  | (no new feature) reading PostgreSQL's grammar              |
| 47  | `match` expression, built end to end                       |


## 8. The lab plan (LAB01-LAB47)

Each entry lists:

- **Concepts** -- what `LESSON_GOALS_LABnn.md` teaches, in order.
- **Theory** -- the theory corner, when there is one.
- **Example** -- the changes to pebble.
- **Exercises** -- the minimum set (more are welcome).
- **Compare with** -- the same technique in the tools' own examples or
  in real projects.
- **Later** -- options and macros to name but not teach.

### Part I -- Before the tools (LAB01-LAB03)

#### LAB01 -- Tokens by hand

Concepts
- The big picture, in plain words: how a program text becomes a running
  program (characters -> tokens -> structure -> meaning), which part
  flex automates and which part bison automates. One ASCII diagram,
  traced with `1 + 2;`.
- Lexical analysis: characters become tokens. Token *kind* versus
  *lexeme* versus *pattern*; whitespace is skipped.
- A scanner is a loop with one character of lookahead (`getc` and
  `ungetc`); numbers are multi-character tokens.
- The yacc convention: a single-character token is its own character
  code; named tokens start at 258 (`TOK_NUM`); 0 means end of input.

Theory
- A scanner is a finite automaton: draw the states for "integer" versus
  "operator" and follow `12+3` through them.

Example
- `src/scanner.c` and `scanner.h` (`pb_scanner_next()`), `src/main.c`.
- Tokens: integers, `+ - * / ( ) ;`.
- `pebble --tokens FILE` prints `NUM(12) '+' NUM(3) ';' EOF` (one
  output line per `;`; no FILE reads stdin).
- A character that starts no token becomes `TOK_UNDEF` (257, bison's
  "invalid token"), printed as `UNDEF('@')`. LAB07 turns it into an
  error message; in LAB15 bison's header names it `TOK_YYUNDEF`.
- A minimal Meson build (library, executable, one GTest program) and the
  root `.gitignore`.

Exercises
- Add `%`. Then feed `12abc` and decide what *should* happen.
- Draw the automaton for decimal numbers with a fraction part.
- Make the scanner skip `#` comments; notice how quickly hand-written
  scanners grow.

Compare with
- bison `examples/c/rpcalc/`: its hand-written `yylex`.

#### LAB02 -- Grammars and a recursive-descent parser

Concepts
- Context-free grammars: terminals, nonterminals, productions, the start
  symbol; the BNF notation used for the rest of the course.
- Derivations and parse trees; ambiguity (`E -> E + E | E * E | num`
  gives two trees for `1+2*3`).
- Precedence by layering (`expr`, `term`, `factor`). Recursive descent:
  one C function per nonterminal, evaluating while parsing.

Theory
- Leftmost derivation of `2*(3+4)`, drawn as a parse tree.

Example
- `src/parser.c`: `program: statement*`, `statement: expr ';'`; each
  statement prints its value.
- The grammar is written in bison notation (`program : %empty | program
  statement ;`, left-recursive `expr` and `term`); the functions turn
  the left-recursive rules into `while` loops, and LAB03 explains why.
- `int pb_parse (void)` returns 0, or 1 after a syntax error (like
  `yyparse()`; `setjmp`/`longjmp` inside). The message is bison's
  default wording, `pebble: syntax error`. Values print with `%.10g`,
  as in bison's `calc` example.
- `tests/test-parser.c`: table-driven cases, each run in a child process
  (`g_test_trap_subprocess()`) to check stdout.

Exercises
- Draw both parse trees of `1+2*3` for the ambiguous grammar.
- Swap the `term` and `factor` levels and predict what breaks.
- Feed `1+;` and read the (poor) error message.

Compare with
- The bison manual's `calc` grammar, which you will reach in LAB15.

#### LAB03 -- Associativity, unary minus and lookahead by hand

Concepts
- Left recursion sends recursive descent into infinite recursion, so
  left-associative operators become loops. Right associativity (`^`)
  comes naturally from recursion.
- Unary minus and its precedence relative to `^` (`-2^2` is `-4`).
- Lookahead: every decision uses one token (LL(1) intuition). Hand-written
  parsers struggle with error messages and with growth, which is the
  motivation for generators.

Example
- `^` (right-associative) and unary `-`.
- Error messages such as `expected ')'`.
- A `--trace` option that prints function entry and exit: the call tree
  *is* the parse tree.

Exercises
- Write `expr: expr '+' term` literally as a recursive function; watch
  the stack overflow.
- Make `^` left-associative by mistake; compare `2^3^2`.
- Count the functions and lines needed to add comparison operators.

Compare with
- Hand-written front ends are still common: gawk and bash pair
  hand-written scanners with yacc grammars (LAB47).

### Part II -- flex (LAB04-LAB14)

#### LAB04 -- A first flex scanner

Concepts
- The anatomy of a `.l` file, explained line by line: the definitions
  section, `%%`, the rules section, `%%`, the user code section. A rule
  is a pattern and an action; `yytext` and `yyleng`; `yylex()` returns
  when an action returns.
- A scanner is a function: actions `return TOK_NUM;` and pass values
  through a global `yylval` (declared by hand for now).
- What flex generates and how it is built: the Meson recipe (section
  6.4), `%option noyywrap`, and `nounput`/`noinput` to silence warnings
  about unused functions.
- The default rule (echo unmatched input) and `%top{}` / `%{ %}` code
  blocks.

Example
- `src/scanner.l` replaces `scanner.c`; `--tokens` output is identical.
- The hand-written parser now calls `yylex()`.
- Walkthrough starts with a five-line "hello flex" in a scratch
  directory that only prints tokens.

Exercises
- Remove `noyywrap` and read the link error.
- Read `_build/src/scanner.c`: find your actions inside the big
  `switch`.
- Delete the rule for `;` and see the default rule echo it.

Compare with
- flex manual examples: `examples/manual/wc.lex`, `numbers.lex`.

#### LAB05 -- Regular expressions I

Concepts
- Characters and literal strings: `"<="`, escapes (`\n`, `\.`), `.`.
- Character classes: `[abc]`, ranges `[0-9]`, negation `[^"\n]`.
- Repetition and grouping: `*`, `+`, `?`, `(...)`, alternation `|`.

Example
- Multi-character operators `< <= > >= == !=`.
- The hand-written parser gains one comparison level (non-associative).

Exercises
- Write a pattern for "one or more spaces or tabs" three ways.
- Predict what `=` followed by `==` scans as; verify with `--tokens`.
- Break a class with an unescaped `]`; read flex's error.

Compare with
- The "Patterns" chapter of the flex manual (`info flex Patterns`).

#### LAB06 -- Regular expressions II: numbers and definitions

Concepts
- Named definitions (`digit [0-9]`) used as `{digit}`.
- Counted repetition `{1,8}`, POSIX classes `[[:xdigit:]]`, class
  operations `[a-z]{-}[aeiou]`, case-insensitive groups `(?i:0x)`.
- Converting lexemes to values in actions (`strtod`, `strtoll`) and
  range checks.

Example
- Numbers become doubles: `12`, `1.5`, `.5`, `2e-3`, `0x1F`.

Exercises
- Write and test the float pattern; find inputs that should *not* match.
- Overflow a hex literal; report it as a scanner error.
- Rewrite the float pattern with definitions until it reads like the
  language reference.

Compare with
- PostgreSQL `src/backend/parser/scan.l`: the `decinteger`, `numeric`
  and `real` definitions (note `_?` for digit separators like
  `1_000`).

#### LAB07 -- How flex chooses: longest match and rule order

Concepts
- The matching rules: longest match wins; on a tie, the earlier rule
  wins. Keywords before identifiers, or one identifier rule plus a
  keyword table.
- `%option nodefault` and `warn`, a final catch-all rule for unknown
  characters, and flex's "rule cannot be matched" warning.
- Flex compiles all patterns into one DFA: matching costs time
  proportional to the input, not to the number of rules.

Theory
- Regex to NFA to DFA, by intuition. Run `flex -T` on a three-rule
  scanner and read the NFA and DFA it prints. `flex -v` statistics.

Example
- Identifiers; the `print` keyword (`print expr;` statements); an error
  for unknown characters.

Exercises
- Put the identifier rule above `"print"`; read the warning; explain.
- Compare DFA sizes (`flex -v`) for keyword rules versus a keyword
  table.
- Remove the catch-all rule under `nodefault`; feed `@`.

Compare with
- PostgreSQL `scan.l`: one `{identifier}` rule plus
  `ScanKeywordLookup()` (a table) instead of one rule per keyword.

#### LAB08 -- Where am I? Lines, columns and diagnostics

Concepts
- `%option yylineno` and its cost.
- Columns with `YY_USER_ACTION` (runs before every action); tabs;
  recording the start and end of each token in a location struct.
- Diagnostics that quote the source line with a caret (`src/diag.c`).

Example
- Every token carries a location; scanner and parser errors print
  `file:line:col` and a caret.

Exercises
- Make a token span two lines (a newline inside a future string); check
  your end position.
- Measure the slowdown of `yylineno` on a large generated input.
- Move the column update out of `YY_USER_ACTION` and see what breaks.

Compare with
- bison `examples/c/lexcalc/scan.l`: location tracking in the scanner.

#### LAB09 -- Start conditions I: comments

Concepts
- Exclusive (`%x`) and inclusive (`%s`) start conditions; `BEGIN`,
  `INITIAL`, `YY_START`; rules prefixed with `<COMMENT>`.
- Why one big regex for `/* ... */` is a trap (greediness, long
  comments, line counting).
- `<<EOF>>` rules: report an unterminated comment at its opening
  location.

Example
- `#` line comments (a plain pattern) and `/* */` block comments (a start
  condition).

Exercises
- Write the "one regex" comment pattern and break it with two comments
  on one line.
- Turn `COMMENT` into an inclusive condition and explain the result.
- Report the *opening* line of an unterminated comment.

Compare with
- flex manual: the "Start Conditions" chapter's comment example.
- jq `src/lexer.l`: the `IN_COMMENT` condition.

#### LAB10 -- Start conditions II: string literals

Concepts
- A `STRING` exclusive condition that accumulates the value in a
  `GString`, one rule per escape (`\n`, `\t`, `\\`, `\"`, `\x41`).
- Errors inside tokens: unterminated strings, raw newlines, unknown
  escapes, each with a precise location.
- Returning one token for many matched pieces: the token's location runs
  from the opening to the closing quote.

Example
- `print "hello\n";` works (the hand-written parser accepts a string
  after `print`).

Exercises
- Allow raw newlines in strings; update the line count correctly.
- Add `\u{0105}` (Unicode escape) using `g_unichar_to_utf8()`.
- Forget to reset the `GString`; observe two strings merging.

Compare with
- flex manual `examples/manual/string1.lex` and `string2.lex`.
- PostgreSQL `scan.l`: `xq` and `xe` conditions with `addlit()`.

#### LAB11 -- Nesting and lookahead in the scanner

Concepts
- `%option stack`: `yy_push_state()`, `yy_pop_state()`,
  `yy_top_state()`; nested comments (or a depth counter).
- Adjusting a match: `yyless(n)` gives back characters, `yymore()`
  appends the next match; `unput()` and `input()`, and their costs.
- Trailing context `r/s` and anchors `^` and `$`.

Example
- Nested `/* /* */ */` comments.
- The `..` token, with `1..5` scanning as `NUM .. NUM` (trailing context
  keeps `1.` from becoming a float). The grammar uses it in LAB36.

Exercises
- Solve `1..5` with `yyless()` instead of trailing context; compare.
- Use `^` to recognise a `#!` line only at the start of the file.
- Find the variable-trailing-context warning and explain it.

Compare with
- PostgreSQL `scan.l`: nested comments (`xc`) and the `yyless()` trick
  around `xcstart`.
- flex manual `examples/manual/yymore.lex`.

#### LAB12 -- Input sources

Concepts
- `yyin`, end of file and `yywrap()` versus `yyrestart()` for several
  files in a row.
- Scanning strings: `yy_scan_string()`, `yy_scan_bytes()` and
  `yy_delete_buffer()`.
- `YY_INPUT` to read from any source; interactive versus batch scanners
  (`%option interactive` / `never-interactive`).

Example
- `pebble a.pb b.pb` runs files in order; `pebble -e 'print 1+2;'`.

Exercises
- Count `YY_INPUT` calls for a file and for a terminal.
- Scan a string without `yy_delete_buffer()`; find the leak.
- Implement `-e` with `YY_INPUT` instead and compare.

Compare with
- flex manual: "Multiple Input Buffers" and `examples/manual/cat.lex`.

#### LAB13 -- UTF-8 in a byte-oriented scanner

Concepts
- flex matches bytes, not characters. UTF-8 byte patterns
  (`[\xC2-\xDF][\x80-\xBF]`, ...) and `%option 8bit`.
- Validating input (`g_utf8_validate()`) and reporting the location of
  invalid bytes.
- Columns in characters, not bytes, so carets line up under `ą`.

Example
- Identifiers may contain non-ASCII letters (`zażółć = 1` becomes legal
  in LAB21); strings carry UTF-8 unchanged.

Exercises
- Feed Latin-2 bytes; read your error.
- Make carets line up under a Polish string with a mistake after it.
- Measure DFA growth (`flex -v`) from the UTF-8 patterns.

Compare with
- PostgreSQL `scan.l`: `%option 8bit`, and an `ident_start` definition
  that simply accepts any byte from `\200` to `\377`. Discuss that
  trade-off against exact UTF-8 patterns.

#### LAB14 -- Scanner hygiene: debugging, testing, cleanup

Concepts
- `%option debug` (`-d`) and `yy_flex_debug`: which rule matched what.
- Testing a scanner on its own: GTest cases that scan strings and compare
  token streams.
- `yylex_destroy()` and the buffers a non-reentrant scanner still
  allocates; checking with valgrind.

Example
- `tests/test-scanner.c` with one test per token family.
- `main.c` calls `yylex_destroy()`.

Exercises
- Run `--tokens` with `yy_flex_debug = 1`.
- Remove `yylex_destroy()` and read valgrind's report.
- Write a test that would have caught the `1..5` bug of LAB11.

Compare with
- jq `src/lexer.l` and PostgreSQL `scan.l`: `%option` lines (`warn`,
  `nodefault`) as a house style.

### Part III -- bison (LAB15-LAB30)

#### LAB15 -- A first bison parser

Concepts
- The anatomy of a `.y` file, explained line by line: the declarations
  section, `%%`, the grammar rules (how BNF from LAB02 is written in
  bison), `%%`, the epilogue. `%token`; the contract between
  `yyparse()`, `yylex()` and `yyerror()`.
- The generated header (`%header`) replaces the hand-written token
  header; `%define api.token.prefix {TOK_}` keeps the token names.
- Semantic values: `%define api.value.type {double}`, `$$`, `$1`...,
  and the default action `$$ = $1`.
- Building with bison: the Meson recipe (section 6.4), and what
  `parser.c` and `parser.h` contain.

Example
- `src/parser.y` replaces `parser.c`, porting the hand-written parser's
  layered grammar (as of LAB14: `print`, strings, comparisons, `expr`,
  `term`, `factor`) directly. The output of every example is unchanged.

Exercises
- Remove `%header` and read the compile errors in the scanner.
- Write `expr: expr '+' term` (left recursive) and note that LR parsers
  are happy with it, unlike LAB03's recursive descent.
- Find `yytranslate` and the token numbers in `parser.c`.

Compare with
- bison `examples/c/calc/` and the manual's "Infix Notation Calculator".

#### LAB16 -- Shift and reduce by hand

Concepts
- Bottom-up parsing, in plain words: read tokens left to right, keep
  them on a stack, and whenever the top of the stack matches the
  right-hand side of a rule, replace it by the rule's left-hand side.
  Contrast with LAB02's top-down recursive descent.
- The two moves: *shift* (push the next token) and *reduce* (replace a
  rule's right-hand side by its left-hand side, running the rule's
  action). Accepting means only the start symbol is left.
- The lookahead token decides between shifting and reducing. In
  `1 + 2 * 3`, after `2` the parser must *not* reduce `1 + 2` yet,
  because `*` comes next.
- Watching bison do it: `%define parse.trace`, `yydebug`,
  `pebble --trace`; reading the "Shifting", "Reducing stack by rule"
  and "Now at end of input" lines; `%printer` so values appear in the
  trace.

Theory
- Hand-simulate `1+2*3` and `(1+2)*3` on paper with a three-column
  table (stack | remaining input | action), using the layered grammar
  from LAB15. Then run `pebble --trace` and match each line of the
  trace with a row of the table. States are ignored here; they are the
  subject of LAB17.

Example
- `--trace` turns on `yydebug`; `%printer` for numbers.

Exercises
- Hand-trace `2*3+4` before running it; compare with the trace.
- Write it: add unary plus to the grammar; predict and verify the
  extra reduce steps.
- Find the moment in the trace of `1+2*3` where the parser *could*
  reduce `1+2` but shifts `*` instead, and say which token made the
  decision.

Compare with
- bison manual: "The Bison Parser Algorithm" ("Lookahead Tokens") and
  "Tracing Your Parser".

#### LAB17 -- States and items: reading the bison report

Concepts
- Items: a rule with a dot showing how much of it has been seen
  (`expr: expr . '+' term` means "an expr has been read, a `'+'` may
  come next").
- A state is the set of items the parser could be in at that moment;
  the stack of LAB16 actually holds states. How the parser knows which
  rules are possible without searching.
- Reading `parser.output` from top to bottom: the numbered grammar, the
  terminals and nonterminals, then every state with its items and
  actions (`shift, and go to state 7`, `reduce using rule 3`,
  `$default`, and the "go to" entries for nonterminals).
- Connecting the trace (`Entering state 7`) with the report; optional
  pictures with `bison --graph` and `dot -Tsvg`.

Theory
- Build state 0 by hand for a three-rule grammar (which items does the
  parser start with, and which follow from them?), then check it against
  bison's report. A small sketch only, no algorithm.

Example
- The modulo operator `%` joins `*` and `/` in the grammar and the
  scanner. The report shows which states it changes.

Exercises
- Predict the state sequence for `(1)`; verify with `--trace`.
- Find the state where `*` is shifted but `+` causes a reduce; explain
  it from its items.
- Write it: add `%` yourself in a scratch copy before reading the
  lab's version; count the states before and after.
- Draw the automaton for the three-rule grammar with `--graph`; compare
  it with your hand-built state 0.

Compare with
- bison manual: "Parser States" and "Understanding Your Parser"
  (the `calc.output` walkthrough).

#### LAB18 -- Ambiguity and precedence

Concepts
- Write the ambiguous `expr: expr '+' expr | expr '*' expr | ...` and
  read the shift/reduce conflicts it produces.
- `%left`, `%right`, `%nonassoc`, `%precedence`; the order of
  declarations sets the precedence; `%prec` for unary minus.
- How precedence resolves conflicts (look for "resolved as" in the
  report); `-Wprecedence` and useless declarations.

Theory
- One conflict in detail: the item set, the two choices, and how
  precedence picks one.

Example
- The layered grammar becomes one `expr` rule with precedence
  declarations. `^` is `%right`, comparisons are `%nonassoc`, unary
  minus uses `%prec UMINUS`.

Exercises
- Delete `%prec UMINUS`; compare `-2^2` before and after.
- Make comparisons `%left`; is `1 < 2 < 3` now legal?
- Add a useless `%left` and read the warning.

Compare with
- PostgreSQL `gram.y`: its precedence table (grep `%nonassoc`).
- jq `src/parser.y`: `%precedence FUNCDEF`, `%right '|'`.

#### LAB19 -- Typed semantic values

Concepts
- More than one value type: `%union` (classic) and
  `%define api.value.type union` (modern); `%token <type>`,
  `%nterm <type>` (formerly `%type`).
- How bison type-checks `$n` and `$$`, and the escape hatch
  `$<type>n`.
- Ownership: the scanner allocates a string; the action that consumes it
  takes ownership.

Example
- `src/value.[ch]`: a `PbValue` (number or string).
- `print "total:", 3 + 4;` prints strings and numbers.

Exercises
- Use a `$1` with the wrong type and read bison's error.
- Switch between `%union` and `api.value.type union`; diff `parser.h`.
- Leak a string on purpose; find it with valgrind.

Compare with
- bison `examples/c/calc/calc.y` (`api.value.type union`).
- PostgreSQL `gram.y`: `%union` with dozens of members.

#### LAB20 -- Lists and recursion

Concepts
- Left versus right recursion: both parse lists, but right recursion
  grows the parser stack (`YYMAXDEPTH`, "memory exhausted").
- `%empty`, separators versus terminators, optional trailing commas.
- Postfix operators: indexing `a[i]` binds tighter than any prefix
  operator.

Example
- `print a, b, c;`; list literals `[1, 2, 3,]`; indexing `xs[0]`.

Exercises
- Make the print list right recursive; parse 100 000 items; read the
  error.
- Write the list rule three ways: no trailing comma, required, optional.
- Find the states that handle `[` as a literal and as an index.

Compare with
- bison manual: "Recursive Rules" and "Memory Management".

#### LAB21 -- Variables and the symbol table

Concepts
- Identifiers in expressions; `let x = e;` declarations.
- Assignment as an expression: `%right '='` makes `a = b = 1` work.
- Semantic errors (undefined variable) versus syntax errors: different
  messages, different exit paths.

Example
- A `GHashTable` symbol table; `let`, `=`, and variable references.

Exercises
- Make `=` left-associative; what does `a = b = 1` do now?
- Use a variable before it is declared; compare the error with a syntax
  error.
- Use a Polish identifier (`zażółć`).

Compare with
- bison `examples/c/mfcalc/`: a symbol table in the manual.

#### LAB22 -- Building a syntax tree

Concepts
- Why actions are not enough: `if`, `while` and short-circuit `and`
  need *deferred* evaluation. Executing inside actions runs both
  branches.
- AST nodes built in actions (`%nterm <PbNode *>`), freed in one place.
- Parse first, run later: a syntax error now stops the program before any
  output. `pebble --ast` prints the tree.

Example
- `src/ast.[ch]` and `src/eval.[ch]`. Every statement and expression
  becomes a node; the evaluator walks the tree.

Exercises
- Before switching: try to implement `if` with actions only; explain the
  failure.
- Read the `--ast` output for `1+2*3` and compare it with LAB02's tree.
- Add a node type for `%` without touching the evaluator; see what
  breaks.

Compare with
- bison `examples/c/glr/` and `reccalc/`: values that are trees or
  results.

#### LAB23 -- Control flow and the dangling else

Concepts
- The dangling else: `if (a) if (b) x; else y;` is a shift/reduce
  conflict. Read it in the report.
- Counterexamples: `-Wcounterexamples` (Meson option `-Dcex=true`)
  prints two parses of the same input.
- Three resolutions: a precedence trick, rewriting the grammar
  (matched/unmatched statements), or `%expect 1`, and why the first two
  are preferred.

Example
- `if/else`, `while`, `{ }` blocks, `true`, `false`, `nil`, and
  `and/or/not` with short-circuit evaluation.

Exercises
- Remove the resolution; read the counterexample.
- Implement the matched/unmatched rewrite in a scratch copy; compare
  the state counts.
- Add `%expect 1`, then add a second conflict and see the build fail.

Compare with
- bison manual: "Shift/Reduce Conflicts" (the same example).

#### LAB24 -- Functions and reduce/reduce conflicts

Concepts
- Reduce/reduce conflicts: two rules can reduce the same input. The
  classic cause is two optional lists that are both empty at the same
  point.
- Why bison's default (the earlier rule wins) is dangerous, and how to
  restructure the grammar.
- Calls are postfix operators: `f(x)(y)` and `xs[0](1)`.

Example
- `fn name(params) { ... }`, `return`, calls. A minimal call frame in
  the evaluator.

Exercises
- Write `params: %empty | list; list: %empty | list ',' IDENT`; read the
  conflict and counterexample; fix it.
- Allow trailing commas in parameter lists without new conflicts.
- Find the reduce/reduce entry in `parser.output`.

Compare with
- bison manual: "Reduce/Reduce Conflicts".

#### LAB25 -- Mid-rule actions and named references

Concepts
- Mid-rule actions: an action inside a rule is a hidden empty
  nonterminal. It runs before the rest of the rule is parsed.
- They can create conflicts, because the parser must commit early.
  Typed mid-rule values (`<type>{ ... }`) and `-Wmidrule-values`.
- Named references: `expr[left] '+' expr[right]` with `$left`, `$right`
  and `$[name]`.

Example
- A mid-rule action opens a function scope before the body is parsed,
  so `return` outside a function is reported during parsing.
- Long rules switch to named references.

Exercises
- Add the same mid-rule action to two alternatives with a common
  prefix; read the conflict.
- Replace the mid-rule action with a separate nonterminal; compare.
- Use a mid-rule value without a type; read the warning.

Compare with
- bison manual: "Actions in Mid-Rule" and "Named References".

#### LAB26 -- Locations

Concepts
- `%locations`, `@$` and `@n`, `YYLTYPE`, and the default rule
  `YYLLOC_DEFAULT` that spans the right-hand side.
- The scanner fills `yylloc` (reusing LAB08's tracking).
- Locations on AST nodes, so runtime errors point at the source. 3.8
  prints locations in traces.

Example
- Every node stores its location; "division by zero" and "undefined
  variable" show a caret.

Exercises
- Print `@$` for an empty rule; explain what `YYLLOC_DEFAULT` does.
- Make a location cover only the operator in `a + b`.
- Compare `--trace` output with and without `%locations`.

Compare with
- bison `examples/c/lexcalc/`, and the manual's `ltcalc` example.

#### LAB27 -- Syntax error messages

Concepts
- `yyerror()` with locations and carets.
- `%define parse.error` `simple`, `detailed`, `verbose` and `custom`;
  `yyreport_syntax_error()` with `yypcontext_expected_tokens()`.
- Token aliases (`%token PRINT "print"`, `NUM "number"`) so messages
  read naturally; `-Wdangling-alias`.

Example
- Messages such as `unexpected ')', expecting number or identifier`,
  with the caret from LAB08.

Exercises
- Compare the same error under all four `parse.error` values.
- Remove the aliases and reread the messages.
- In `custom` mode, print at most three expected tokens.

Compare with
- bison `examples/c/bistromathic/`: `parse.error custom`.

#### LAB28 -- Error recovery

Concepts
- The `error` token: how the parser pops states until it can shift
  `error`, then discards input until it can continue.
- Recovery points (statement and block level); `yyerrok`, `yyclearin`,
  `yynerrs`.
- Actions that report semantic errors with `YYERROR`; ending a parse
  early with `YYACCEPT` and `YYABORT`.

Example
- `--check` reports *all* syntax errors in a file, not just the first.

Exercises
- Put the recovery rule at the expression level; compare cascades of
  errors.
- Remove `yyerrok`; watch errors being suppressed after recovery.
- Trace a recovery with `--trace` and find the discarded tokens.

Compare with
- bison manual: "Error Recovery" and the `calc` example's recovery
  rule. (PostgreSQL does *not* recover: it stops at the first error.)

#### LAB29 -- Memory discipline

Concepts
- `%destructor` frees values that bison discards during recovery or
  abort; `<*>` and `<>` default destructors.
- Ownership rules for strings and nodes across scanner, actions and
  recovery.
- Stack limits (`YYMAXDEPTH`, `YYINITDEPTH`) and `YYNOMEM` (3.8);
  sanitizer test runs from now on.

Example
- `%destructor` for every value type. `meson test` passes with
  `-Db_sanitize=address,undefined`.

Exercises
- Remove one `%destructor`; feed an error; read the ASan report.
- Use `%printer` and `%destructor` together; read a recovery trace.
- Shrink `YYMAXDEPTH`; trigger "memory exhausted".

Compare with
- bison `examples/c/reccalc/`: `%destructor` for allocated values.

#### LAB30 -- Beyond LALR(1): IELR, canonical LR and LAC

Concepts
- LALR(1) merges similar states, which can create "mysterious"
  conflicts; `%define lr.type ielr` or `canonical-lr` avoid them.
- Default reductions make errors appear late and expected-token lists
  incomplete; `lr.default-reduction`.
- LAC (`%define parse.lac full`) checks the lookahead before acting, so
  "expecting ..." lists are exact.

Theory
- The manual's mysterious-conflict grammar, run through all three
  `lr.type` values, comparing the reports.

Example
- pebble enables `parse.lac full`; error messages list exactly the
  tokens that could continue.

Exercises
- Find an input whose message changes with LAC.
- Compare the size of `parser.c` for each `lr.type`.
- Produce `bison --html` (needs xsltproc) and browse the automaton.

Compare with
- bison manual: "Mysterious Conflicts" and "Tuning LR".

### Part IV -- Architecture and advanced features (LAB31-LAB41)

#### LAB31 -- Organising grammar files

Concepts
- `%code requires`, `%code provides`, `%code top` and plain `%code`:
  where each lands in `parser.h` and `parser.c`.
- `%parse-param`: a `PbParser` context struct replaces globals for the
  result tree and error count.
- `%initial-action` to initialise the first location; `%expect 0` as a
  guard; `%define api.header.include`.

Example
- `src/pebble.h` holds `PbLocation` and `PbParser`; the parser fills
  `parser->tree` and `parser->n_errors`.

Exercises
- Move a type from `%code requires` to `%code`; read the error in the
  scanner.
- Find each `%code` block in the generated files.
- Parse two files one after another without resetting globals; confirm
  nothing leaks between them.

Compare with
- bison manual: "Prologue Alternatives".

#### LAB32 -- A reentrant scanner

Concepts
- `%option reentrant`: a `yyscan_t` handle, `yylex_init_extra()`,
  `yylex_destroy()`, and no globals in the scanner.
- `yyextra` with `%option extra-type` for per-scanner state (location,
  string buffer, comment depth); accessor functions `yyget_text()`.
- Generating `scanner.h` (`--header-file`) so other code can create
  scanners.

Example
- The scanner is reentrant; the (still non-pure) parser receives the
  scanner through `%lex-param`.

Exercises
- Create two scanners and interleave their tokens.
- Use `yytext` from outside an action; read the compile error and fix
  it with an accessor.
- Diff the generated code with and without `reentrant`.

Compare with
- flex manual: "Reentrant C Scanners".
- jq `src/lexer.l`: `reentrant` with `extra-type`.

#### LAB33 -- A pure parser

Concepts
- `%define api.pure full`: `yylval` and `yylloc` become parameters;
  `yyerror()` gains the location and parser arguments.
- `%param` versus `%lex-param` and `%parse-param`; flex's
  `bison-bridge` and `bison-locations` options.
- Proving there are no globals: parse two programs in two threads.

Example
- No global state is left in the scanner or the parser.
- `tests/test-threads.c` parses in parallel with `GThread`.

Exercises
- Run the thread test against LAB32's build; explain the crash.
- Find `yylval` in the pure `parser.c`; where does it live now?
- Swap `%param` for separate `%lex-param` and `%parse-param`.

Compare with
- bison `examples/c/lexcalc/` and `reccalc/` (pure and reentrant).
- PostgreSQL `gram.y`: `%parse-param` and `%lex-param` with
  `core_yyscan_t`.

#### LAB34 -- Include files

Concepts
- A buffer stack: `yypush_buffer_state()`, `yypop_buffer_state()`,
  `<<EOF>>` to return to the including file.
- Where to handle `include`: in the scanner (a start condition reads the
  file name) versus in the parser (an action asks the scanner to push).
- A custom location type (`%define api.location.type {PbLocation}`)
  that carries the file name; include cycles and search paths
  (`PEBBLE_PATH`).

Example
- `include "lib.pb";` and diagnostics such as
  `lib.pb:3:9: error: ...`.

Exercises
- Include a file twice, then make two files include each other.
- Report the include chain in an error ("included from main.pb:1").
- Move include handling from the scanner to the parser; compare.

Compare with
- flex manual: "Multiple Input Buffers" (the include example) and
  `examples/manual/pas_include.lex`.

#### LAB35 -- Token filters: automatic semicolons

Concepts
- A filter function between scanner and parser: the parser calls the
  filter, which calls the scanner.
- One token of lookahead inside the filter; inserting and merging tokens
  without making the grammar ambiguous.
- Rules for "a newline ends the statement if the line could end here"
  (as in Go), and their corner cases.

Example
- `src/filter.c`: newlines become `;` after tokens that can end a
  statement. Existing programs with explicit `;` still work.

Exercises
- Write a program whose meaning changes if the rule is wrong
  (`return` followed by a newline).
- Make the scanner return `NEWLINE` and handle it in the grammar
  instead; count the new conflicts.
- Trace a filtered token stream with `--tokens`.

Compare with
- PostgreSQL `src/backend/parser/parser.c`: `base_yylex()` turns
  `NOT` + `LIKE` into `NOT_LA` to keep the grammar LALR(1).

#### LAB36 -- Contextual keywords and lexical tie-ins

Concepts
- A contextual keyword: `in` is a keyword only in a `for` header.
- Three solutions: a reserved word, a grammar that accepts `IDENT` and
  checks the text, or a lexical tie-in (the parser sets a flag the
  scanner reads).
- Tie-ins and error recovery: resetting the flag when recovery discards
  input.

Example
- `for x in 0..10 { ... }`, using the `..` token from LAB11; `in` stays
  usable as a variable name elsewhere.

Exercises
- Implement all three solutions; compare grammar size and messages.
- Break recovery inside a `for` header; watch the flag leak.
- Make `in` fully reserved; which programs break?

Compare with
- bison manual: "Handling Context Dependencies".
- PostgreSQL `gram.y`: `unreserved_keyword` and `col_name_keyword`
  (keywords usable as identifiers, handled in the grammar).

#### LAB37 -- String interpolation

Concepts
- A scanner whose mode depends on bracket nesting: `"${a + 1} and ..."`
  needs a stack of start conditions.
- Splitting one string into tokens: `STRING_START`, `STRING_PART`,
  `INTERP_BEGIN`, `INTERP_END`, `STRING_END`.
- A grammar for interpolated strings and its AST (a concatenation
  node).

Example
- `print "${name} has ${len(xs)} items";`, with nesting
  (`"${ "${x}" }"`).

Exercises
- Put a `}` inside a string inside an interpolation; trace the states.
- Report an unterminated interpolation at the `${`.
- Compare with handling interpolation *after* scanning, as a
  second-stage parse of the string.

Compare with
- jq `src/lexer.l`: `IN_QQSTRING`, `IN_QQINTERP` and the `enter()` and
  `try_exit()` helpers.

#### LAB38 -- Push parsers and a REPL

Concepts
- `%define api.push-pull both`: `yypstate_new()`, `yypush_parse()`,
  `yypull_parse()`; the caller drives the parser one token at a time.
- `YYPUSH_MORE` means "incomplete input": the REPL shows a continuation
  prompt. `yypstate_expected_tokens()` gives hints.
- Interactive scanning: line-at-a-time input and what the scanner may
  read ahead.

Example
- With no file on a terminal, `pebble` starts a REPL (`> ` and `... `
  prompts) using `fgets`, the reentrant scanner and the push parser.
  Files still use pull parsing.

Exercises
- Type an `if` over three lines; watch the prompts.
- Print the expected tokens at the continuation prompt.
- Make the REPL survive syntax errors without restarting.

Compare with
- bison `examples/c/pushcalc/` and `bistromathic/` (which uses
  readline; pebble does not need it).

#### LAB39 -- Multiple start symbols

Concepts
- One grammar, several entry points: the scanner first returns a fake
  token (`START_PROGRAM` or `START_EXPR`) that selects the start rule.
- Parsing fragments at run time: `eval("1 + 2")` parses an expression
  from a string with a fresh scanner and parser.
- Reentrancy pays off: a parse inside a running program.

Example
- The `eval()` built-in. `pebble -e` accepts a bare expression and
  prints its value.

Exercises
- Add a `START_TYPE`-style entry that parses only a list literal.
- Call `eval()` recursively; check the sanitizer run.
- Find the fake tokens in the trace.

Compare with
- bison manual FAQ: "Multiple start-symbols".
- PostgreSQL `gram.y`: `parse_toplevel` with `MODE_TYPE_NAME`, and
  `parser.c`: `mode_token[]`.

#### LAB40 -- Beyond LALR: arrow functions and GLR

Concepts
- Unbounded lookahead: in `(a, b) => a + b` the parser cannot know it is
  reading parameters until it sees `=>`. LALR(1) reports conflicts.
- Cover grammars: parse a parenthesised list, then reinterpret it as
  parameters (and check it) when `=>` follows. This is what pebble uses.
- GLR parsing: `%glr-parser` forks on conflicts and keeps the parses that
  survive; `%expect-rr`, `%dprec` and `%merge` for real ambiguity;
  deferred actions; GLR cannot be a push parser.

Theory
- How GLR splits and merges stacks, shown with `--trace` on the side
  parser.

Example
- pebble: arrow functions (`x => x * 2`, `(a, b) => a + b`) through a
  cover grammar.
- `src/glr/arrow.y`: a small GLR parser for the naive grammar, built and
  tested alongside pebble, printing which interpretation won.

Exercises
- Add the naive rules to pebble's grammar; read the conflicts.
- Give the side parser a truly ambiguous input; resolve it with
  `%dprec`, then with `%merge`.
- Explain why pebble's main parser stays LALR (push parsing, speed,
  determinism).

Compare with
- bison `examples/c/glr/` (C++-like declarations versus expressions).

#### LAB41 -- A second language: JSON

Concepts
- Several scanners and parsers in one program: `%option prefix="json_"`
  and `%define api.prefix {json_}`, separate headers, no clashes.
- Writing a grammar from a specification (RFC 8259).
- Scanner details: `\uXXXX` escapes, surrogate pairs, UTF-8 output.

Example
- `load_json(path)` returns pebble lists and maps; maps are indexed with
  `m["key"]`.

Exercises
- Leave out the prefix on one side; read the link errors.
- Feed JSON with a trailing comma; give a JSON-specific message.
- Reuse `PbLocation` in both parsers.

Compare with
- bison manual: "Multiple Parsers in the Same Program".
- PostgreSQL: `src/backend/utils/adt/jsonpath_scan.l` and
  `jsonpath_gram.y` (a second flex/bison pair).

### Part V -- Craft (LAB42-LAB44)

#### LAB42 -- Testing a language implementation

Concepts
- Golden tests: `tests/cases/*.pb` with expected output and expected
  error messages, run by one GTest program.
- Grammar coverage: `#line` directives map generated code back to
  `parser.y`, so `gcov` shows which rules were never reduced
  (`-Db_coverage=true`).
- Optional: fuzzing the scanner and parser with clang's libFuzzer.

Example
- The test suite covers every rule and every error message.

Exercises
- Find an untested rule in the coverage report; add a case.
- Generate `--no-lines` output and see coverage lose the `.y` mapping.
- Optional: fuzz for ten minutes; fix what it finds.

Compare with
- PostgreSQL's regression tests for SQL syntax errors
  (`src/test/regress/sql/`).

#### LAB43 -- Scanner performance

Concepts
- `flex -p` (performance report) and `flex -b` (`lex.backup`):
  backing-up states, and how to remove them.
- Table options: `-Cf`, `-CF`, `-Cem`, `%option full` and `fast`, and
  their size/speed trade-off.
- Measuring: a large generated input and Meson's `benchmark()`.

Example
- pebble's scanner has no backing up, enforced by a test.

Exercises
- Add a rule that causes backing up; read `lex.backup`; fix it.
- Benchmark three table options on a 50 MB input.
- Compare a keyword table with keyword rules for speed.

Compare with
- PostgreSQL `scan.l`: the header comment requiring that `lex.backup`
  reports no backing up.

#### LAB44 -- Legacy lex and yacc

Concepts
- POSIX yacc and lex: `bison -y`, `-Wyacc`, `flex -l`; what portable
  grammars must avoid.
- Old directives you will meet: `%pure-parser`, `%name-prefix`,
  `%error-verbose`, `%defines`, `#define YYSTYPE`, `yytname`.
- `bison --update` rewrites deprecated directives.

Example
- No new feature. `examples/legacy.y` is a 1990s-style version of the
  calculator grammar; it is built, then modernised with `bison --update`.

Exercises
- Build the legacy grammar with `-Wall`; read every deprecation.
- Run `bison -y` on pebble's grammar; list what breaks and why.
- Compare `REJECT` with a rewrite that avoids it.

Compare with
- PostgreSQL `gram.y`: `%pure-parser` and `%name-prefix="base_yy"`.

### Part VI -- Reading the giants (LAB45-LAB47)

#### LAB45 -- Reading PostgreSQL's scanner

Concepts
- A production scanner: `reentrant`, `bison-bridge`, `8bit`,
  `never-interactive`, custom allocators (`noyyalloc`), a prefix, and
  `yyextra`.
- Many exclusive conditions (`xb`, `xc`, `xd`, `xq`, dollar quoting) and
  a literal buffer; locations as byte offsets.
- Keywords from a table; the no-backing-up rule.

Example
- No new feature. A guided reading. `flex -b -v` is run on
  `src/backend/parser/scan.l` directly; the developer compares its
  options with pebble's.

Exercises
- Explain how `$tag$ ... $tag$` dollar quoting is scanned.
- Confirm `lex.backup` reports no backing up.
- Find PostgreSQL's equivalent of each pebble scanner technique.

#### LAB46 -- Reading PostgreSQL's grammar

Concepts
- A very large LALR(1) grammar kept conflict-free (`%expect 0`), with a
  precedence table and keyword categories.
- Start-symbol modes (`parse_toplevel`), the `base_yylex()` lookahead
  filter, and the absence of error recovery.
- Running `bison -Wall --report=all` on `gram.y` and reading the report
  of a real grammar.

Example
- No new feature. A guided reading.

Exercises
- Count states and rules; find the biggest state.
- Trace where `NOT_LA` comes from and which conflict it avoids.
- Locate the rule for `SELECT` targets and follow it to its AST node.

#### LAB47 -- Capstone: a feature end to end

Concepts
- Designing a language feature: syntax first, then tokens, grammar,
  conflicts, AST, locations, messages, recovery and tests, in that order.
- A reading tour of other real grammars, with one question each:
  jq (`src/lexer.l`, `src/parser.y`), Mesa's GLSL preprocessor
  (`src/compiler/glsl/glcpp/`), gawk (`awkgram.y`, a hand-written scanner
  inside a yacc grammar), bash (`parse.y`) and Ruby (`parse.y`).

Example
- `match` expression:

  ```text
  print match (x) { 1 => "one", 2, 3 => "few", _ => "many" };
  ```

  The lab gives the developer a checklist; the reference solution is in
  the lab.

Exercises
- Build the feature from the checklist before reading the solution.
- Answer the reading-tour questions.


## 9. Coverage checklists

### 9.1 flex

| Topic                                         | Lab(s)      |
|-----------------------------------------------|-------------|
| Sections, rules, `yytext`, `yyleng`, `yylex`  | 04          |
| Patterns, classes, repetition, definitions    | 05, 06      |
| Longest match, rule order, `nodefault`, `-T`  | 07          |
| `yylineno`, `YY_USER_ACTION`                  | 08          |
| Start conditions, `BEGIN`, `<<EOF>>`          | 09, 10      |
| `%option stack`, `yyless`, `yymore`, `unput`  | 11          |
| Trailing context, anchors                     | 11          |
| `yyin`, `yywrap`, `yyrestart`, `yy_scan_*`    | 12          |
| `YY_INPUT`, interactive / never-interactive   | 12, 38      |
| UTF-8, `8bit`                                 | 13          |
| `debug`, `yylex_destroy`                      | 14          |
| `reentrant`, `extra-type`, accessors          | 32          |
| `bison-bridge`, `bison-locations`             | 33          |
| Buffer stack (`yypush_buffer_state`)          | 34          |
| `prefix`, `--header-file`                     | 32, 41      |
| `-p`, `-b`, `-C...`, `full`, `fast`           | 43          |
| `flex -l`, `REJECT`                           | 44          |

### 9.2 bison

| Topic                                         | Lab(s)      |
|-----------------------------------------------|-------------|
| Grammar file, `yyparse`, `yyerror`, `%header` | 15          |
| `api.value.type`, `%union`, typed symbols     | 15, 19      |
| Shift/reduce, traces, `%printer`              | 16          |
| Items, states, the `.output` report           | 17          |
| `%left` ... `%precedence`, `%prec`            | 18          |
| Recursion, `%empty`, `YYMAXDEPTH`             | 20, 29      |
| AST construction                              | 22          |
| S/R conflicts, counterexamples, `%expect`     | 23          |
| R/R conflicts                                 | 24          |
| Mid-rule actions, named references            | 25          |
| `%locations`, `YYLLOC_DEFAULT`                | 26          |
| `parse.error`, aliases, custom reporting      | 27          |
| `error`, `yyerrok`, `YYERROR`, `YYABORT`      | 28          |
| `%destructor`, `YYNOMEM`                      | 29          |
| `lr.type`, default reductions, `parse.lac`    | 30          |
| `%code`, `%parse-param`, `%initial-action`    | 31          |
| `api.pure`, `%param`, `%lex-param`            | 32, 33      |
| `api.location.type`                           | 34          |
| `api.push-pull`, `yypstate_*`                 | 38          |
| `%glr-parser`, `%dprec`, `%merge`             | 40          |
| `api.prefix`                                  | 41          |
| `--graph`, `--html`, `--xml`                  | 17, 30      |
| `-y`, legacy directives, `--update`           | 44          |

### 9.3 Theory

| Idea                                          | Lab         |
|-----------------------------------------------|-------------|
| Tokens, lexemes, patterns, finite automata    | 01          |
| Grammars, derivations, parse trees, ambiguity | 02          |
| Lookahead, left recursion, associativity      | 03          |
| Regex to NFA to DFA                           | 07          |
| Bottom-up parsing: shift, reduce, the stack   | 16          |
| LR items and states                           | 17          |
| Conflicts and precedence                      | 18, 23, 24  |
| LALR versus IELR versus LR(1), LAC            | 30          |
| GLR                                           | 40          |


## 10. Deliberately out of scope

Each item below gets at most a mention:

- bison's C++, Java and D output modes; C++ scanners from flex.
- Other generators: RE/flex, re2c, Lemon, ANTLR, Lrama (named once in
  LAB47, because Ruby uses it).
- flex serialized tables (`--tables-file`).
- Parser internationalisation (`YYENABLE_NLS`).
- Compiler back ends: type checking, optimisation, code generation.
  pebble is interpreted by a plain tree walker.
- readline and other line editors for the REPL.


## 11. Tool version notes

- flex 2.6.4 is the current release (2017).
- bison 3.8.2 is the current release (2021). Features newer than 3.7
  that the plan relies on: `%header` (replaces `%defines`), `--html`,
  `YYNOMEM`, and locations in traces. Counterexamples arrived in 3.7,
  and `parse.error detailed`/`custom` in 3.6.
- Directives renamed over the years (taught in LAB44): `%pure-parser` to
  `%define api.pure`, `%name-prefix` to `%define api.prefix`,
  `%error-verbose` to `%define parse.error verbose`, `%defines` to
  `%header`, `%type` to `%nterm` (for nonterminals).


## 12. Plan change log

Record every deviation from this plan here, newest first:

- 2026-10-04: per the developer, whose repository is public, the plan
  no longer names them; it says "the developer" throughout. New ground
  rule 13 keeps names out of all files.

- 2026-10-04: per the developer, each lab's lesson file is named after
  its lab, `LESSON_GOALS_LABnn.md`, instead of `LESSON_GOALS.md`. LAB01
  and LAB02 were renamed; step 2 of 4.1 now also deletes the copied
  lesson.

- 2026-10-04 (LAB02): per the developer, section 4.2 now asks for clear,
  detailed explanations worked through on a concrete example. LAB02's
  sections on ambiguity and top-down parsing were expanded accordingly.

- 2026-10-03 (LAB02): filled in details the plan left open (now listed
  under LAB02's Example): bison-notation grammar with loops in the C
  functions, `pb_parse()` returning 0/1 like `yyparse()`, the
  `pebble: syntax error` message, `%.10g` output, and a new
  `tests/test-parser.c`. Test sources must not pass a zero-length buffer
  to `fmemopen()`: under ASan it fails with `EINVAL`. gdb joins the
  optional tools in section 5 (one optional exercise).

- 2026-10-03 (LAB01): per the developer, tests use Meson's TAP protocol
  (`protocol: 'tap'`); added to section 6.4.

- 2026-10-03 (LAB01): per the developer, the 250-450 line figure in 4.2
  is a typical length, not a cap; lessons are not trimmed to fit it.
- 2026-10-03 (LAB01): filled in details the plan left open. Unknown
  characters become `TOK_UNDEF` (257), so the "add `%`" exercise has
  something to change and the token numbering below 258 is explained
  in full. `--tokens` breaks the line after each `;`. Token values are
  `double` from LAB01 on (the lexical syntax stays integer-only until
  LAB06), matching LAB15's `api.value.type {double}`. bison can rename
  token 0 (`%token EOF 0`, giving `TOK_EOF`) but not 257, which stays
  `TOK_YYUNDEF`. Section 4.4 now says how "write it" applies to
  LAB01-LAB03, which come before flex and bison.

- 2026-10-02 (before any lab existed): per the developer, GLib 2.90 is
  the minimum (with matching GLib version guards), and bison is pinned
  to 3.8.2 in `%require` and `find_program()`.
- 2026-10-02 (before any lab existed): per the developer, the old LAB16
  ("Inside the LR parser") was split into LAB16 (shift and reduce by hand,
  traces) and LAB17 (items, states and the `.output` report; adds the
  `%` operator). Later labs moved up by one: 47 labs.
- 2026-10-02 (before any lab existed): per the developer, who is new to
  the tools and to the theory, every lab must explain each new line of
  `.l`/`.y` code and define each new term (rules 5-7); the template
  gained "The idea", "New words", "The new code, line by line", "New
  syntax" and "Check your understanding"; each lab now carries a
  cumulative `REFERENCE.md`; every lab has a "write it" exercise.
