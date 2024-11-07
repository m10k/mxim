/*
 * ximserver.h - This file is part of mxim
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

#ifndef XIMSERVER_H
#define XIMSERVER_H

typedef struct xim_server xim_server_t;

int xim_server_init(xim_server_t **server, const char *address, unsigned short port);
int xim_server_free(xim_server_t **server);

int xim_server_start(xim_server_t *server);
int xim_server_stop(xim_server_t *server);

#endif /* XIMSERVER_H */
