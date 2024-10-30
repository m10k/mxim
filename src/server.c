/*
 * server.c - This file is part of mxim
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

#include "server.h"
#include <errno.h>
#include <stdlib.h>
#include <X11/Xlib.h>

struct xim_server {
	Display *display;
};

static int _xim_server_is_connected(xim_server_t *server)
{
	return server->display != NULL;
}

static int _xim_server_connect(xim_server_t *server)
{
	if (_xim_server_is_connected(server)) {
		return -EALREADY;
	}

	if (!(server->display = XOpenDisplay(NULL))) {
		return -EIO;
	}

	return 0;
}

int xim_server_init(xim_server_t **server)
{
	xim_server_t *s;
	int err;

	err = -ENOMEM;

	if ((s = calloc(1, sizeof(*s)))) {
		err = _xim_server_connect(s);
	}

	if (!err) {
		*server = s;
	} else {
		xim_server_free(&s);
	}

	return err;
}

int xim_server_free(xim_server_t **server)
{
	if (!server || !*server) {
		return -EINVAL;
	}

	if ((*server)->display) {
		XCloseDisplay((*server)->display);
		(*server)->display = NULL;
	}

	free(*server);
	*server = NULL;

	return 0;
}

int xim_server_run(xim_server_t *server)
{
	return -ENOSYS;
}
