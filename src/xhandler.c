/*
 * xhandler.c - This file is part of mxim
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

#include "xhandler.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

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

struct x_handler {
	Display *display;
	Atom atoms[ATOM_MAX];
	char *properties[ATOM_MAX];
	Window window;
};

static int _x_handler_is_connected(x_handler_t *handler)
{
	return handler->display != NULL;
}

static int _x_handler_connect(x_handler_t *handler)
{
	Window root_window;
	XSetWindowAttributes attrs;
	int screen;
	int i;

	if (_x_handler_is_connected(handler)) {
		return -EALREADY;
	}

	if (!(handler->display = XOpenDisplay(NULL))) {
		return -EIO;
	}

	for (i = ATOM_IM; i < ATOM_MAX; i++) {
		if ((handler->atoms[i] = XInternAtom(handler->display,
		                                    _atom_names[i],
		                                    False)) == None) {
			fprintf(stderr, "%s: Could not lookup atom: %s\n",
			        __func__, _atom_names[i]);
			return -EFAULT;
		}
	}

	screen = DefaultScreen(handler->display);
	root_window = RootWindow(handler->display, screen);

	memset(&attrs, 0, sizeof(attrs));
	attrs.event_mask = ExposureMask;

	handler->window = XCreateWindow(handler->display, root_window, 0, 0, 1, 1, 0,
	                                CopyFromParent, CopyFromParent,
	                                DefaultVisual(handler->display, screen),
	                                CWEventMask, &attrs);
	XSetSelectionOwner(handler->display, handler->atoms[ATOM_IM], handler->window, CurrentTime);
	XSetSelectionOwner(handler->display, handler->atoms[ATOM_XIM_SERVERS], handler->window, CurrentTime);

	/* register IM Server by prepending it to the XIM_SERVERS property of the root window */
	XChangeProperty(handler->display, root_window,
	                handler->atoms[ATOM_XIM_SERVERS], XA_ATOM, 32, PropModePrepend,
	                (unsigned char*)&handler->atoms[ATOM_IM], 1);
	XSync(handler->display, False);

	handler->properties[ATOM_LOCALES] = "@locales=en_US";
	handler->properties[ATOM_TRANSPORT] = "@transport=tcp/127.0.0.1:1234";

	return 0;
}

int x_handler_init(x_handler_t **handler)
{
        x_handler_t *hnd;
	int err;

	err = -ENOMEM;

	if ((hnd = calloc(1, sizeof(*hnd)))) {
		err = _x_handler_connect(hnd);
	}

	if (!err) {
		*handler = hnd;
	} else {
	        x_handler_free(&hnd);
	}

	return err;
}

int x_handler_free(x_handler_t **handler)
{
	if (!handler || !*handler) {
		return -EINVAL;
	}

	if ((*handler)->display) {
		XCloseDisplay((*handler)->display);
		(*handler)->display = NULL;
	}

	free(*handler);
	*handler = NULL;

	return 0;
}

static int _handle_selection_request(x_handler_t *handler, XSelectionRequestEvent *event)
{
	XEvent response;

	if (event->target == handler->atoms[ATOM_LOCALES]) {
		static const char locale[] = "@locale=en_US";

		XChangeProperty(handler->display, event->requestor,
		                event->property, event->target,
		                8, PropModeReplace, (unsigned char*)locale,
		                strlen(locale));
	} else if (event->target == handler->atoms[ATOM_TRANSPORT]) {
		static const char transport[] = "@transport=tcp/127.0.0.1:1234";

		XChangeProperty(handler->display, event->requestor,
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
	XSendEvent(handler->display, event->requestor, False, NoEventMask, &response);

	return 0;
}

static int _handle_x_event(x_handler_t *handler)
{
	XEvent event;
	int err;

	if (XNextEvent(handler->display, &event) != 0) {
		return -EAGAIN;
	}

	switch (event.type) {
	case SelectionRequest:
		err = _handle_selection_request(handler, (XSelectionRequestEvent*)&event);
		break;

	default:
		fprintf(stderr, "Unhandled XEvent with type 0x%x\n", event.type);
		err = -ENOSYS;
		break;
	}

	return err;
}

int x_handler_run(x_handler_t *handler)
{
	int err;

	while (1) {
		_handle_x_event(handler);
	}

	return err;
}

int x_handler_get_client_window(x_handler_t *handler, Window window, Window *client)
{
	Window current;
	Window root;
	Window parent;

	root = parent = None;
	current = window;

	while (current != root) {
		Window current_parent;
		Window *children;
		unsigned nchildren;

		children = NULL;
		XQueryTree(handler->display, current, &root, &current_parent, &children, &nchildren);

		if (children) {
			XFree(children);
		}

		if (current_parent == root) {
			break;
		}

		if (parent != root) {
			parent = current_parent;
		}

		current = current_parent;
	}

	*client = parent;
	return 0;
}

int x_handler_set_text_property(x_handler_t *handler, Window window, const char *name, const char *value)
{
	XTextProperty prop;
	Atom atom;

	if ((atom = XInternAtom(handler->display, name, False)) == None) {
		return -EIO;
	}

	if (Xutf8TextListToTextProperty(handler->display, (char**)&value, 1,
	                                XUTF8StringStyle, &prop) != Success) {
		return -ENOMEM;
	}

	XSetTextProperty(handler->display, window, &prop, atom);
	XSync(handler->display, False);
	XFree(prop.value);

	return 0;
}
