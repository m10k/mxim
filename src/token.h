/*
 * token.h - This file is part of mxim
 * Copyright (C) 2024-2025 Matthias Kruk
 *
 * Mxim is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 3, or (at your
 * option) any later version.
 *
 * Mxim is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mxim; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
	TOKEN_INVALID = 0,
	TOKEN_LBRACE,
        TOKEN_RBRACE,
        TOKEN_LBRACKET,
        TOKEN_RBRACKET,
        TOKEN_EQUALS,
        TOKEN_COMMA,
        TOKEN_INTEGER,
        TOKEN_STRING,
        TOKEN_IDENTIFIER,
        TOKEN_NEWLINE,
        TOKEN_COMMENT,
        TOKEN_WHITESPACE,
        TOKEN_EOF,
        TOKEN_MAX
} token_type_t;

#define token_type_valid(ttype) ((ttype) >= 0 && (ttype) < TOKEN_MAX)

typedef struct token token_t;

struct token {
	token_type_t type;
	char *lexeme;
	int line;
	int col;
};

typedef struct lexer lexer_t;

int lexer_new(lexer_t **lexer, const char *file);
int lexer_free(lexer_t **lexer);

int lexer_get_position(const lexer_t *lexer, int *line, int *col);
token_type_t lexer_next_token_type(lexer_t *lexer);
int lexer_have_token(lexer_t *lexer, ...);
token_t* lexer_get_token(lexer_t *lexer, ...);

int token_free(token_t **token);

#endif /* TOKEN_H */
