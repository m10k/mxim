/*
 * fd.c - This file is part of mxim
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
#include "thread.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

extern struct fd_dom _dom_in4;

static struct fd_dom *_doms[] = {
	&_dom_in4
};

int fd_open(fd_t **dst, fd_dom_t dom, ...)
{
	int err;
	fd_t *fd;
	va_list args;

	if (!dst || !fd_dom_is_valid(dom)) {
		return -EINVAL;
	}

	if (!(fd = calloc(1, sizeof(*fd)))) {
		return -ENOMEM;
	}

	assert(mutex_init(&fd->lock) == 0);
	fd->fd = -1;

	va_start(args, dom);
	err = _doms[dom]->ops->open(fd, args);
	va_end(args);

	if (err < 0) {
		fd_free(&fd);
	} else {
		fd->dom = dom;
		fd->ops = _doms[dom]->ops;
		*dst = fd;
	}

	return err;
}

int fd_free(fd_t **fd)
{
	int err;

	if (!fd || !*fd) {
		return -EINVAL;
	}

	err = fd_close(*fd);
	free(*fd);
	*fd = NULL;

	return err;
}

int fd_close(fd_t *fd)
{
	int err;

	if (!fd) {
		return -EINVAL;
	}

	err = -EBADFD;

	fd_lock(fd);
	if (fd->fd >= 0) {
		err = fd->ops->close(fd);
		close(fd->fd);
		fd->fd = -1;
	}
	fd_unlock(fd);

	return err;
}

ssize_t fd_read(fd_t *fd, void *dst, const size_t dst_size)
{
	ssize_t ret_val;

	if (!fd || !dst) {
		ret_val = (ssize_t)-EINVAL;
	} else if(!fd->ops->read) {
		ret_val = (ssize_t)-EOPNOTSUPP;
	} else {
		ret_val = fd->ops->read(fd, dst, dst_size);
	}

	return ret_val;
}

ssize_t fd_write(fd_t *fd, const void *src, const size_t src_len)
{
	ssize_t ret_val;

	if (!fd || !src) {
		ret_val = (ssize_t)-EINVAL;
	} else if(!fd->ops->write) {
		ret_val = (ssize_t)-EOPNOTSUPP;
	} else {
		ret_val = fd->ops->write(fd, src, src_len);
	}

	return ret_val;
}

int fd_accept(fd_t *src, fd_t **dst)
{
	int ret_val;

	if (!src || !dst) {
		ret_val = -EINVAL;
	} else if(!src->ops->accept) {
		ret_val = -EOPNOTSUPP;
	} else {
		ret_val = src->ops->accept(src, dst);
	}

	return ret_val;
}

int fd_get_fd(fd_t *fd)
{
	int ret_val;

	ret_val = -EINVAL;

	if (fd) {
	        fd_lock(fd);
		ret_val = fd->fd;
		fd_unlock(fd);
	}

	return ret_val;
}

int fd_set_callback(fd_t *fd, fd_event_t event, fd_callback_t *handler, void *data)
{
	if (!fd || !fd_event_is_valid(event)) {
		return -EINVAL;
	}

	fd_lock(fd);
	fd->handlers[event].callback = handler;
	fd->handlers[event].data = data;
	fd_unlock(fd);

	return 0;
}

int fd_notify(fd_t *fd, fd_event_t ev, void *arg)
{
	int ret_val;

	ret_val = -EINVAL;

	if (fd && fd_event_is_valid(ev)) {
		struct fd_event_handler handler;

		fd_lock(fd);
		handler = fd->handlers[ev];
		fd_unlock(fd);

		if (handler.callback) {
			handler.callback(fd, ev, handler.data, arg);
		}

		ret_val = 0;
	}

	return ret_val;
}
