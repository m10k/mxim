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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

enum _atoms {
	ATOM_IM = 0,
	ATOM_XIM_SERVERS,
	ATOM_LOCALES,
	ATOM_TRANSPORT,
	ATOM_MAX
};
static const char *_atom_names[] = {
	"@server=mxim",
	"XIM_SERVERS",
	"LOCALES",
	"TRANSPORT"
};

struct xim_server {
	Display *display;
	Atom atoms[ATOM_MAX];
	char *properties[ATOM_MAX];
	Window window;
};

static int _xim_server_is_connected(xim_server_t *server)
{
	return server->display != NULL;
}

static int _xim_server_connect(xim_server_t *server)
{
	Window root_window;
	XSetWindowAttributes attrs;
	int screen;
	int i;

	if (_xim_server_is_connected(server)) {
		return -EALREADY;
	}

	if (!(server->display = XOpenDisplay(NULL))) {
		return -EIO;
	}

	for (i = ATOM_IM; i < ATOM_MAX; i++) {
		if ((server->atoms[i] = XInternAtom(server->display,
		                                    _atom_names[i],
		                                    False)) == None) {
			fprintf(stderr, "%s: Could not lookup atom: %s\n",
			        __func__, _atom_names[i]);
			return -EFAULT;
		}
	}

	screen = DefaultScreen(server->display);
	root_window = RootWindow(server->display, screen);

	memset(&attrs, 0, sizeof(attrs));
	attrs.event_mask = ExposureMask;

	server->window = XCreateWindow(server->display, root_window, 0, 0, 1, 1, 0,
	                               CopyFromParent, CopyFromParent,
	                               DefaultVisual(server->display, screen),
	                               CWEventMask, &attrs);
	XSetSelectionOwner(server->display, server->atoms[ATOM_IM], server->window, CurrentTime);
	XSetSelectionOwner(server->display, server->atoms[ATOM_XIM_SERVERS], server->window, CurrentTime);

	/* register IM Server by prepending it to the XIM_SERVERS property of the root window */
	XChangeProperty(server->display, root_window,
	                server->atoms[ATOM_XIM_SERVERS], XA_ATOM, 32, PropModePrepend,
	                (unsigned char*)&server->atoms[ATOM_IM], 1);
	XSync(server->display, False);

	server->properties[ATOM_LOCALES] = "@locales=en_US";
	server->properties[ATOM_TRANSPORT] = "@transport=tcp/127.0.0.1:1234";

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

static int _handle_selection_request(xim_server_t *server, XSelectionRequestEvent *event)
{
	XEvent response;

	if (event->target == server->atoms[ATOM_LOCALES]) {
		static const char locale[] = "@locale=en_US";

		XChangeProperty(server->display, event->requestor,
		                event->property, event->target,
		                8, PropModeReplace, (unsigned char*)locale,
		                strlen(locale));
	} else if (event->target == server->atoms[ATOM_TRANSPORT]) {
		static const char transport[] = "@transport=tcp/127.0.0.1:1234";

		XChangeProperty(server->display, event->requestor,
		                event->property, event->target,
		                8, PropModeReplace, (unsigned char*)transport,
		                strlen(transport));
	} else {
		fprintf(stderr, "XSelectionRequestEvent on unhandled property 0x%lx\n", event->target);
		return -ENOSYS;
	}

	memset(&response, 0, sizeof(response));
	response.type = SelectionNotify;
	response.xselection.requestor = event->requestor;
	response.xselection.selection = event->selection;
	response.xselection.target = event->target;
	response.xselection.time = event->time;
	response.xselection.property = event->property;
	XSendEvent(server->display, event->requestor, False, NoEventMask, &response);

	return 0;
}

static int _handle_x_event(xim_server_t *server)
{
	XEvent event;
	int err;

	if (XNextEvent(server->display, &event) != 0) {
		return -EAGAIN;
	}

	switch (event.type) {
	case SelectionRequest:
		err = _handle_selection_request(server, (XSelectionRequestEvent*)&event);
		break;

	default:
		fprintf(stderr, "Unhandled XEvent with type 0x%x\n", event.type);
		err = -ENOSYS;
		break;
	}

	return err;
}

int xim_server_run(xim_server_t *server)
{
	int err;

	while (1) {
		_handle_x_event(server);
	}

	return err;
}
