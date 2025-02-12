/*
 * jkim.c - This file is part of mxim
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

#include "keysym.h"
#include "ximtypes.h"
#include "inputmethod.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>

struct XIMSTYLES {
	uint16_t num_styles;
	uint16_t unused;
	XIMStyle style[2];
} __attribute__((packed));

static int _jkim_commit(input_method_t *im, input_context_t *ic, cmd_arg_t *arg);
static int _jkim_delete(input_method_t *im, input_context_t *ic, cmd_arg_t *arg);
static int _jkim_lang_switch(input_method_t *im, input_context_t *ic, cmd_arg_t *arg);
static int _jkim_cursor_move(input_method_t *im, input_context_t *ic, cmd_arg_t *arg);
static int _jkim_candidate_move(input_method_t *im, input_context_t *ic, cmd_arg_t *arg);
static int _jkim_candidate_select(input_method_t *im, input_context_t *ic, cmd_arg_t *arg);
static int _jkim_segment_move(input_method_t *im, input_context_t *ic, cmd_arg_t *arg);
static int _jkim_segment_resize(input_method_t *im, input_context_t *ic, cmd_arg_t *arg);
static int _jkim_segment_new(input_method_t *im, input_context_t *ic, cmd_arg_t *arg);
static int _jkim_toggle_onoff(input_method_t *im, input_context_t *ic, cmd_arg_t *arg);

static const attr_t _im_attr_inputstyle = {
	.id = 1,
	.type = ATTR_TYPE_XIMSTYLES,
	.name = XNQueryInputStyle
};

static struct XIMSTYLES _im_attrvalue_inputstyle_data = {
	.num_styles = 2,
	.style      = {
		XIMPreeditNothing | XIMStatusNothing,
		XIMPreeditNothing | XIMStatusNone
	}
};

static attr_value_t _im_attrvalue_inputstyle = {
	.id = 1,
	.len = sizeof(_im_attrvalue_inputstyle_data),
	.data = (uint8_t*)&_im_attrvalue_inputstyle_data
};

static const attr_t _ic_attr_inputstyle = {
	.id = 1,
	.type = ATTR_TYPE_CARD32,
	.name = XNInputStyle
};
static const attr_t _ic_attr_clientwindow = {
	.id = 2,
	.type = ATTR_TYPE_WINDOW,
	.name = XNClientWindow
};
static const attr_t _ic_attr_focuswindow = {
	.id = 3,
	.type = ATTR_TYPE_WINDOW,
	.name = XNFocusWindow
};
static const attr_t _ic_attr_filterevents = {
	.id = 4,
	.type = ATTR_TYPE_CARD32,
	.name = XNFilterEvents
};
static const attr_t _ic_attr_nlseparator = {
	.id = 5,
	.type = 0,
	.name = XNSeparatorofNestedList
};

static const uint32_t _ic_attrvalue_filterevents_data = 0;
static attr_value_t _ic_attrvalue_filterevents = {
	.id = 4,
	.len = sizeof(_ic_attrvalue_filterevents_data),
	.data = (uint32_t*)&_ic_attrvalue_filterevents_data
};

static const char *_encodings[] = {
	"UTF-8",
	NULL
};

input_method_t _jkim = {
	/* root-window input method style */
	.input_style = XIMPreeditNone | XIMStatusNone,
	/* handle any locale */
	.locale = NULL,
	.im_attrs = {
		[0] = {
			.attr = &_im_attr_inputstyle,
			.value = &_im_attrvalue_inputstyle
		}
	},

	.ic_attrs = {
		[0] = {
			.attr = &_ic_attr_inputstyle,
			.value = NULL
		},
		[1] = {
			.attr = &_ic_attr_clientwindow,
			.value = NULL
		},
		[2] = {
			.attr = &_ic_attr_focuswindow,
			.value = NULL
		},
		[3] = {
			.attr = &_ic_attr_filterevents,
			.value = &_ic_attrvalue_filterevents
		},
		[4] = {
			.attr = &_ic_attr_nlseparator,
			.value = NULL
		}
	},

	.cmds = {
		[CMD_COMMIT]           = (cmd_func_t*)_jkim_commit,
		[CMD_DELETE]           = (cmd_func_t*)_jkim_delete,
		[CMD_LANG_SELECT]      = (cmd_func_t*)_jkim_lang_switch,
		[CMD_CURSOR_MOVE]      = (cmd_func_t*)_jkim_cursor_move,
		[CMD_CANDIDATE_MOVE]   = (cmd_func_t*)_jkim_candidate_move,
		[CMD_CANDIDATE_SELECT] = (cmd_func_t*)_jkim_candidate_select,
		[CMD_SEGMENT_MOVE]     = (cmd_func_t*)_jkim_segment_move,
		[CMD_SEGMENT_RESIZE]   = (cmd_func_t*)_jkim_segment_resize,
		[CMD_SEGMENT_NEW]      = (cmd_func_t*)_jkim_segment_new,
		[CMD_ONOFF]            = (cmd_func_t*)_jkim_toggle_onoff,
	},
	.active = 1,

	/* no extensions */
	.exts = NULL,
	.encodings = _encodings
};

static int _jkim_lang_switch(input_method_t *im, input_context_t *ic, cmd_arg_t *arg)
{
	lang_t lang;
	static const char *_langs[] = {
		[LANG_EN] = "LANG_EN",
		[LANG_JA] = "LANG_JA",
		[LANG_KR] = "LANG_KR",
	};

	if (!im->active) {
		return -EAGAIN;
	}

	lang = (lang_t)arg->u;
	fprintf(stderr, "Switching context %p to %s\n", (void*)ic, _langs[lang]);
	return input_context_set_language(ic, lang);
}

static int _jkim_cursor_move(input_method_t *im, input_context_t *ic, cmd_arg_t *arg)
{
	if (!im->active) {
		return -EAGAIN;
	}

	return input_context_cursor_move(ic, arg->s[0], arg->s[1]);
}

static int _jkim_delete(input_method_t *im, input_context_t *ic, cmd_arg_t *arg)
{
	if (!im->active) {
		return -EAGAIN;
	}

	return input_context_erase(ic, arg->i);
}

static int _jkim_commit(input_method_t *im, input_context_t *ic, cmd_arg_t *arg)
{
	if (!im->active) {
		return -EAGAIN;
	}

	return input_context_commit(ic);
}

static int _jkim_candidate_move(input_method_t *im, input_context_t *ic, cmd_arg_t *arg)
{
	if (!im->active) {
		return -EAGAIN;
	}

	return input_context_move_candidate(ic, arg->i);
}

static int _jkim_candidate_select(input_method_t *im, input_context_t *ic, cmd_arg_t *arg)
{
	if (!im->active) {
		return -EAGAIN;
	}

	return input_context_select_candidate(ic, arg->u);
}

static int _jkim_segment_move(input_method_t *im, input_context_t *ic, cmd_arg_t *arg)
{
	if (!im->active) {
		return -EAGAIN;
	}

	return input_context_move_segment(ic, arg->i);
}

static int _jkim_segment_resize(input_method_t *im, input_context_t *ic, cmd_arg_t *arg)
{
	if (!im->active) {
		return -EAGAIN;
	}

	return -ENOSYS;
}

static int _jkim_segment_new(input_method_t *im, input_context_t *ic, cmd_arg_t *arg)
{
	if (!im->active) {
		return -EAGAIN;
	}

	return input_context_insert_segment(ic);
}

static int _jkim_toggle_onoff(input_method_t *im, input_context_t *ic, cmd_arg_t *arg)
{
	im->active = !im->active;
	return 0;
}
