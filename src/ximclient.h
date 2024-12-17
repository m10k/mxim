/*
 * ximclient.h - This file is part of mxim
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

#ifndef XIMCLIENT_H
#define XIMCLIENT_H

#include "fd.h"
#include <stddef.h>

typedef struct xim_client xim_client_t;
typedef struct input_method input_method_t;
typedef struct input_context input_context_t;

int xim_client_new(xim_client_t **client, fd_t *fd);
int xim_client_free(xim_client_t **client);
int xim_client_get_im(xim_client_t *client, const int id, input_method_t **im);
int xim_client_get_ic(xim_client_t *client, const int id, input_context_t **ic);

int xim_client_commit(xim_client_t *client, const int im, const int ic,
                      const void *data, const size_t data_len);

#endif /* XIMCLIENT_H */
