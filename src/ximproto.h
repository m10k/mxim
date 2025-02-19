/*
 * ximproto.h - This file is part of mxim
 * Copyright (C) 2024 Matthias Kruk
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

#ifndef XIMPROTO_H
#define XIMPROTO_H

#include "ximtypes.h"
#include <stdint.h>
#include <sys/types.h>
#include <X11/Xlib.h>

typedef enum {
	XIM_CONNECT                    =  1,
	XIM_CONNECT_REPLY              =  2,
	XIM_DISCONNECT                 =  3,
	XIM_DISCONNECT_REPLY           =  4,

	XIM_AUTH_REQUIRED              = 10,
	XIM_AUTH_REPLY                 = 11,
	XIM_AUTH_NEXT                  = 12,
	XIM_AUTH_SETUP                 = 13,
	XIM_AUTH_NG                    = 14,

	XIM_ERROR                      = 20,

	XIM_OPEN                       = 30,
	XIM_OPEN_REPLY                 = 31,
	XIM_CLOSE                      = 32,
	XIM_CLOSE_REPLY                = 33,
	XIM_REGISTER_TRIGGERKEYS       = 34,
	XIM_TRIGGER_NOTIFY             = 35,
	XIM_TRIGGER_NOTIFY_REPLY       = 36,
	XIM_SET_EVENT_MASK             = 37,
	XIM_ENCODING_NEGOTIATION       = 38,
	XIM_ENCODING_NEGOTIATION_REPLY = 39,
	XIM_QUERY_EXTENSION            = 40,
	XIM_QUERY_EXTENSION_REPLY      = 41,
	XIM_SET_IM_VALUES              = 42,
	XIM_SET_IM_VALUES_REPLY        = 43,
	XIM_GET_IM_VALUES              = 44,
	XIM_GET_IM_VALUES_REPLY        = 45,

	XIM_CREATE_IC                  = 50,
	XIM_CREATE_IC_REPLY            = 51,
	XIM_DESTROY_IC                 = 52,
	XIM_DESTROY_IC_REPLY           = 53,
	XIM_SET_IC_VALUES              = 54,
	XIM_SET_IC_VALUES_REPLY        = 55,
	XIM_GET_IC_VALUES              = 56,
	XIM_GET_IC_VALUES_REPLY        = 57,
	XIM_SET_IC_FOCUS               = 58,
	XIM_UNSET_IC_FOCUS             = 59,
	XIM_FORWARD_EVENT              = 60,
	XIM_SYNC                       = 61,
	XIM_SYNC_REPLY                 = 62,
	XIM_COMMIT                     = 63,
	XIM_RESET_IC                   = 64,
	XIM_RESET_IC_REPLY             = 65,

	XIM_GEOMETRY                   = 70,
	XIM_STR_CONVERSION             = 71,
	XIM_STR_CONVERSION_REPLY       = 72,
	XIM_PREEDIT_START              = 73,
	XIM_PREEDIT_START_REPLY        = 74,
	XIM_PREEDIT_DRAW               = 75,
	XIM_PREEDIT_CARET              = 76,
	XIM_PREEDIT_CARET_REPLY        = 77,
	XIM_PREEDIT_DONE               = 78,
	XIM_STATUS_START               = 79,
	XIM_STATUS_DRAW                = 80,
	XIM_STATUS_DONE                = 81,
	XIM_PREEDITSTATE               = 82
} xim_msg_type_t;

typedef enum {
	XIM_ERROR_BAD_ALLOC            =   1,
	XIM_ERROR_BAD_STYLE            =   2,
	XIM_ERROR_BAD_CLIENT_WINDOW    =   3,
	XIM_ERROR_BAD_FOCUS_WINDOW     =   4,
	XIM_ERROR_BAD_AREA             =   5,
	XIM_ERROR_BAD_SPOT_LOCATION    =   6,
	XIM_ERROR_BAD_COLORMAP         =   7,
	XIM_ERROR_BAD_ATOM             =   8,
	XIM_ERROR_BAD_PIXEL            =   9,
	XIM_ERROR_BAD_PIXMAP           =  10,
	XIM_ERROR_BAD_NAME             =  11,
	XIM_ERROR_BAD_CURSOR           =  12,
	XIM_ERROR_BAD_PROTOCOL         =  13,
	XIM_ERROR_BAD_FOREROUND        =  14,
	XIM_ERROR_BAD_BACKGROUND       =  15,
	XIM_ERROR_LOCALE_NOT_SUPPORTED =  16,
	XIM_ERROR_BAD_SOMETHING        = 999
} xim_error_t;

typedef enum {
	XIM_COMMIT_FLAG_SYNC   = 1,
	XIM_COMMIT_FLAG_CHARS  = 2,
	XIM_COMMIT_FLAG_KEYSYM = 4
} xim_commit_flags_t;

typedef enum {
	XIM_FORWARD_EVENT_FLAG_SYNC   = 1,
	XIM_FORWARD_EVENT_FLAG_FILTER = 2,
	XIM_FORWARD_EVENT_FLAG_LOOKUP = 4
} xim_forward_event_flags_t;

typedef struct {
	xim_msg_type_t type;
	uint8_t subtype;
	uint16_t length;
} xim_msg_t;

typedef struct {
	xim_msg_t hdr;

	uint8_t byte_order;

	struct {
		uint16_t major;
		uint16_t minor;
	} client_ver;

	struct {
		int num_protos;
		char **protos;
	} auth;
} xim_msg_connect_t;

typedef struct {
	xim_msg_t hdr;

	struct {
		uint16_t major;
		uint16_t minor;
	} server_ver;
} xim_msg_connect_reply_t;

typedef struct {
	xim_msg_t hdr;
} xim_msg_disconnect_t;

typedef struct {
	xim_msg_t hdr;
} xim_msg_disconnect_reply_t;

typedef struct {
	xim_msg_t hdr;

	char *locale;
} xim_msg_open_t;

typedef struct {
	xim_msg_t hdr;

	int id;
	attr_t **im_attrs;
	attr_t **ic_attrs;
} xim_msg_open_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
} xim_msg_close_t;

typedef struct {
	xim_msg_t hdr;

	int im;
} xim_msg_close_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int num_exts;
	char **exts;
} xim_msg_query_extension_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	const ext_t *exts;
} xim_msg_query_extension_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	char **encodings;
	char **details;
} xim_msg_encoding_negotiation_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int category;
	int encoding;
} xim_msg_encoding_negotiation_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int num_attrs;
	int *attrs;
} xim_msg_get_im_values_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int num_values;
	attr_value_t **values;
} xim_msg_get_im_values_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	attr_value_t **values;
} xim_msg_set_im_values_t;

typedef struct {
	xim_msg_t hdr;

	int im;
} xim_msg_set_im_values_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int num_values;
	attr_value_t **values;
} xim_msg_create_ic_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
} xim_msg_create_ic_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
	int num_attrs;
	int *attrs;
} xim_msg_get_ic_values_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
	int num_values;
	attr_value_t **values;
} xim_msg_get_ic_values_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
	attr_value_t **values;
} xim_msg_set_ic_values_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
} xim_msg_set_ic_values_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
} xim_msg_set_ic_focus_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
} xim_msg_unset_ic_focus_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
} xim_msg_destroy_ic_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
} xim_msg_destroy_ic_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
} xim_msg_sync_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
} xim_msg_sync_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
} xim_msg_reset_ic_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
	struct {
		size_t len;
		void *data;
	} preedit;
} xim_msg_reset_ic_reply_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
	xim_commit_flags_t flags;

	uint32_t sym;
	struct {
		size_t len;
		void *data;
	} string;
} xim_msg_commit_t;

struct XCoreKeyEvent {
	uint8_t type;
	uint8_t detail;
	uint16_t sequence_number;
	uint32_t time;
	uint32_t root;
	uint32_t event;
	uint32_t child;
	int16_t root_x;
	int16_t root_y;
	int16_t event_x;
	int16_t event_y;
	uint16_t state;
	uint8_t same_screen;
	uint8_t unused;
} __attribute__((packed));

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
	xim_forward_event_flags_t flags;
	int serial;
	struct XCoreKeyEvent event;
} xim_msg_forward_event_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
	struct {
		uint32_t forward;
		uint32_t sync;
	} masks;
} xim_msg_set_event_mask_t;

typedef struct {
	xim_msg_t hdr;

	int im;
	int ic;
	unsigned flags;
	xim_error_t error;
	size_t detail_len;
	unsigned detail_type;
	void *detail;
} xim_msg_error_t;

int xim_msg_new(xim_msg_t **dst, xim_msg_type_t type);
int xim_msg_decode(xim_msg_t **dst, const uint8_t *src, const size_t src_len);
int xim_msg_encode(xim_msg_t *src, uint8_t *dst, const size_t dst_size);

#endif /* XIMPROTO_H */
