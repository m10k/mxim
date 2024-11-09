/*
 * fd.h - This file is part of mxim
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

#ifndef FD_H
#define FD_H

#include "thread.h"
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef struct fd fd_t;

typedef enum {
	FD_EVENT_IN = 0,
	FD_EVENT_ERR,
	FD_EVENT_HUP,
	FD_EVENT_NUM
} fd_event_t;

typedef enum {
	FD_DOM_IN4,
	FD_DOM_X11,
	FD_DOM_NUM,
} fd_dom_t;

typedef enum {
	FD_TYPE_SERVER = 0,
	FD_TYPE_CLIENT,
	FD_TYPE_NUM
} fd_type_t;

struct fd_ops {
	int     (*open)(fd_t*, va_list);
	ssize_t (*read)(fd_t*, void*, const size_t);
	ssize_t (*write)(fd_t*, const void*, const size_t);
	int     (*accept)(fd_t*, fd_t**);
	int     (*close)(fd_t*);
};

struct fd_dom {
	fd_dom_t dom;
	struct fd_ops *ops;
};

typedef void (fd_callback_t)(fd_t*, fd_event_t, void*, void*);

struct fd_event_handler {
	fd_callback_t *callback;
	void *data;
};

struct fd {
	int fd;
	mutex_t lock;

	struct fd_ops *ops;
	struct sockaddr *addr;
	socklen_t addrlen;

	fd_dom_t dom;
	void *priv;
	void *userdata;

	struct fd_event_handler handlers[FD_EVENT_NUM];
};

#define fd_dom_is_valid(dom)     ((dom)   >= 0 && (dom)   < FD_DOM_NUM)
#define fd_type_is_valid(type)   ((type)  >= 0 && (type)  < FD_TYPE_NUM)
#define fd_event_is_valid(event) ((event) >= 0 && (event) < FD_EVENT_NUM)

#define fd_lock(fd)   mutex_lock(&(fd)->lock)
#define fd_unlock(fd) mutex_unlock(&(fd)->lock)

int     fd_open(fd_t **fd, fd_dom_t dom, ...);
int     fd_free(fd_t **fd);
int     fd_close(fd_t *fd);
ssize_t fd_read(fd_t *fd, void *dst, const size_t dst_size);
ssize_t fd_write(fd_t *fd, const void *src, const size_t src_len);
int     fd_accept(fd_t *server, fd_t **client);
int     fd_set_callback(fd_t *fd, fd_event_t event,
                        fd_callback_t *handler, void *data);
int     fd_notify(fd_t *fd, fd_event_t event, void *arg);

#endif /* FD_H */
