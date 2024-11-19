/*
 * ximproto.c - This file is part of mxim
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

#include "ximproto.h"
#include "ximtypes.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

struct XIM_PACKET {
	uint8_t opcode_major;
	uint8_t opcode_minor;
	uint16_t length;
} __attribute__((packed));

struct XIM_CONNECT {
	uint8_t byte_order;
	uint8_t unused;

	struct {
		uint16_t major;
		uint16_t minor;
	} client_ver;

	struct {
		uint16_t num_protos;
		char protos[];
	} auth;
} __attribute__((packed));

struct XIM_CONNECT_REPLY {
	struct {
		uint16_t major;
		uint16_t minor;
	} server_ver;
} __attribute__((packed));

static const struct {
	xim_msg_type_t type;
	char *name;
	size_t size;
} _msg_info[] = {
	[XIM_CONNECT] = {
		.type = XIM_CONNECT,
		.name = "XIM_CONNECT",
		.size = sizeof(xim_msg_connect_t)
	},
	[XIM_CONNECT_REPLY] = {
		.type = XIM_CONNECT_REPLY,
		.name = "XIM_CONNECT_REPLY",
		.size = sizeof(xim_msg_connect_reply_t)
	}
};

static int need_more_data(struct XIM_PACKET *src, const size_t src_len)
{
	return (src_len < sizeof(*src) || /* check if header is there */
	        src_len < (sizeof(*src) + src->length * 4)); /* check if payload is there */
}

static int decode_XIM_CONNECT(xim_msg_t **dst, const struct XIM_CONNECT *src, const size_t src_len)
{
	xim_msg_connect_t *msg;
	size_t remaining_len;
	int n;
	int skip;

	if (!(msg = calloc(1, sizeof(*msg)))) {
		return -ENOMEM;
	}

	msg->client_ver.major = src->client_ver.major;
	msg->client_ver.minor = src->client_ver.minor;

	if (!(msg->auth.protos = calloc(src->auth.num_protos + 1, sizeof(char*)))) {
		free(msg);
		return -ENOMEM;
	}

	msg->auth.num_protos = src->auth.num_protos;
	remaining_len = src_len - offsetof(struct XIM_CONNECT, auth.num_protos);

	for (skip = n = 0; n < src->auth.num_protos; n++) {
		int parsed;

		if ((parsed = decode_STRING(&msg->auth.protos[n],
		                            (uint8_t*)src->auth.protos + skip,
		                            remaining_len)) < 0) {
			break;
		}

		skip += parsed;
		remaining_len -= parsed;
	}

	*dst = (xim_msg_t*)msg;
	return skip + sizeof(*src);
}

int xim_msg_decode(xim_msg_t **dst, const uint8_t *src, const size_t src_len)
{
	struct XIM_PACKET *hdr;
	xim_msg_t *msg;
	int err;

	hdr = (struct XIM_PACKET*)src;
	msg = NULL;

	if (need_more_data(hdr, src_len)) {
		err = -EAGAIN;
	} else {
		switch (hdr->opcode_major) {
		case XIM_CONNECT:
			fprintf(stderr, "Decoding XIM_CONNECT\n");
			err = decode_XIM_CONNECT(&msg, (struct XIM_CONNECT*)(hdr + 1),
			                         src_len - sizeof(*hdr));
			break;

		default:
			err = -ENOSYS;
			break;
		}

		if (err > 0) {
			msg->type = hdr->opcode_major;
			msg->subtype = hdr->opcode_minor;
			msg->length = hdr->length * 4;

			*dst = msg;
		}
	}

	return err;
}

int xim_msg_new(xim_msg_t **msg, const xim_msg_type_t type)
{
	xim_msg_t *m;

	if (!msg) {
		return -EINVAL;
	}

	if (!(m = calloc(1, _msg_info[type].size))) {
		return -ENOMEM;
	}

	m->type = type;
	m->subtype = 0;

	*msg = m;
	return 0;
}

static int encode_XIM_CONNECT_REPLY(xim_msg_connect_reply_t *src, uint8_t *dst, const size_t dst_size)
{
	struct XIM_CONNECT_REPLY *raw;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (dst_size < sizeof(*raw)) {
		return -EMSGSIZE;
	}

	raw = (struct XIM_CONNECT_REPLY*)dst;
	raw->server_ver.major = src->server_ver.major;
	raw->server_ver.minor = src->server_ver.minor;

	return sizeof(*raw);
}

int xim_msg_encode(xim_msg_t *src, uint8_t *dst, const size_t dst_size)
{
	struct XIM_PACKET *hdr;
	int payload_len;
	int ret_val;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (dst_size < sizeof(struct XIM_PACKET)) {
		return -ENOMEM;
	}

	hdr = (struct XIM_PACKET*)dst;
	ret_val = -EFAULT;
	payload_len = -1;

	switch (src->type) {
	case XIM_CONNECT_REPLY:
		payload_len = encode_XIM_CONNECT_REPLY((xim_msg_connect_reply_t*)src,
		                                       (uint8_t*)(hdr + 1),
		                                       dst_size - sizeof(*hdr));
		break;

	default:
		ret_val = -ENOSYS;
		break;
	}

	if (payload_len >= 0) {
		hdr->opcode_major = src->type;
		hdr->opcode_minor = src->subtype;
		hdr->length = payload_len / 4;

		ret_val = sizeof(*hdr) + payload_len;
	}

	return ret_val;
}
