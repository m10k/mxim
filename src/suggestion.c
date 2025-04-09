#define _POSIX_C_SOURCE 200809L

#include "suggestion.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int suggestion_new(suggestion_t **suggestion, const char *value, const char *display)
{
	suggestion_t *sug;

	if (!suggestion || !value) {
		return -EINVAL;
	}

	if (!(sug = malloc(sizeof(*sug)))) {
		return -ENOMEM;
	}

	sug->display = display ? strdup((char*)display) : NULL;
	sug->value = value ? strdup((char*)value) : NULL;

	if ((display != NULL) != (sug->display != NULL) ||
	    (value != NULL) != (sug->value != NULL)) {
		suggestion_free(&sug);
		return -ENOMEM;
	}

	*suggestion = sug;
	return 0;
}

int suggestion_free(suggestion_t **suggestion)
{
	if (!suggestion) {
		return -EINVAL;
	}

	free((*suggestion)->value);
	free((*suggestion)->display);
	free(*suggestion);
	*suggestion = NULL;
	return 0;
}

const char *suggestion_get_value(suggestion_t *suggestion)
{
	return suggestion->value;
}

const char *suggestion_get_display(suggestion_t *suggestion)
{
	return suggestion->display ? suggestion->display : suggestion->value;
}

int suggestion_cmp(const suggestion_t *a, const suggestion_t *b)
{
	return strcmp(a->value, b->value);
}
