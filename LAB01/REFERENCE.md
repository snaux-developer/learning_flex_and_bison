# pebble course reference (as of LAB01)

## flex

Nothing yet: flex arrives in LAB04.

## bison

- token numbers -- 0 is end of input; 1-255: a one-character token is
  its own character code (`'+'` is 43); 256 is the `error` token; 257
  is "invalid token" (`YYUNDEF`); named tokens start at 258. *(LAB01)*

## Terms

- **accepting state** -- a state of an automaton that means "a complete
  token has been read". *(LAB01)*
- **character code** -- the number a character is stored as; `'+'` is
  43 in ASCII. *(LAB01)*
- **end of input** -- the token (code 0, `TOK_EOF`) that tells the
  parser no more input is coming. Not the same as C's `EOF` (-1).
  *(LAB01)*
- **finite automaton** -- a machine with a fixed set of states that
  reads one character at a time and follows an arrow for each. Every
  scanner is one. *(LAB01)*
- **grammar** -- the rules that say which token sequences form valid
  programs. The first one is written in LAB02. *(LAB01)*
- **lexeme** -- the exact characters of one token: `007` in `007;`.
  *(LAB01)*
- **lexical analysis** -- splitting characters into tokens; the
  scanner's job. *(LAB01)*
- **lookahead** -- input read but not yet used. After `12` in `12+3`
  the scanner has read `+` to know the number ended. *(LAB01)*
- **parser** -- the stage after the scanner: checks that the tokens
  form a valid program and finds its structure. *(LAB01)*
- **pattern** -- the rule saying which lexemes belong to a token kind:
  "one or more digits" for `TOK_NUM`. *(LAB01)*
- **scanner** -- the code that turns characters into tokens; also
  called lexer, lexical analyser or tokenizer. *(LAB01)*
- **semantic value** -- the data a token carries to the parser: 7 for
  the lexeme `007`. *(LAB01)*
- **state** -- where an automaton is between two characters. In a
  hand-written scanner, a position in the code. *(LAB01)*
- **token** -- the smallest unit of meaning in a program: `12`, `+`,
  `;`. *(LAB01)*
- **token kind** -- which sort of token it is: `TOK_NUM`, `'+'`.
  *(LAB01)*
- **transition** -- an arrow of an automaton: "in this state, on this
  character, go to that state". *(LAB01)*
- **whitespace** -- blanks between tokens (space, tab, newline); they
  separate tokens but are not tokens. *(LAB01)*
