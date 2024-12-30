/*
 * xhandler.h - This file is part of mxim
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

#ifndef XHANDLER_H
#define XHANDLER_H

#include <X11/Xlib.h>

typedef struct x_handler x_handler_t;

int x_handler_init(x_handler_t **handler);
int x_handler_free(x_handler_t **handler);
int x_handler_run(x_handler_t *handler);

int x_handler_get_client_window(x_handler_t *handler, Window window, Window *client);
int x_handler_set_text_property(x_handler_t *handler, Window window, const char *name, const char *value);

#endif /* XHANDLER_H */
