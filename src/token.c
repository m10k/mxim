/*
 * token.c - This file is part of mxim
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

#include "string.h"
#include "token.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct lexer {
	/* input file descriptor */
	int fd;

	/* the lexeme currently getting lexed */
	string_t *lexeme;

	/* current line and column */
	int line;
	int col;

	/* buffer for `lexer_have_token()' */
	token_t *next_token;
};

static const token_type_t _single_char_types[] = {
	[0]    = TOKEN_EOF,
	['{']  = TOKEN_LBRACE,
	['}']  = TOKEN_RBRACE,
	['[']  = TOKEN_LBRACKET,
	[']']  = TOKEN_RBRACKET,
	['=']  = TOKEN_EQUALS,
	[',']  = TOKEN_COMMA,
	['\n'] = TOKEN_NEWLINE
};

static const char *_token_names[] = {
        [TOKEN_LBRACE]     = "TOKEN_LBRACE",
        [TOKEN_RBRACE]     = "TOKEN_RBRACE",
        [TOKEN_LBRACKET]   = "TOKEN_LBRACKET",
        [TOKEN_RBRACKET]   = "TOKEN_RBRACKET",
        [TOKEN_EQUALS]     = "TOKEN_EQUALS",
        [TOKEN_COMMA]      = "TOKEN_COMMA",
        [TOKEN_INTEGER]    = "TOKEN_INTEGER",
        [TOKEN_STRING]     = "TOKEN_STRING",
        [TOKEN_IDENTIFIER] = "TOKEN_IDENTIFIER",
        [TOKEN_NEWLINE]    = "TOKEN_NEWLINE",
        [TOKEN_COMMENT]    = "TOKEN_COMMENT",
        [TOKEN_WHITESPACE] = "TOKEN_WHITESPACE",
        [TOKEN_EOF]        = "TOKEN_EOF",
        [TOKEN_INVALID]    = "TOKEN_INVALID"
};

static const char* _token_type_str(const token_type_t type)
{
	return _token_names[token_type_valid(type) ? type : TOKEN_INVALID];
}

static int _is_identifier_char(const char c)
{
	return (c >= 'a' && c <= 'z') ||
	       (c >= 'A' && c <= 'Z') ||
	       (c >= '0' && c <= '9') ||
	       (c == '_');
}

static int _token_new(token_t **token)
{
	token_t *t;

	if (!(t = calloc(1, sizeof(*t)))) {
		return -ENOMEM;
	}

	t->type = TOKEN_INVALID;

	*token = t;
	return 0;
}

int token_free(token_t **token)
{
	if (!token || !*token) {
		return -EINVAL;
	}

	free((*token)->lexeme);
	(*token)->lexeme = NULL;
	free(*token);
	*token = NULL;

	return 0;
}

int lexer_new(lexer_t **lexer, const char *file)
{
	lexer_t *lex;
	int err;

	lex = NULL;
	err = 0;

	if (!lexer || !file) {
		return -EINVAL;
	}

	if (!(lex = calloc(1, sizeof(*lex)))) {
		return -ENOMEM;
	}

	if ((err = string_new(&lex->lexeme)) < 0) {
		free(lex);
		return -ENOMEM;
	}

	if ((lex->fd = open(file, O_RDONLY)) < 0) {
		err = -errno;
	}

	if (!err) {
		*lexer = lex;
		lex->line = 1;
		lex->col = 1;
	} else {
		free(lex);
	}

	return err;
}

int lexer_free(lexer_t **lexer)
{
	if (!lexer) {
		return -EINVAL;
	}

	string_free(&(*lexer)->lexeme);

	if ((*lexer)->fd >= 0) {
		close((*lexer)->fd);
		(*lexer)->fd = -1;
	}

	free(*lexer);
	*lexer = NULL;

	return 0;
}

int lexer_get_position(const lexer_t *lexer, int *line, int *col)
{
	if (!lexer) {
		return -EINVAL;
	}

	if (line) {
		*line = lexer->line;
	}
	if (col) {
		*col = lexer->col;
	}

	return 0;
}


static int _lexer_get_char(lexer_t *lexer, char *dst)
{
	int err;
	char c;

	if (!lexer || !dst) {
		return -EINVAL;
	}

	if (lexer->fd < 0) {
		/* A previous backstep or read failed */
		return -EBADFD;
	}

	err = read(lexer->fd, &c, sizeof(c));

	if (err < 0) {
		err = -errno;
		close(lexer->fd);
		lexer->fd = -1;
	} else if (err > 0) {
		lexer->col++;

		if (c == '\n') {
			lexer->line++;
			lexer->col = 1;
		}

		*dst = c;
		err = 0;
	} else {
		*dst = 0;
	}

	return err;
}

static int _lexer_backstep(lexer_t *lexer)
{
	if (!lexer) {
		return -EINVAL;
	}

	if (lexer->fd < 0) {
		return -EBADFD;
	}

	if (lseek(lexer->fd, -1, SEEK_CUR) < 0) {
		/*
		 * This lexer doesn't work if we cannot revert to the
		 * previous position. No need to fail gracefully.
		 */
		close(lexer->fd);
		lexer->fd = -1;

		return -errno;
	}

	return 0;
}

static int _lexer_get_string(lexer_t *lexer, token_t *token);

static int _lexer_get_string_escape(lexer_t *lexer, token_t *token)
{
	int err;
	char cur;
	char substr[3];
	int substr_len;

	memset(substr, 0, sizeof(substr));
	substr_len = 0;

	if (!lexer || !token) {
		return -EINVAL;
	}

	if ((err = _lexer_get_char(lexer, &cur)) < 0) {
		return err;
	}

	if (cur == 0) {
		return -EBADMSG;
	}

	/*
	 * This function was called because the previous character was a
	 * backslash. So if cur is ", the input was \" and we want only the
	 * double-quote in the lexeme. Otherwise we treat the backslash as
	 * a literal backslash, so we want the backslash and whatever came
	 * after it.
	 */
	if (cur != '"') {
		substr[substr_len++] = '\\';
	}
	substr[substr_len++] = cur;

	if ((err = string_append_utf8(lexer->lexeme, substr, substr_len)) < 0) {
		return err;
	}

	return _lexer_get_string(lexer, token);
}

static int _lexer_get_string(lexer_t *lexer, token_t *token)
{
	int err;
	char cur;

	if (!lexer || !token) {
		return -EINVAL;
	}

	if ((err = _lexer_get_char(lexer, &cur)) < 0) {
		return err;
	}

	switch (cur) {
	case 0:
		err = -EBADMSG;
		break;

	case '\\':
		err = _lexer_get_string_escape(lexer, token);
		break;

	default:
		if (cur != '"') {
			if ((err = string_append_utf8(lexer->lexeme, &cur, sizeof(cur))) < 0) {
				break;
			}

			err = _lexer_get_string(lexer, token);
		} else {
			token->type = TOKEN_STRING;
			err = string_get_utf8(lexer->lexeme, &token->lexeme);

			assert(err != 0);

			if (err > 0) {
				err = 0;
			}
		}
		break;
	}

	return err;
}

static int _lexer_get_integer(lexer_t *lexer, token_t *token)
{
	int err;
	char cur;

	if (!lexer || !token) {
		return -EINVAL;
	}

	if ((err = _lexer_get_char(lexer, &cur)) < 0) {
		return err;
	}

	if (isdigit(cur)) {
		if ((err = string_append_utf8(lexer->lexeme, &cur, sizeof(cur))) > 0) {
			err = _lexer_get_integer(lexer, token);
		}
	} else {
		/* character is part of the next token */
		_lexer_backstep(lexer);

		token->type = TOKEN_INTEGER;
		err = string_get_utf8(lexer->lexeme, &token->lexeme);

		/* err really should not be zero */
		assert(err != 0);

		if (err >= 0) {
			err = 0;
		}
	}

	return err;
}

static int _lexer_get_prefixed_integer(lexer_t *lexer, token_t *token)
{
	int err;
	char cur;

	if (!lexer || !token) {
		return -EINVAL;
	}

	if ((err = _lexer_get_char(lexer, &cur)) < 0) {
		return err;
	}

	if (isdigit(cur)) {
		if ((err = string_append_utf8(lexer->lexeme, &cur, sizeof(cur))) > 0) {
			err = _lexer_get_integer(lexer, token);
		}
	} else {
		/* We already have a prefix (+/-) so we *need* a digit */
		err = -EBADMSG;
	}

	return err;
}

static int _lexer_get_comment(lexer_t *lexer, token_t *token)
{
	int err;
	char cur;

	if (!lexer || !token) {
		return -EINVAL;
	}

	if ((err = _lexer_get_char(lexer, &cur)) < 0) {
		return err;
	}

	if (cur == '\n') {
		/* end of comment */
		_lexer_backstep(lexer);
		token->type = TOKEN_COMMENT;
		err = string_get_utf8(lexer->lexeme, &token->lexeme);

		assert(err != 0);
		if (err > 0) {
			err = 0;
		}
	} else {
		err = string_append_utf8(lexer->lexeme, &cur, sizeof(cur));

		if (err == 0) {
			err = -ENOMEM;
		} else if (err > 0) {
			err = _lexer_get_comment(lexer, token);
		}
	}

	return err;
}

static int _lexer_get_whitespace(lexer_t *lexer, token_t *token)
{
	int err;
	char cur;

	if (!lexer || !token) {
		return -EINVAL;
	}

	if ((err = _lexer_get_char(lexer, &cur)) < 0) {
		return err;
	}

	if (cur != ' ' && cur != '\t') {
		_lexer_backstep(lexer);
		token->type = TOKEN_WHITESPACE;
		err = string_get_utf8(lexer->lexeme, &token->lexeme);

		assert(err != 0);
		if (err > 0) {
			err = 0;
		}
	} else {
		err = string_append_utf8(lexer->lexeme, &cur, sizeof(cur));

		if (err == 0) {
			err = -ENOMEM;
		} else if(err > 0) {
			err = _lexer_get_whitespace(lexer, token);
		}
	}

	return err;
}

static int _lexer_get_identifier(lexer_t *lexer, token_t *token)
{
	int err;
	char cur;

	if (!lexer || !token) {
		return -EINVAL;
	}

	if ((err = _lexer_get_char(lexer, &cur)) < 0) {
		return err;
	}

	if (!_is_identifier_char(cur)) {
		_lexer_backstep(lexer);
		token->type = TOKEN_IDENTIFIER;
		err = string_get_utf8(lexer->lexeme, &token->lexeme);

		assert(err != 0);

		if (err > 0) {
			err = 0;
		}
	} else {
		err = string_append_utf8(lexer->lexeme, &cur, sizeof(cur));

		if (err == 0) {
			err = -ENOMEM;
		} else if (err > 0) {
			err = _lexer_get_identifier(lexer, token);
		}
	}

	return err;
}

static int _get_next_token(lexer_t *lexer, token_t **token)
{
	int err;
	char first_char;
	token_t *tok;

	if (!lexer || !token) {
		return -EINVAL;
	}

	if ((err = string_clear(lexer->lexeme)) < 0) {
		return err;
	}

	if ((err = _token_new(&tok)) < 0) {
		return err;
	}

	tok->line = lexer->line;
	tok->col = lexer->col;

	if ((err = _lexer_get_char(lexer, &first_char)) < 0) {
		return err;
	}

	switch (first_char) {
	case 0:
	case '{':
	case '}':
	case '[':
	case ']':
	case '=':
	case ',':
	case '\n':
		/* single-character token */
		if ((err = string_append_utf8(lexer->lexeme, &first_char, sizeof(first_char))) < 0) {
			return err;
		}

		tok->type = _single_char_types[(int)first_char];
		if ((err = string_get_utf8(lexer->lexeme, &tok->lexeme)) > 0) {
			err = 0;
		}
		break;

	case '"':
		err = _lexer_get_string(lexer, tok);
		break;

	case '-':
	case '+':
		if ((err = string_append_utf8(lexer->lexeme, &first_char, sizeof(first_char))) < 0) {
			return err;
		}

		err = _lexer_get_prefixed_integer(lexer, tok);
		break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if ((err = string_append_utf8(lexer->lexeme, &first_char, sizeof(first_char))) < 0) {
			return err;
		}

		err = _lexer_get_integer(lexer, tok);
		break;

	case '#':
		if ((err = string_append_utf8(lexer->lexeme, &first_char, sizeof(first_char))) < 0) {
			return err;
		}

		err = _lexer_get_comment(lexer, tok);
		break;

	case ' ':
	case '\t':
		if ((err = string_append_utf8(lexer->lexeme, &first_char, sizeof(first_char))) < 0) {
			return err;
		}

		err = _lexer_get_whitespace(lexer, tok);
		break;

	default:
		if (!_is_identifier_char(first_char)) {
			err = -EBADMSG;
		} else {
			if ((err = string_append_utf8(lexer->lexeme, &first_char, sizeof(first_char))) < 0) {
				return err;
			}

			err = _lexer_get_identifier(lexer, tok);
		}
		break;
	}

	if (!err) {
		*token = tok;
	} else {
		token_free(&tok);
	}

	return err;
}

static token_t* _get_next_relevant_token(lexer_t *lexer)
{
	token_t *token;

	token = NULL;

	while (!token && _get_next_token(lexer, &token) == 0) {
		if (token->type == TOKEN_EOF) {
			token_free(&token);
			break;
		}

		if (token->type == TOKEN_COMMENT    ||
		    token->type == TOKEN_WHITESPACE ||
		    token->type == TOKEN_NEWLINE) {
			token_free(&token);
			continue;
		}
	}

	return token;
}

static int _have_token_with_type(lexer_t *lexer, va_list types)
{
	token_type_t expected;
	int matches;

	matches = 0;

	if (!lexer->next_token) {
		lexer->next_token = _get_next_relevant_token(lexer);

		if (!lexer->next_token) {
			return 0;
		}
	}

	while ((expected = va_arg(types, token_type_t)) > 0 &&
	       token_type_valid(expected)) {
		if (lexer->next_token->type == expected) {
			matches = 1;
			break;
		}
	}

	return matches;
}

token_type_t lexer_next_token_type(lexer_t *lexer)
{
	if (!lexer) {
		return TOKEN_INVALID;
	}

	if (!lexer->next_token) {
		lexer->next_token = _get_next_relevant_token(lexer);

		if (!lexer->next_token) {
			return TOKEN_INVALID;
		}
	}

	return lexer->next_token->type;
}

int lexer_have_token(lexer_t *lexer, ...)
{
	va_list types;
	int matches;

	if (!lexer) {
		return -EINVAL;
	}

	va_start(types, lexer);
	matches = _have_token_with_type(lexer, types);
	va_end(types);

	return matches;
}

token_t* lexer_get_token(lexer_t *lexer, ...)
{
	va_list types;
	token_t *token;

	va_start(types, lexer);

	if (_have_token_with_type(lexer, types)) {
		token = lexer->next_token;
		lexer->next_token = NULL;
	} else {
		token = NULL;
	}

	va_end(types);

	return token;
}
