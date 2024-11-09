/*
 * in4.c - This file is part of mxim
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

#include "socket.h"
#include "in4.h"
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define IN4_DEFAULT_BACKLOG 4

static int     _in4_open(fd_t *fd, va_list args);
static int     _in4_close(fd_t *fd);
static ssize_t _in4_read(fd_t *fd, void *dst, const size_t dst_size);
static ssize_t _in4_write(fd_t *fd, const void *src, const size_t src_len);
static int     _in4_accept(fd_t *server, fd_t **client);

static struct fd_ops _in4_ops = {
	.open   = _in4_open,
	.close  = _in4_close,
	.read   = _in4_read,
	.write  = _in4_write,
	.accept = _in4_accept
};

struct fd_dom _dom_in4 = {
	.dom = FD_DOM_IN4,
	.ops = &_in4_ops
};

struct in4_priv {
	struct sockaddr_in addr;
};

static int _in4_open_sock(struct in4_priv *priv)
{
	int ret_val;
	int sock;

	ret_val = -EINVAL;
	sock = -1;

	if (priv) {
		if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
			ret_val = -errno;
			perror("socket");
		} else if (bind(sock, (struct sockaddr*)&priv->addr,
		                sizeof(priv->addr)) < 0) {
			ret_val = -errno;
			perror("bind");
		} else if (listen(sock, IN4_DEFAULT_BACKLOG) < 0) {
			ret_val = -errno;
			perror("bind");
		} else {
			ret_val = sock;
		}

		if (ret_val < 0 && sock >= 0) {
			close(sock);
		}
	}

	return ret_val;
}

static int _in4_open(fd_t *fd, va_list args)
{
	int ret_val;
	struct in4_priv *priv;
	const char *addr;
	unsigned short port;
	int sock;

	if (!fd) {
		return -EINVAL;
	}

	ret_val = 0;
	sock = -1;
	addr = (const char*)va_arg(args, char*);
	port = (unsigned short)va_arg(args, int);

	if (!(priv = calloc(1, sizeof(*priv)))) {
		return -ENOMEM;
	}

	priv->addr.sin_family = AF_INET;
	priv->addr.sin_port = htons(port);

	if (inet_pton(AF_INET, addr, &priv->addr.sin_addr) < 0) {
		ret_val = -errno;
	} else if ((sock = _in4_open_sock(priv)) < 0) {
		ret_val = sock;
	} else {
		fd_lock(fd);
		fd->fd = sock;
		fd->priv = priv;
		fd->addr = (struct sockaddr*)&priv->addr;
		fd->addrlen = sizeof(priv->addr);
		fd_unlock(fd);
	}

	if (ret_val < 0) {
		if (sock >= 0) {
			close(sock);
		}
		free(priv);
	}

	return ret_val;
}

static ssize_t _in4_read(fd_t *fd, void *dst, const size_t dst_size)
{
	ssize_t ret_val;

	ret_val = (ssize_t)-EINVAL;

	if (fd && dst) {
		fd_lock(fd);
		ret_val = read(fd->fd, dst, dst_size);
		fd_unlock(fd);
	}

	return ret_val;
}

static ssize_t _in4_write(fd_t *fd, const void *src, const size_t src_len)
{
	ssize_t ret_val;

	ret_val = (ssize_t)-EINVAL;

	if (fd && src) {
		fd_lock(fd);
		ret_val = write(fd->fd, src, src_len);
		fd_unlock(fd);
	}

	return ret_val;
}

static int _in4_accept(fd_t *server, fd_t **client)
{
	fd_t *new_fd;
	struct in4_priv *priv;
	int ret_val;

	new_fd = NULL;
	priv = NULL;
	ret_val = 0;

	if (!server || !client) {
		ret_val = -EINVAL;
	} else if (!(new_fd = calloc(1, sizeof(*new_fd))) ||
	           !(priv = calloc(1, sizeof(*priv)))) {
		ret_val = -ENOMEM;
	} else {
		assert(mutex_init(&new_fd->lock) == 0);

		new_fd->addr = (struct sockaddr*)&priv->addr;
		new_fd->addrlen = sizeof(priv->addr);
		new_fd->priv = priv;

		fd_lock(server);
		new_fd->dom = server->dom;
		new_fd->ops = server->ops;
		memcpy(&new_fd->handlers, &server->handlers, sizeof(new_fd->handlers));

		if ((new_fd->fd = accept(server->fd, new_fd->addr, &new_fd->addrlen)) < 0) {
			ret_val = -errno;
		}
		fd_unlock(server);
	}

	if (ret_val < 0) {
		if (new_fd) {
			assert(mutex_destroy(&new_fd->lock) == 0);
			free(new_fd);
		}
		if (priv) {
			free(priv);
		}
	} else {
		*client = new_fd;
	}

	return ret_val;
}

static int _in4_close(fd_t *fd)
{
	if (!fd) {
		return -EINVAL;
	}

	if (fd->priv) {
		free(fd->priv);
		fd->priv = NULL;
	}

	return 0;
}
