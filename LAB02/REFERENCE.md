# pebble course reference (as of LAB02)

## flex

Nothing yet: flex arrives in LAB04.

## bison

- `%empty` -- an alternative with nothing in it: `program : %empty |
  program statement ;` means zero or more statements. *(LAB02)*
- `'+'` and `NUM` in a rule -- terminals: a one-character token, and a
  named token kind (`TOK_NUM` in C). *(LAB02)*
- `name : alt1 | alt2 ;` -- a rule: the nonterminal `name` can be made
  of `alt1` or `alt2`. Lowercase names are nonterminals. *(LAB02)*
- token numbers -- 0 is end of input; 1-255: a one-character token is
  its own character code (`'+'` is 43); 256 is the `error` token; 257
  is "invalid token" (`YYUNDEF`); named tokens start at 258. *(LAB01)*

## Terms

- **accepting state** -- a state of an automaton that means "a complete
  token has been read". *(LAB01)*
- **alternative** -- one of the right-hand sides of a nonterminal,
  separated by `|`. *(LAB02)*
- **ambiguous grammar** -- a grammar that allows two parse trees for one
  input: `e : e '+' e | e '*' e | NUM ;` for `1 + 2 * 3`. *(LAB02)*
- **associativity** -- how operators of the same level group:
  `8 - 3 - 2` means `(8 - 3) - 2`. More in LAB03. *(LAB02)*
- **BNF** -- Backus-Naur Form, the notation for grammar rules; the
  course uses bison's flavour. *(LAB02)*
- **bottom-up parsing** -- building the parse tree from the tokens up,
  recognising a rule after reading all of it; bison's way (LAB16).
  *(LAB02)*
- **character code** -- the number a character is stored as; `'+'` is
  43 in ASCII. *(LAB01)*
- **context-free grammar** -- a grammar whose rules each have a single
  nonterminal on the left, usable wherever it appears. *(LAB02)*
- **derivation** -- replacing nonterminals by right-hand sides, one per
  step, from the start symbol down to tokens. *(LAB02)*
- **EBNF** -- extended BNF, with shorthands such as `x*` (zero or more),
  `x+` (one or more) and `x?` (optional). bison does not read it.
  *(LAB02)*
- **end of input** -- the token (code 0, `TOK_EOF`) that tells the
  parser no more input is coming. Not the same as C's `EOF` (-1).
  *(LAB01)*
- **finite automaton** -- a machine with a fixed set of states that
  reads one character at a time and follows an arrow for each. Every
  scanner is one. *(LAB01)*
- **grammar** -- the rules that say which token sequences form valid
  programs, and what their structure is. *(LAB01, LAB02)*
- **layering** -- one nonterminal per precedence level (`expr`, `term`,
  `factor`), each built from the next one down. *(LAB02)*
- **left recursion** -- a rule that refers to itself at the left end:
  `program : program statement`. bison's preferred way to write lists.
  *(LAB02)*
- **left-hand side**, **right-hand side** -- the nonterminal before a
  rule's `:`, and the symbols after it. *(LAB02)*
- **leftmost derivation** -- a derivation that always replaces the
  leftmost nonterminal first. *(LAB02)*
- **lexeme** -- the exact characters of one token: `007` in `007;`.
  *(LAB01)*
- **lexical analysis** -- splitting characters into tokens; the
  scanner's job. *(LAB01)*
- **lookahead** -- input read but not yet used: one character in the
  scanner (LAB01), one token in the parser (LAB02). *(LAB01)*
- **nonterminal** -- a name for a piece of structure (`expr`); never in
  the input, always replaced by one of its rules. *(LAB02)*
- **parse tree** -- a derivation drawn as a tree; the leaves, read left
  to right, are the tokens. *(LAB02)*
- **parser** -- the stage after the scanner: checks that the tokens
  form a valid program and finds its structure. *(LAB01)*
- **pattern** -- the rule saying which lexemes belong to a token kind:
  "one or more digits" for `TOK_NUM`. *(LAB01)*
- **precedence** -- which operator groups first; `*` before `+`.
  *(LAB02)*
- **recursion** (in a grammar) -- a rule that refers to itself,
  directly or through other rules; BNF's only way to repeat. *(LAB02)*
- **recursive descent** -- a hand-written parser with one function per
  nonterminal, calling each other as the rules say. *(LAB02)*
- **right recursion** -- a rule that refers to itself at the right end:
  `program : statement program`. *(LAB02)*
- **root**, **leaf** -- the node at the top of a tree (for a whole
  program, the start symbol) and the nodes at the bottom (the tokens).
  Trees are drawn upside down. *(LAB02)*
- **rule** (**production**) -- one way to build a nonterminal:
  `statement : expr ';'`. *(LAB02)*
- **scanner** -- the code that turns characters into tokens; also
  called lexer, lexical analyser or tokenizer. *(LAB01)*
- **semantic value** -- the data a token carries to the parser: 7 for
  the lexeme `007`. *(LAB01)*
- **start symbol** -- the nonterminal for a whole input: `program`.
  *(LAB02)*
- **state** -- where an automaton is between two characters. In a
  hand-written scanner, a position in the code. *(LAB01)*
- **syntax error** -- input the grammar does not allow: `1 + ;`.
  *(LAB02)*
- **terminal** -- a symbol that appears in the input: a token kind
  (`NUM`, `'+'`). *(LAB02)*
- **token** -- the smallest unit of meaning in a program: `12`, `+`,
  `;`. *(LAB01)*
- **token kind** -- which sort of token it is: `TOK_NUM`, `'+'`.
  *(LAB01)*
- **top-down parsing** -- building the parse tree from the root down,
  as recursive descent does. *(LAB02)*
- **transition** -- an arrow of an automaton: "in this state, on this
  character, go to that state". *(LAB01)*
- **whitespace** -- blanks between tokens (space, tab, newline); they
  separate tokens but are not tokens. *(LAB01)*
