/*
 * ximclient.c - This file is part of mxim
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

#include "fd.h"
#include "inputmethod.h"
#include "ximclient.h"
#include "ximproto.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLIENT_IC_MAX 16
#define CLIENT_IM_MAX 16

struct xim_client {
	fd_t *fd;
	uint8_t rxbuf[1024];
	size_t rxbuf_len;

	input_context_t *ics[CLIENT_IC_MAX];
	input_method_t *ims[CLIENT_IM_MAX];
};

static int xim_client_send(xim_client_t *client, xim_msg_t *msg)
{
	uint8_t buf[1024];
	int buf_len;
	int err;

	if ((buf_len = xim_msg_encode(msg, buf, sizeof(buf))) < 0) {
		fprintf(stderr, "xim_msg_encode: %s\n", strerror(-buf_len));
		return buf_len;
	}

	if ((err = fd_write(client->fd, buf, buf_len)) < 0) {
		fprintf(stderr, "fd_write: %s\n", strerror(-err));
	}

	return err;
}

static int make_detail(char **dst, const char *fmt, va_list args)
{
	char *detail;
	int detail_size;
	int detail_len;
	va_list my_args;

	if (!dst || !fmt) {
		return -EINVAL;
	}

	va_copy(my_args, args);
	detail_len = vsnprintf(NULL, 0, fmt, my_args);
	va_end(my_args);

	if (detail_len == INT_MAX) {
		return -EOVERFLOW;
	}

	if (detail_len == 0) {
		return 0;
	}

	detail_size = detail_len + 1;
	if (!(detail = malloc(detail_size))) {
		return -ENOMEM;
	}

	va_copy(my_args, args);
	vsnprintf(detail, detail_size, fmt, my_args);
	va_end(my_args);

	*dst = detail;
	return detail_len;
}

static int xim_client_send_error(xim_client_t *client, const int im, const int ic,
                                 xim_error_t error, const char *fmt, ...)
{
	xim_msg_error_t msg;
	char *detail;
	int detail_len;
	int err;
	va_list args;

	detail = NULL;

	va_start(args, fmt);
	detail_len = make_detail(&detail, fmt, args);
	va_end(args);

	if (detail_len < 0) {
		detail_len = 0;
	}

	msg.hdr.type = XIM_ERROR;
	msg.hdr.subtype = 0;
	msg.im = im;
	msg.ic = ic;
	msg.flags = (im ? 1 : 0) | (ic ? 2 : 0);
	msg.error = error;
	msg.detail_len = detail_len;
	msg.detail = detail;
	msg.detail_type = 4; /* char data */

	if ((err = xim_client_send(client, (xim_msg_t*)&msg)) < 0) {
		fprintf(stderr, "xim_client_send: %s\n", strerror(-err));
	}

	free(detail);
	return err;
}

static void handle_connect_msg(xim_client_t *client, xim_msg_connect_t *msg)
{
	xim_msg_connect_reply_t *reply;
	int err;

	/* FIXME: There is no need to dynamically allocate the reply */
	if((err = xim_msg_new((xim_msg_t**)&reply, XIM_CONNECT_REPLY)) < 0) {
		/* FIXME: handle error */
		fprintf(stderr, "xim_msg_new: %s\n", strerror(-err));
		return;
	}

	reply->server_ver.major = 1;
	reply->server_ver.minor = 0;

	if ((err = xim_client_send(client, (xim_msg_t*)reply)) < 0) {
		fprintf(stderr, "xim_client_send: %s\n", strerror(-err));
	}

	/* FIXME: Free reply */

	return;
}

static void set_event_mask(xim_client_t *client, const int im, const int ic, const uint32_t mask)
{
	xim_msg_set_event_mask_t msg;
	int err;

	msg.hdr.type = XIM_SET_EVENT_MASK;
	msg.hdr.subtype = 0;
	msg.im = im;
	msg.ic = ic;
	msg.masks.forward = mask;
	msg.masks.sync = mask;

	if ((err = xim_client_send(client, (xim_msg_t*)&msg)) < 0) {
		fprintf(stderr, "xim_client_send: %s\n", strerror(-err));
	}
}

static void handle_open_msg(xim_client_t *client, xim_msg_open_t *msg)
{
	input_method_t *im;
	int err;
	int id;
	int i;

	for (i = id = 0; i < (sizeof(client->ims) / sizeof(client->ims[0])); i++) {
		if (!client->ims[i]) {
			id = i + 1;
			break;
		}
	}

	if (!id) {
		xim_client_send_error(client, 0, 0, XIM_ERROR_BAD_ALLOC,
		                      "Client has reached the maximum number of open input methods");
		return;
	}

	if (!(im = input_method_for_locale(msg->locale))) {
		/* XIM_ERROR */

		if ((err = xim_client_send_error(client, 0, 0, XIM_ERROR_LOCALE_NOT_SUPPORTED, NULL)) < 0) {
			fprintf(stderr, "xim_client_send_error: %s\n", strerror(-err));
			/* FIXME: handle error */
		}
	} else {
		/* XIM_OPEN_REPLY */

		xim_msg_open_reply_t reply;

		reply.hdr.type = XIM_OPEN_REPLY;
		reply.hdr.subtype = 0;
		reply.id = id;
		reply.im_attrs = NULL;
		reply.ic_attrs = NULL;

		if ((err = input_method_get_im_attrs(im, &reply.im_attrs)) < 0) {
			fprintf(stderr, "Could not get IM attributes from input method: %s\n",
			        strerror(-err));
			xim_client_send_error(client, 0, 0, XIM_ERROR_BAD_SOMETHING,
			                      "Could not get IM attributes from input method: %s\n",
			                      strerror(-err));
		} else if ((err = input_method_get_ic_attrs(im, &reply.ic_attrs)) < 0) {
			fprintf(stderr, "Could not get IC attributes from input method: %s\n",
			        strerror(-err));
			xim_client_send_error(client, 0, 0, XIM_ERROR_BAD_SOMETHING,
			                      "Could not get IC attributes from input method: %s\n",
			                      strerror(-err));
		} else {
			client->ims[id - 1] = im;

			if ((err = xim_client_send(client, (xim_msg_t*)&reply)) < 0) {
				fprintf(stderr, "xim_client_send: %s\n", strerror(-err));
				/* FIXME: handle error */
			}

			set_event_mask(client, id, 0, KeyPressMask);
		}

		attrs_free(&reply.im_attrs);
		attrs_free(&reply.ic_attrs);
	}

	return;
}

static void handle_query_extension_msg(xim_client_t *client, xim_msg_query_extension_t *msg)
{
	xim_msg_query_extension_reply_t reply;
	input_method_t *im;
	int err;

	if (msg->im <= 0 || msg->im > CLIENT_IM_MAX ||
	    !(im = client->ims[msg->im - 1])) {
		xim_client_send_error(client, 0, 0, XIM_ERROR_BAD_SOMETHING, "Invalid IM id");
		return;
	}

	reply.hdr.type = XIM_QUERY_EXTENSION_REPLY;
	reply.hdr.subtype = 0;
	reply.im = msg->im;
	reply.exts = im->exts;

	if ((err = xim_client_send(client, (xim_msg_t*)&reply)) < 0) {
		fprintf(stderr, "xim_client_send: %s\n", strerror(-err));
	}

	return;
}

static void select_encoding(xim_client_t *client, int im, int encoding)
{
	xim_msg_encoding_negotiation_reply_t reply;
	int err;

	reply.hdr.type = XIM_ENCODING_NEGOTIATION_REPLY;
	reply.hdr.subtype = 0;
	reply.im = im;
	reply.category = 1;
	reply.encoding = encoding;

	if ((err = xim_client_send(client, (xim_msg_t*)&reply)) < 0) {
		fprintf(stderr, "xim_client_send: %s\n", strerror(-err));
	}
}

static void handle_encoding_negotiation_msg(xim_client_t *client, xim_msg_encoding_negotiation_t *msg)
{
	input_method_t *im;
	int err;
	int i;

	if (!client || !msg || !msg->encodings) {
		return;
	}

	if (msg->im <= 0 || msg->im > CLIENT_IM_MAX ||
	    !(im = client->ims[msg->im - 1])) {
		xim_client_send_error(client, msg->im, 0, XIM_ERROR_BAD_SOMETHING, "Invalid IM id");
		return;
	}

	if (im->encodings) {
		for (i = 0; im->encodings[i]; i++) {
			int j;

			for (j = 0; msg->encodings[j]; j++) {
				if (strcmp(im->encodings[i], msg->encodings[j]) == 0) {
					/* XIM_ENCODING_NEGOTIATION_REPLY */
					fprintf(stderr, "Client supports %s encoding\n", msg->encodings[j]);
					select_encoding(client, msg->im, j);
					return;
				}
			}

			fprintf(stderr, "Encoding %s not supported by client\n", im->encodings[i]);
		}
	}

	/* XIM_ERROR: Server and client don't have any common encodings */
	if ((err = xim_client_send_error(client, msg->im, 0, XIM_ERROR_BAD_SOMETHING,
	                                 "Server doesn't support any of the client's encodings")) < 0) {
		fprintf(stderr, "xim_client_send_error: %s\n", strerror(-err));
	}

	return;
}

static void handle_get_im_values_msg(xim_client_t *client, xim_msg_get_im_values_t *msg)
{
	xim_msg_get_im_values_reply_t reply;
	input_method_t *im;
	int err;
	int i;

	if (!client || !msg) {
		return;
	}

	if (msg->im <= 0 || msg->im > CLIENT_IM_MAX ||
	    !(im = client->ims[msg->im - 1])) {
		xim_client_send_error(client, msg->im, 0, XIM_ERROR_BAD_SOMETHING, "Invalid IM id");
		return;
	}

	memset(&reply, 0, sizeof(reply));
	reply.hdr.type = XIM_GET_IM_VALUES_REPLY;
	reply.hdr.subtype = 0;
	reply.im = msg->im;
	reply.num_values = 0;

	if (!(reply.values = calloc(msg->num_attrs, sizeof(attr_value_t*)))) {
		return;
	}

	for (i = 0; i < msg->num_attrs; i++) {
		int idx;
		/* FIXME: check if index is valid */

		idx = msg->attrs[i] - 1;
		reply.values[i] = im->im_attrs[idx].value;
		reply.num_values++;
	}

	if ((err = xim_client_send(client, (xim_msg_t*)&reply)) < 0) {
		fprintf(stderr, "xim_client_send: %s\n", strerror(-err));
	}

	free(reply.values);
	return;
}

static void handle_create_ic_msg(xim_client_t *client, xim_msg_create_ic_t *msg)
{
	xim_msg_create_ic_reply_t reply;
	input_method_t *im;
	input_context_t *ic;
	int err;
	int id;
	int i;

	if (msg->im <= 0 || msg->im > CLIENT_IM_MAX ||
	    !(im = client->ims[msg->im - 1])) {
		xim_client_send_error(client, msg->im, 0, XIM_ERROR_BAD_SOMETHING,
		                      "Invalid IM id");
		return;
	}

	for (i = id = 0; i < (sizeof(client->ics) / sizeof(client->ics[0])); i++) {
		if (!client->ics[i]) {
			id = i + 1;
			break;
		}
	}

	if (!id) {
		xim_client_send_error(client, msg->im, 0, XIM_ERROR_BAD_ALLOC,
		                      "Client has reached the maximum number of open input contexts");
		/* TODO: Handle error */
		return;
	}

	if ((err = input_context_new(&ic, im)) < 0) {
		xim_client_send_error(client, msg->im, 0, XIM_ERROR_BAD_ALLOC,
		                      "Could not allocate input context: %s", strerror(-err));
		/* TODO: Handle error */
		return;
	}

	for (i = 0; i < msg->num_values; i++) {
		if ((err = input_context_set_attribute(ic, msg->values[i])) < 0) {
			/* TODO: Handle error */
		}
	}

	client->ics[id - 1] = ic;

	reply.hdr.type = XIM_CREATE_IC_REPLY;
	reply.hdr.subtype = 0;
	reply.im = msg->im;
	reply.ic = id;

	if ((err = xim_client_send(client, (xim_msg_t*)&reply)) < 0) {
		fprintf(stderr, "xim_client_send: %s\n", strerror(-err));
		/* TODO: Handle error */
	}

	return;
}

static void handle_get_ic_values_msg(xim_client_t *client, xim_msg_get_ic_values_t *msg)
{
	input_method_t *im;
	input_context_t *ic;
	attr_value_t **values;
	int num_values;
	int err;
	int i;

	err = 0;
	values = NULL;
	num_values = 0;

	if (msg->im <= 0 || msg->im > CLIENT_IM_MAX ||
	    !(im = client->ims[msg->im - 1])) {
		xim_client_send_error(client, msg->im, msg->ic, XIM_ERROR_BAD_SOMETHING, "Invalid IM id");
		return;
	}

	if (msg->ic <= 0 || msg->ic > CLIENT_IC_MAX ||
	    !(ic = client->ics[msg->ic - 1])) {
		xim_client_send_error(client, msg->im, msg->ic, XIM_ERROR_BAD_SOMETHING, "Invalid IC id");
		return;
	}

	if (!(values = calloc(msg->num_attrs + 1, sizeof(*values)))) {
		xim_client_send_error(client, msg->im, msg->ic, XIM_ERROR_BAD_ALLOC,
		                      "Cannot allocate memory for attributes");
		return;
	}

	for (i = 0; i < msg->num_attrs; i++) {
		if ((err = input_context_get_attribute(ic, msg->attrs[i], &values[i])) < 0) {
			xim_client_send_error(client, msg->im, msg->ic, XIM_ERROR_BAD_SOMETHING,
			                      "Could not get IC value: %s", strerror(-err));
			break;
		}

		num_values++;
	}

	if (!err) {
		xim_msg_get_ic_values_reply_t reply;

		reply.hdr.type = XIM_GET_IC_VALUES_REPLY;
		reply.hdr.subtype = 0;
		reply.im = msg->im;
		reply.ic = msg->ic;
		reply.num_values = num_values;
		reply.values = values;

		if ((err = xim_client_send(client, (xim_msg_t*)&reply)) < 0) {
			fprintf(stderr, "xim_client_send: %s\n", strerror(-err));
		}
	}

	attr_values_free(&values);
	return;
}

static void _xim_client_handle_msg(xim_client_t *client, xim_msg_t *msg)
{
	fprintf(stderr, "Handling message\n");

	switch (msg->type) {
	case XIM_CONNECT:
		fprintf(stderr,
		        "XIM_CONNECT received\n"
		        " -> %hu.%hu\n", ((xim_msg_connect_t*)msg)->client_ver.major,
		        ((xim_msg_connect_t*)msg)->client_ver.minor);
		handle_connect_msg(client, (xim_msg_connect_t*)msg);
		break;

	case XIM_OPEN:
		fprintf(stderr,
		        "XIM_OPEN received\n"
		        " -> locale = %s\n",
		        ((xim_msg_open_t*)msg)->locale);
		handle_open_msg(client, (xim_msg_open_t*)msg);
		break;

	case XIM_QUERY_EXTENSION:
		fprintf(stderr, "XIM_QUERY_EXTENSION received\n");
		if (((xim_msg_query_extension_t*)msg)->exts) {
			int i;

			for (i = 0; i < ((xim_msg_query_extension_t*)msg)->num_exts; i++) {
				fprintf(stderr, " ext[%d] -> %s\n", i,
				        ((xim_msg_query_extension_t*)msg)->exts[i]);
			}
		}
		handle_query_extension_msg(client, (xim_msg_query_extension_t*)msg);
		break;

	case XIM_ENCODING_NEGOTIATION:
		fprintf(stderr, "XIM_ENCODING_NEGOTIATION received\n");
		if (((xim_msg_encoding_negotiation_t*)msg)->encodings) {
			int i;

			for (i = 0; ((xim_msg_encoding_negotiation_t*)msg)->encodings[i]; i++) {
				fprintf(stderr, " enc[%d] -> %s\n", i,
				        ((xim_msg_encoding_negotiation_t*)msg)->encodings[i]);
			}
		}
		handle_encoding_negotiation_msg(client, (xim_msg_encoding_negotiation_t*)msg);
		break;

	case XIM_GET_IM_VALUES:
		fprintf(stderr, "XIM_GET_IM_VALUES\n");
		handle_get_im_values_msg(client, (xim_msg_get_im_values_t*)msg);
		break;

	case XIM_CREATE_IC:
		fprintf(stderr, "XIM_CREATE_IC\n");
		handle_create_ic_msg(client, (xim_msg_create_ic_t*)msg);
		break;


	case XIM_GET_IC_VALUES:
		fprintf(stderr, "XIM_GET_IC_VALUES\n");
		handle_get_ic_values_msg(client, (xim_msg_get_ic_values_t*)msg);
		break;

	default:
		fprintf(stderr, "Unhandled message type: %d\n", msg->type);
		break;
	}
}

static void _xim_client_in(fd_t *fd, fd_event_t event, xim_client_t *client, void *data)
{
	xim_msg_t *msg;
	ssize_t received_bytes;
	ssize_t parsed_bytes;

	received_bytes = fd_read(fd, client->rxbuf + client->rxbuf_len,
	                         sizeof(client->rxbuf) - client->rxbuf_len);

	fprintf(stderr, "%s(): Received %ld bytes\n", __func__, received_bytes);

	if (received_bytes == 0) {
		/* client disconnected */
		xim_client_free(&client);
		return;
	}

	if (received_bytes < 0) {
		/* FIXME: handle error */
		fprintf(stderr, "fd_read: %s\n", strerror(-received_bytes));
		return;
	}

	client->rxbuf_len += received_bytes;

	while ((parsed_bytes = xim_msg_decode(&msg, client->rxbuf, client->rxbuf_len)) > 0) {
		_xim_client_handle_msg(client, msg);

		/* Remove parsed message from the buffer */
		client->rxbuf_len -= parsed_bytes;
		memmove(client->rxbuf, client->rxbuf + parsed_bytes, client->rxbuf_len);
		free(msg); /* FIXME: this is not the right function to free parsed messages */
	}
}

int xim_client_new(xim_client_t **client, fd_t *fd)
{
	xim_client_t *xc;

	if (!(xc = calloc(1, sizeof(*xc)))) {
		return -ENOMEM;
	}

	xc->fd = fd;
	fd->userdata = xc;

	fd_set_callback(fd, FD_EVENT_IN, (fd_callback_t*)_xim_client_in, xc);

	*client = xc;
	return 0;
}

int xim_client_free(xim_client_t **client)
{
	if (!client || !*client) {
		return -EINVAL;
	}

	if ((*client)->fd) {
		fd_free(&(*client)->fd);
	}

	free(*client);
	*client = NULL;

	return 0;
}
