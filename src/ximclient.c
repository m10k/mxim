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
#include "ximclient.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

struct xim_client {
	fd_t *fd;
	uint8_t rxbuf[1024];
	size_t rxbuf_len;


};

static void _xim_client_in(fd_t *fd, fd_event_t event, xim_client_t *client, void *data)
{
	return;
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
