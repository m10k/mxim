/*
 * dictparser.c - This file is part of mxim
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

#define _POSIX_C_SOURCE 200809L
#include "dict.h"
#include "dictparser.h"
#include "token.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct string_list;
struct integer_list;
struct array;
struct value;
struct entry_list;
struct entry;
struct property_list;
struct property;

struct dict_parser {
	lexer_t *lexer;
	char *file;
	struct entry_list *entry_list;
};

int _syntax_error(dict_parser_t *parser, const char *fmt, ...)
{
	va_list args;
	int line;
	int col;

	if (!parser) {
		return -EINVAL;
	}

	if (lexer_get_position(parser->lexer, &line, &col) < 0) {
		line = -1;
		col = -1;
	}

	fprintf(stderr, "Syntax error in %s:line %d:col %d%s", parser->file, line, col, fmt ? " " : "\n");

	if (fmt) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
	}

	return 0;
}

#if MXIM_DEBUG
static void _token_debug(token_t *token, int depth);
static void _value_debug(struct value *value, int depth);
static void _string_list_debug(struct string_list *string_list, int depth);
static void _integer_list_debug(struct integer_list *integer_list, int depth);
static void _array_debug(struct array *array, int depth);
static void _entry_list_debug(struct entry_list *entry_list, int depth);
static void _entry_debug(struct entry *entry, int depth);
static void _property_list_debug(struct property_list *property_list, int depth);
static void _property_debug(struct property *property, int depth);
#endif /* MXIM_DEBUG */

static int  _value_free(struct value **value);
static int  _value_parse(dict_parser_t *parser, struct value **value);
static int  _string_list_parse(dict_parser_t *parser, struct string_list **string_list);
static int  _string_list_free(struct string_list **string_list);
static int  _integer_list_parse(dict_parser_t *parser, struct integer_list **integer_list);
static int  _integer_list_free(struct integer_list **integer_list);
static int  _array_parse(dict_parser_t *parser, struct array **array);
static int  _array_free(struct array **array);
static int  _entry_list_parse(dict_parser_t *parser, struct entry_list **entry_list);
static int  _entry_list_free(struct entry_list **entry_list);
static int  _entry_parse(dict_parser_t *parser, struct entry **entry);
static int  _entry_free(struct entry **entry);
static int  _property_list_parse(dict_parser_t *parser, struct property_list **property_list);
static int  _property_list_free(struct property_list **property_list);
static int  _property_parse(dict_parser_t *parser, struct property **property);
static int  _property_free(struct property **property);

static int _get_property(struct property_list *property_list, const char *key, void *out);

struct entry_list {
	struct entry_list *entry_list;
	token_t *comma;
	struct entry *entry;
};

#if MXIM_DEBUG
static void _entry_list_debug(struct entry_list *entry_list, int depth)
{
	if (entry_list) {
		fprintf(stderr, "%*sentry_list %p\n", depth, "", (void*)entry_list);
		_entry_list_debug(entry_list->entry_list, depth + 1);
		_token_debug(entry_list->comma, depth + 1);
		_entry_debug(entry_list->entry, depth + 1);
	}
}
#endif /* MXIM_DEBUG */

static int _entry_list_parse(dict_parser_t *parser, struct entry_list **entry_list)
{
	struct entry_list *top;
	token_t *comma;
	int err;

	top = NULL;
	comma = NULL;
	err = 0;

	do {
		struct entry_list *list;

		if (!(list = calloc(1, sizeof(*list)))) {
			err = -ENOMEM;
			break;
		}

		if ((err = _entry_parse(parser, &list->entry)) < 0) {
			_syntax_error(parser, "Expected entry\n");
			_entry_list_free(&list);
			break;
		}

		list->entry_list = top;
		list->comma = comma;
		top = list;

		comma = lexer_get_token(parser->lexer, TOKEN_COMMA, 0);
	} while (comma);

	if (err) {
		_entry_list_free(&top);
		token_free(&comma);
	} else {
		*entry_list = top;
	}

	return err;
}

static int _entry_list_free(struct entry_list **entry_list)
{
	if (!entry_list || !*entry_list) {
		return -EINVAL;
	}

	_entry_list_free(&(*entry_list)->entry_list);
	token_free(&(*entry_list)->comma);
	_entry_free(&(*entry_list)->entry);

	free(*entry_list);
	*entry_list = NULL;

	return 0;
}

struct entry {
	token_t *lbrace;
	struct property_list *property_list;
	token_t *rbrace;
};

#if MXIM_DEBUG
static void _entry_debug(struct entry *entry, int depth)
{
	if (entry) {
		fprintf(stderr, "%*sentry %p\n", depth, "", (void*)entry);
		_token_debug(entry->lbrace, depth + 1);
		_property_list_debug(entry->property_list, depth + 1);
		_token_debug(entry->rbrace, depth + 1);
	}
}
#endif /* MXIM_DEBUG */

static int _entry_parse(dict_parser_t *parser, struct entry **entry)
{
	struct entry *ent;
	int err;

	err = -EPROTO;

	if (!parser || !entry) {
		return -EINVAL;
	}

	if (!(ent = calloc(1, sizeof(*ent)))) {
		return -ENOMEM;
	}

	if ((ent->lbrace = lexer_get_token(parser->lexer, TOKEN_LBRACE, 0)) &&
	    (err = _property_list_parse(parser, &ent->property_list)) == 0 &&
	    (ent->rbrace = lexer_get_token(parser->lexer, TOKEN_RBRACE, 0))) {
		err = 0;
	}

	if (err) {
		_entry_free(&ent);
	} else {
		*entry = ent;
	}

	return err;
}

static int _entry_free(struct entry **entry)
{
	if (!entry || !*entry) {
		return -EINVAL;
	}

	token_free(&(*entry)->lbrace);
	_property_list_free(&(*entry)->property_list);
	token_free(&(*entry)->rbrace);

	free(*entry);
	*entry = NULL;

	return 0;
}

struct property_list {
	struct property_list *property_list;
	token_t *comma;
	struct property *property;
};

#if MXIM_DEBUG
static void _property_list_debug(struct property_list *property_list, int depth)
{
	if (property_list) {
		fprintf(stderr, "%*sproperty_list %p\n", depth, "", (void*)property_list);
		_property_list_debug(property_list->property_list, depth + 1);
		_token_debug(property_list->comma, depth + 1);
		_property_debug(property_list->property, depth + 1);
	}
}
#endif /* MXIM_DEBUG */

static int _property_list_parse(dict_parser_t *parser, struct property_list **property_list)
{
	struct property_list *top;
	token_t *comma;
	int err;

	top = NULL;
	comma = NULL;
	err = 0;

	do {
		struct property_list *list;

		if (!(list = calloc(1, sizeof(*list)))) {
			err = -ENOMEM;
			break;
		}

		if ((err = _property_parse(parser, &list->property)) < 0) {
			_syntax_error(parser, "Expected property\n");
			_property_list_free(&list);
			break;
		}

		list->property_list = top;
		list->comma = comma;
		top = list;

		comma = lexer_get_token(parser->lexer, TOKEN_COMMA, 0);
	} while (comma);

	if (err) {
		_property_list_free(&top);
		token_free(&comma);
	} else {
		*property_list = top;
	}

	return err;
}

static int _property_list_free(struct property_list **property_list)
{
	if (!property_list || !*property_list) {
		return -EINVAL;
	}

	_property_list_free(&(*property_list)->property_list);
	token_free(&(*property_list)->comma);
	_property_free(&(*property_list)->property);

	free(*property_list);
	*property_list = NULL;

	return 0;
}

struct property {
	token_t *identifier;
	token_t *equals;
	struct value *value;
};

#if MXIM_DEBUG
static void _property_debug(struct property *property, int depth)
{
	if (property) {
		fprintf(stderr, "%*sproperty %p\n", depth, "", (void*)property);
		_token_debug(property->identifier, depth + 1);
		_token_debug(property->equals, depth + 1);
		_value_debug(property->value, depth + 1);
	}
}
#endif /* MXIM_DEBUG */

static int _property_parse(dict_parser_t *parser, struct property **property)
{
	struct property *prop;
	int err;

	err = -EPROTO;

	if (!parser || !property) {
		return -EINVAL;
	}

	if (!(prop = calloc(1, sizeof(*prop)))) {
		return -ENOMEM;
	}

	if ((prop->identifier = lexer_get_token(parser->lexer, TOKEN_IDENTIFIER, 0)) &&
	    (prop->equals = lexer_get_token(parser->lexer, TOKEN_EQUALS, 0))) {
		err = _value_parse(parser, &prop->value);
	}

	if (!err) {
		*property = prop;
	} else {
		_property_free(&prop);
	}

	return err;
}

static int _property_free(struct property **property)
{
	if (!property || !*property) {
		return -EINVAL;
	}

	token_free(&(*property)->identifier);
	token_free(&(*property)->equals);
	_value_free(&(*property)->value);

	free(*property);
	*property = NULL;

	return 0;
}

struct value {
	token_t *integer;
	token_t *string;
	struct array *array;
	struct entry *entry;
};

#if MXIM_DEBUG
static void _token_debug(token_t *token, int depth)
{
	if (token) {
		fprintf(stderr, "%*stoken %p [%u, \"%s\"]\n",
		        depth, "", (void*)token,
		        token->type, token->lexeme);
	}
}

static void _value_debug(struct value *value, int depth)
{
	if (!value) {
		return;
	}

	fprintf(stderr, "%*svalue %p\n", depth, "", (void*)value);
	_token_debug(value->integer, depth + 1);
	_token_debug(value->string, depth + 1);
	_array_debug(value->array, depth + 1);
	_entry_debug(value->entry, depth + 1);
}
#endif /* MXIM_DEBUG */

static int _value_free(struct value **value)
{
	if (!value || !*value) {
		return -EINVAL;
	}

	token_free(&(*value)->integer);
	token_free(&(*value)->string);
	_array_free(&(*value)->array);
	_entry_free(&(*value)->entry);

	free(*value);
	*value = NULL;

	return 0;
}

static int _value_parse(dict_parser_t *parser, struct value **value)
{
	struct value *val;
	int err;

	err = 0;

	if (!(val = calloc(1, sizeof(*val)))) {
		return -ENOMEM;
	}

	switch (lexer_next_token_type(parser->lexer)) {
	case TOKEN_STRING:
		val->string = lexer_get_token(parser->lexer, TOKEN_STRING, 0);
		assert(val->string);
		break;

	case TOKEN_INTEGER:
		val->integer = lexer_get_token(parser->lexer, TOKEN_INTEGER, 0);
		assert(val->integer);
		break;

	case TOKEN_LBRACKET:
		err = _array_parse(parser, &val->array);
		break;

	case TOKEN_LBRACE:
		err = _entry_parse(parser, &val->entry);

	default:
		_syntax_error(parser, "Expected string, integer, array, or entry\n");
		err = -EPROTO;
		break;
	}

	if (!err) {
		*value = val;
	} else {
		_value_free(&val);
	}

	return err;
}

struct array {
	token_t *lbracket;
	struct integer_list *integer_list;
	struct string_list *string_list;
	struct entry_list *entry_list;
	token_t *rbracket;
};

#if MXIM_DEBUG
static void _array_debug(struct array *array, int depth)
{
	if (!array) {
		return;
	}

	fprintf(stderr, "%*sarray %p %s\n", depth, "", (void*)array,
	        array->lbracket ? array->lbracket->lexeme : "(?)");
	_string_list_debug(array->string_list, depth + 1);
	_integer_list_debug(array->integer_list, depth + 1);
	_entry_list_debug(array->entry_list, depth + 1);
	fprintf(stderr, "%*s\n", depth, array->rbracket ? array->rbracket->lexeme : "(?)");
}
#endif /* MXIM_DEBUG */

static int _array_free(struct array **array)
{
	if (!array || !*array) {
		return -EINVAL;
	}

	token_free(&(*array)->lbracket);
	token_free(&(*array)->rbracket);
	_string_list_free(&(*array)->string_list);
	_integer_list_free(&(*array)->integer_list);
	_entry_list_free(&(*array)->entry_list);
	free(*array);
	*array = NULL;

	return 0;
}

static int _array_parse(dict_parser_t *parser, struct array **array)
{
	struct array *arr;
	int err;

	if (!(arr = calloc(1, sizeof(*arr)))) {
		return -ENOMEM;
	}

	arr->lbracket = lexer_get_token(parser->lexer, TOKEN_LBRACKET, 0);
	assert(arr->lbracket); /* if this fails, it's a bug */

	switch (lexer_next_token_type(parser->lexer)) {
	case TOKEN_STRING:
		err = _string_list_parse(parser, &arr->string_list);
		break;

	case TOKEN_INTEGER:
		err = _integer_list_parse(parser, &arr->integer_list);
		break;

	case TOKEN_LBRACE:
		err = _entry_list_parse(parser, &arr->entry_list);
		break;

	default:
		_syntax_error(parser, "Expected string, integer, or entry\n");
		err = -EPROTO;
		break;
	}

	if (!err) {
		arr->rbracket = lexer_get_token(parser->lexer, TOKEN_RBRACKET, 0);
		if (!arr->rbracket) {
			_syntax_error(parser, "Expected `]'\n");
			err = -EPROTO;
		}
	}

	if (err) {
		_array_free(&arr);
	} else {
		*array = arr;
	}

	return err;
}

struct integer_list {
	struct integer_list *integer_list;
	token_t *comma;
	token_t *integer;
};

#if MXIM_DEBUG
static void _integer_list_debug(struct integer_list *integer_list, int depth)
{
	if (!integer_list) {
		return;
	}

	fprintf(stderr, "%*sinteger_list %p [ %s, %s ]\n", depth, "",
	        (void*)integer_list, integer_list->integer ? integer_list->integer->lexeme : "(?)",
	        integer_list->comma ? integer_list->comma->lexeme : "(?)");
	_integer_list_debug(integer_list->integer_list, depth + 1);
}
#endif /* MXIM_DEBUG */

static int _integer_list_free(struct integer_list **integer_list)
{
	if (!integer_list || !*integer_list) {
		return -EINVAL;
	}

	_integer_list_free(&(*integer_list)->integer_list);
	token_free(&(*integer_list)->comma);
	token_free(&(*integer_list)->integer);

	free(*integer_list);
	*integer_list = NULL;

	return 0;
}

static int _integer_list_parse(dict_parser_t *parser, struct integer_list **integer_list)
{
	struct integer_list *top;
	token_t *comma;
	int err;

	top = NULL;
	comma = NULL;
	err = 0;

	do {
		struct integer_list *list;

		if (!(list = calloc(1, sizeof(*list)))) {
			err = -ENOMEM;
			break;
		}

		list->integer = lexer_get_token(parser->lexer, TOKEN_INTEGER, 0);
		if (!list->integer) {
			int line;
			int col;

			lexer_get_position(parser->lexer, &line, &col);
			fprintf(stderr, "%s:%d:%d: Expected integer\n", parser->file, line, col);

			_integer_list_free(&list);
			err = -EPROTO;
			break;
		}

		list->integer_list = top;
		list->comma = comma;
		top = list;

		comma = lexer_get_token(parser->lexer, TOKEN_COMMA, 0);
	} while (comma);

	if (err) {
		_integer_list_free(&top);
		token_free(&comma);
	} else {
		*integer_list = top;
	}

	return err;
}

struct string_list {
	struct string_list *string_list;
	token_t *comma;
	token_t *string;
};

#if MXIM_DEBUG
static void _string_list_debug(struct string_list *string_list, int depth)
{
	if (string_list) {
		fprintf(stderr, "%*sstring_list %p [ %s, %s ]\n", depth, "",
		        (void*)string_list, string_list->string ? string_list->string->lexeme : "(?)",
		        string_list->comma ? string_list->comma->lexeme : "(?)");
		_string_list_debug(string_list->string_list, depth + 1);
	}
}
#endif /* MXIM_DEBUG */

static int _string_list_free(struct string_list **string_list)
{
	if (!string_list || !*string_list) {
		return -EINVAL;
	}

	_string_list_free(&(*string_list)->string_list);
	token_free(&(*string_list)->comma);
	token_free(&(*string_list)->string);

	free(*string_list);
	*string_list = NULL;

	return 0;
}

static int _string_list_parse(dict_parser_t *parser, struct string_list **string_list)
{
	struct string_list *top;
	token_t *comma;
	int err;

	top = NULL;
	comma = NULL;
	err = 0;

	do {
		struct string_list *list;

		if (!(list = calloc(1, sizeof(*list)))) {
			err = -ENOMEM;
			break;
		}

		list->string = lexer_get_token(parser->lexer, TOKEN_STRING, 0);
		if (!list->string) {
			_syntax_error(parser, "Expected string\n");
			_string_list_free(&list);
			err = -EPROTO;
			break;
		}

		list->string_list = top;
		list->comma = comma;
		top = list;

		comma = lexer_get_token(parser->lexer, TOKEN_COMMA, 0);
	} while (comma);

	if (err) {
		_string_list_free(&top);
		token_free(&comma);
	} else {
		*string_list = top;
	}

	return err;
}

int dict_parser_new(dict_parser_t **parser, const char *file)
{
	dict_parser_t *p;
	int err;

	if (!parser) {
		return -EINVAL;
	}

	if (!(p = calloc(1, sizeof(*p)))) {
		return -ENOMEM;
	}

	if (!(p->file = strdup(file))) {
		free(p);
		return -ENOMEM;
	}

	if ((err = lexer_new(&p->lexer, file)) < 0) {
		free(p->file);
		free(p);
		return err;
	}

	_entry_list_parse(p, &p->entry_list);

	*parser = p;
	return 0;
}

int dict_parser_free(dict_parser_t **parser)
{
	if (!parser) {
		return -EINVAL;
	}

	lexer_free(&(*parser)->lexer);

	free(*parser);
	*parser = NULL;

	return 0;
}

static int _get_integer(token_t *integer, int *out)
{
	int intval;
	int err;

	errno = 0;
	intval = strtol(integer->lexeme, NULL, 10);
	err = -errno;

	if (!err) {
		*out = intval;
	}

	return err;
}

static int _get_string(token_t *string, char **out)
{
	char *str;
	int err;

	if (!(str = strdup(string->lexeme))) {
		err = -ENOMEM;
	} else {
		*out = str;
		err = 0;
	}

	return err;
}

static int _get_dict_candidate(struct entry *entry, dict_candidate_t **out)
{
	dict_candidate_t *c;
	int err;

	if ((err = dict_candidate_new(&c)) < 0) {
		return err;
	}

	if (_get_property(entry->property_list, "priority", (void*)&c->priority) < 0) {
		/* we're OK with candidates without priority property */
		c->priority = 0;
	}

	if ((err = _get_property(entry->property_list, "value", (void*)&c->value)) < 0) {
		dict_candidate_free(&c);
	} else {
		*out = c;
		err = 0;
	}

	return err;
}

static int _count_entries(struct entry_list *entry_list)
{
	int n;

	if (!entry_list) {
		return -EINVAL;
	}

	for (n = 0; entry_list; entry_list = entry_list->entry_list) {
		n++;
	}

	return n;
}

static int _get_dict_candidates(struct array *array, dict_candidate_t ***out)
{
	dict_candidate_t **candidates;
	struct entry_list *entry_list;
	int num_items;
	int i;

	i = 0;
	num_items = _count_entries(array->entry_list);

	assert(num_items >= 0);

	if (num_items == INT_MAX) {
		return -EOVERFLOW;
	}

	if (!(candidates = malloc((num_items + 1) * sizeof(*candidates)))) {
		return -ENOMEM;
	}

	for (entry_list = array->entry_list; entry_list; entry_list = entry_list->entry_list) {
		_get_dict_candidate(entry_list->entry, &candidates[i++]);
	}

	*out = candidates;

	return 0;
}

static int _get_dict_entry(struct entry *entry, dict_entry_t **dict_entry)
{
	dict_entry_t *ent;
	int err;

	if (!(ent = calloc(1, sizeof(*ent)))) {
		return -ENOMEM;
	}

	if (_get_property(entry->property_list, "priority", (void*)&ent->priority) < 0) {
		/* we're OK with entries without priority property */
		ent->priority = 0;
	}

	if ((err = _get_property(entry->property_list, "key", (void*)&ent->key_utf8)) < 0 ||
	    (err = char_from_utf8(ent->key_utf8, strlen(ent->key_utf8), &ent->key)) < 0 ||
	    (err = _get_property(entry->property_list, "candidates", (void*)&ent->candidates)) < 0) {
		dict_entry_free(&ent);
	} else {
		while (ent->candidates[ent->num_candidates]) {
			ent->num_candidates++;
		}

		*dict_entry = ent;
	}

	return err;
}

static int _get_array(struct array *array, void ***out)
{
	/* FIXME: Add support for arrays containing things other than candidates */
	return _get_dict_candidates(array, (dict_candidate_t***)out);
}

static int _get_value(struct value *value, void *out)
{
	int err;

	err = 0;

	if (value->integer) {
		err = _get_integer(value->integer, (int*)out);
	} else if (value->string) {
		err = _get_string(value->string, (char**)out);
	} else if (value->array) {
		err = _get_array(value->array, (void***)out);
	} else if (value->entry) {
		/* FIXME: Add support for object-type properties */
		err = -ENOSYS;
	} else {
		err = -EBADFD;
	}

	return err;
}

static int _get_property(struct property_list *property_list, const char *key, void *out)
{
	for (; property_list; property_list = property_list->property_list) {
		if (!property_list->property ||
		    !property_list->property->identifier ||
		    !property_list->property->identifier->lexeme) {
			continue;
		}

		if (strcmp(property_list->property->identifier->lexeme, key) != 0) {
			continue;
		}

		return _get_value(property_list->property->value, out);
	}

	return -ENOENT;
}

int dict_parser_get_dict(dict_parser_t *parser, dict_t **dict)
{
	struct entry_list *entry_list;
	dict_t *d;
	int err;

	if ((err = dict_new(&d)) < 0) {
		return err;
	}

	for (entry_list = parser->entry_list; entry_list; entry_list = entry_list->entry_list) {
		dict_entry_t *ent;

		err = _get_dict_entry(entry_list->entry, &ent);
		if (err) {
			continue;
		}

		err = dict_add(d, &ent, 1);
		if (err) {
			dict_entry_free(&ent);
			break;
		}
	}

	if (!err) {
		*dict = d;
	} else {
		dict_free(&d);
	}

	return err;
}
