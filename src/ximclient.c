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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct xim_client {
	fd_t *fd;
	uint8_t rxbuf[1024];
	size_t rxbuf_len;

	input_method_t *im;
};

static void handle_connect_msg(xim_client_t *client, xim_msg_connect_t *msg)
{
	xim_msg_connect_reply_t *reply;
	uint8_t buf[2048];
	int buf_len;
	int err;

	/* FIXME: There is no need to dynamically allocate the reply */
	if((err = xim_msg_new((xim_msg_t**)&reply, XIM_CONNECT_REPLY)) < 0) {
		/* FIXME: handle error */
		fprintf(stderr, "xim_msg_new: %s\n", strerror(-err));
		return;
	}

	reply->server_ver.major = 1;
	reply->server_ver.minor = 0;

	fprintf(stderr, "Encoding reply\n");

	if ((buf_len = xim_msg_encode((xim_msg_t*)reply, buf, sizeof(buf))) > 0) {
		err = fd_write(client->fd, buf, buf_len);

		if (err < 0) {
			/* FIXME: handle error */
			fprintf(stderr, "fd_write: %s\n", strerror(-err));
		}
	}

	/* FIXME: Free reply */

	return;
}

static void handle_open_msg(xim_client_t *client, xim_msg_open_t *msg)
{
	input_method_t *im;
	uint8_t buf[2048];
	int buf_len;
	int err;

	if (!(im = input_method_for_locale(msg->locale))) {
		/* XIM_ERROR */
	} else {
		/* XIM_OPEN_REPLY */

		xim_msg_open_reply_t reply;

		reply.hdr.type = XIM_OPEN_REPLY;
		reply.hdr.subtype = 0;
		reply.id = im->id;
		reply.im_attrs = im->im_attrs;
		reply.ic_attrs = im->ic_attrs;

		if ((buf_len = xim_msg_encode((xim_msg_t*)&reply, buf, sizeof(buf))) > 0) {
			err = fd_write(client->fd, buf, buf_len);

			if (err < 0) {
				/* FIXME: handle error */
				fprintf(stderr, "fd_write: %s\n", strerror(-err));
			} else {
				client->im = im;
			}
		}
	}

	return;
}

static void handle_query_extension_msg(xim_client_t *client, xim_msg_query_extension_t *msg)
{
	uint8_t buf[2048];
	int buf_len;
	int err;

	xim_msg_query_extension_reply_t reply;

	if (!client->im) {
		return;
	}

	reply.hdr.type = XIM_QUERY_EXTENSION_REPLY;
	reply.hdr.subtype = 0;
	reply.im = client->im->id;
	reply.exts = client->im->exts;

	if ((buf_len = xim_msg_encode((xim_msg_t*)&reply, buf, sizeof(buf))) > 0) {
		err = fd_write(client->fd, buf, buf_len);

		if (err < 0) {
			fprintf(stderr, "fd_write: %s\n", strerror(-err));
		} else {

		}
	}

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
