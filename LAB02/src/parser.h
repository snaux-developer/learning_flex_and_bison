// SPDX-License-Identifier: MIT

#pragma once

/*
 * Parse the program that pb_scanner_next() delivers and print the value
 * of each statement. Returns 0 on success and 1 after a syntax error,
 * like bison's yyparse().
 */
int pb_parse (void);
