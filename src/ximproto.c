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
#include <string.h>
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

struct XIM_OPEN {
	uint8_t len;
	char str[];
};

struct XIM_OPEN_REPLY {
	uint16_t im_id;
	/* length of LISTofXIMATTR */
	/* LISTofXIMATTR */
	/* length of LISTofXICATTR */
	/* 2 bytes padding */
	/* LISTofXICATTR */
};

struct XIM_QUERY_EXTENSION {
	uint16_t im;
	uint16_t exts_len;
	char exts[];
	/* pad */
};

struct XIM_QUERY_EXTENSION_REPLY {
	uint16_t im;
	uint16_t exts_len;
	uint8_t exts[];
};

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
	},
	[XIM_OPEN] = {
		.type = XIM_OPEN,
		.name = "XIM_OPEN",
		.size = sizeof(xim_msg_open_t)
	},
	[XIM_OPEN_REPLY] = {
		.type = XIM_OPEN_REPLY,
		.name = "XIM_OPEN_REPLY",
		.size = sizeof(xim_msg_open_reply_t)
	},
	[XIM_QUERY_EXTENSION] = {
		.type = XIM_QUERY_EXTENSION,
		.name = "XIM_QUERY_EXTENSION",
		.size = sizeof(xim_msg_query_extension_t)
	},
	[XIM_QUERY_EXTENSION_REPLY] = {
		.type = XIM_QUERY_EXTENSION_REPLY,
		.name = "XIM_QUERY_EXTENSION_REPLY",
		.size = sizeof(xim_msg_query_extension_reply_t)
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

static int decode_XIM_OPEN(xim_msg_t **dst, const struct XIM_OPEN *src, const size_t src_len)
{
	xim_msg_open_t *msg;
	char *locale;
	int parsed_len;
	int padding;
	int padded_len;

	if ((parsed_len = decode_STR(&locale, (const uint8_t*)src, src_len)) < 0) {
		return parsed_len;
	}

	if (!(msg = calloc(1, sizeof(*msg)))) {
		free(locale);
		return -ENOMEM;
	}

	msg->locale = locale;
	padding = PAD(parsed_len);
	padded_len = parsed_len + padding;

	*dst = (xim_msg_t*)msg;
	return padded_len;
}

static int decode_XIM_QUERY_EXTENSION(xim_msg_t **dst, const struct XIM_QUERY_EXTENSION *src,
                                      const size_t src_len)
{
	xim_msg_query_extension_t *msg;
	int computed_len;
	int padding_len;
	int padded_len;
	int parsed_len;

	computed_len = sizeof(*src) + src->exts_len;
	padding_len = PAD(computed_len);
	padded_len = computed_len + padding_len;

	if (padded_len > src_len) {
		return -EBADMSG;
	}

	if (!(msg = calloc(1, sizeof(*msg)))) {
		return -ENOMEM;
	}

	msg->im = src->im;

	for (parsed_len = sizeof(*src); parsed_len < computed_len; ) {
		char **new_exts;
		char *ext;
		int ext_len;

		if ((ext_len = decode_STR(&ext, (uint8_t*)src + parsed_len, src_len - parsed_len)) < 0) {
			free(msg);
			return -EBADMSG;
		}

		if (!(new_exts = realloc(msg->exts, sizeof(char*) * msg->num_exts + 1))) {
			free(msg);
			return -ENOMEM;
		}

		new_exts[msg->num_exts++] = ext;
		msg->exts = new_exts;

	        parsed_len += ext_len;
	}

	*dst = (xim_msg_t*)msg;
	return parsed_len + padding_len;
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

		case XIM_OPEN:
			fprintf(stderr, "Decoding XIM_OPEN\n");
			err = decode_XIM_OPEN(&msg, (struct XIM_OPEN*)(hdr + 1),
			                      src_len - sizeof(*hdr));
			break;

		case XIM_QUERY_EXTENSION:
			fprintf(stderr, "Decoding XIM_QUERY_EXTENSION\n");
			err = decode_XIM_QUERY_EXTENSION(&msg, (struct XIM_QUERY_EXTENSION*)(hdr + 1),
			                                 src_len - sizeof(*hdr));
			break;

		default:
			fprintf(stderr, "Decoding of %hhu type message not implemented\n",
			        hdr->opcode_major);
			err = -ENOSYS;
			break;
		}

		if (err > 0) {
			msg->type = hdr->opcode_major;
			msg->subtype = hdr->opcode_minor;
			msg->length = hdr->length * 4;

			err += sizeof(*hdr);
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

static int encode_XIM_OPEN_REPLY(xim_msg_open_reply_t *src, uint8_t *dst, const size_t dst_size)
{
	struct XIM_OPEN_REPLY *raw;
	size_t required_size;
	uint16_t num_imattrs;
	uint16_t len_imattrs;
	uint16_t num_icattrs;
	uint16_t len_icattrs;
	int encoded_len;
	int i;

	if (!src || !dst) {
		return -EINVAL;
	}

	required_size = 4 * sizeof(uint16_t);
	num_imattrs = num_icattrs = 0;
	len_imattrs = len_icattrs = 0;

	if (src->im_attrs) {
		for (i = 0; src->im_attrs[i].name; i++) {
			size_t attr_len;

			fprintf(stderr, "XIMATTR: %s\n", src->im_attrs[i].name);
			attr_len = 6 + strlen(src->im_attrs[i].name);
			required_size += attr_len + PAD(attr_len);
			num_imattrs++;
			len_imattrs += attr_len + PAD(attr_len);
		}
	}

	if (src->ic_attrs) {
		for (i = 0; src->ic_attrs[i].name; i++) {
			size_t attr_len;

			fprintf(stderr, "XICATTR: %s\n", src->ic_attrs[i].name);
			attr_len = 6 + strlen(src->ic_attrs[i].name);
			required_size += attr_len + PAD(attr_len);
			num_icattrs++;
			len_icattrs += attr_len + PAD(attr_len);
		}
	}

	if (dst_size < required_size) {
		return -EMSGSIZE;
	}

	raw = (struct XIM_OPEN_REPLY*)dst;
	raw->im_id = src->id;
	*(uint16_t*)(raw + 1) = len_imattrs;
	encoded_len = sizeof(*raw) + sizeof(uint16_t);

	for (i = 0; i < num_imattrs; i++) {
		int len;

		if ((len = encode_ATTR(&src->im_attrs[i],
		                       dst + encoded_len,
		                       dst_size - encoded_len)) < 0) {
			return len;
		}
		encoded_len += len;
	}

	*(uint16_t*)(dst + encoded_len + 0) = len_icattrs;
	*(uint16_t*)(dst + encoded_len + 2) = 0;
	encoded_len += 4;

	for (i = 0; i < num_icattrs; i++) {
		int len;

		if ((len = encode_ATTR(&src->ic_attrs[i],
		                       dst + encoded_len,
		                       dst_size - encoded_len)) < 0) {
			return len;
		}
		encoded_len += len;
	}

	return encoded_len;
}

static int encode_XIM_QUERY_EXTENSION_REPLY(xim_msg_query_extension_reply_t *src,
                                            uint8_t *dst, const size_t dst_size)
{
	struct XIM_QUERY_EXTENSION_REPLY *raw;
	int len_exts;
	int i;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (dst_size < sizeof(*raw)) {
		return -ENOMEM;
	}

	raw = (struct XIM_QUERY_EXTENSION_REPLY*)dst;
	raw->im = src->im;
	len_exts = 0;

	if (src->exts) {
		for (i = 0; src->exts[i].name; i++) {
			int ext_len;

			if ((ext_len = encode_EXT(&src->exts[i], raw->exts + len_exts,
			                          dst_size - sizeof(*raw) - len_exts)) < 0) {
				return ext_len;
			}

			len_exts += ext_len;
		}
	}
	raw->exts_len = len_exts;

	return len_exts + sizeof(*raw);
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

	case XIM_OPEN_REPLY:
		payload_len = encode_XIM_OPEN_REPLY((xim_msg_open_reply_t*)src,
		                                    (uint8_t*)(hdr + 1),
		                                    dst_size - sizeof(*hdr));
		break;

	case XIM_QUERY_EXTENSION_REPLY:
		payload_len = encode_XIM_QUERY_EXTENSION_REPLY((xim_msg_query_extension_reply_t*)src,
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
