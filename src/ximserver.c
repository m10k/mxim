/*
 * ximserver.c - This file is part of mxim
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

#include "thread.h"
#include "ximserver.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct {
	int fd;
	struct sockaddr_in addr;
} xim_client_t;

struct xim_server {
	thread_t *thread;

	int fd;
	int epfd;
	struct sockaddr_in addr;
};

static int _watch_fd(int epfd, int fd, void *data)
{
	struct epoll_event ev;
	int err;

	ev.events = EPOLLIN;
	ev.data.ptr = data;
	err = 0;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
		err = -errno;
	}

	return err;
}

int xim_server_init(xim_server_t **server, const char *addr, unsigned short port)
{
	xim_server_t *srv;
	int err;

	if (!(srv = calloc(1, sizeof(*srv)))) {
		err = -ENOMEM;
		goto cleanup;
	}

	if ((err = thread_new(&srv->thread)) < 0) {
		fprintf(stderr, "thread_new: %s\n", strerror(-err));
		goto cleanup;
	}

	if ((srv->epfd = epoll_create1(EPOLL_CLOEXEC)) < 0) {
		err = -errno;
		perror("epoll_create1");
		goto cleanup;
	}

	srv->addr.sin_family = AF_INET;
	srv->addr.sin_port = htons(port);
	if (inet_pton(AF_INET, addr, &srv->addr.sin_addr) < 0) {
		err = -errno;
		perror("inet_pton");
		goto cleanup;
	}

	if ((srv->fd = socket(PF_INET, SOCK_STREAM, 0)) < 0 ||
	    bind(srv->fd, (struct sockaddr*)&srv->addr, sizeof(srv->addr)) < 0 ||
	    listen(srv->fd, 8) < 0) {
		err = -errno;
		goto cleanup;
	}
	fprintf(stderr, "Listening on [%s]:%hu\n", addr, port);

	if ((err = _watch_fd(srv->epfd, srv->fd, srv)) < 0) {
		goto cleanup;
	}

cleanup:
	if (!err) {
		*server = srv;
	} else {
		xim_server_free(&srv);
	}

	return err;
}

int xim_server_free(xim_server_t **server)
{
	if (!server || !*server) {
		return -EINVAL;
	}

	if ((*server)->thread) {
		thread_free(&(*server)->thread);
	}

	if ((*server)->epfd >= 0) {
		close((*server)->epfd);
		(*server)->epfd = -1;
	}
	if ((*server)->fd >= 0) {
		close((*server)->fd);
		(*server)->fd = -1;
	}

	free(*server);
	*server = NULL;

	return 0;
}

static int _xim_server_accept(xim_server_t *server)
{
	xim_client_t *client;
	socklen_t addrlen;
	int err;

	if (!(client = calloc(1, sizeof(*client)))) {
		return -ENOMEM;
	}

	addrlen = sizeof(client->addr);
	if ((client->fd = accept(server->fd, (struct sockaddr*)&client->addr, &addrlen)) < 0) {
		err = -errno;
	} else {
		err = _watch_fd(server->epfd, client->fd, client);
	}

	if (err) {
		free(client);
	}

	return err;
}

int _handle_client_event(xim_client_t *client)
{
	fprintf(stderr, "Client event on %p\n", client);
	return -ENOSYS;
}

static void* _xim_server_run(xim_server_t *server)
{
	int nev;

	while (!thread_is_stopping(server->thread)) {
		struct epoll_event events[8];

		fprintf(stderr, "Waiting for events...\n");
		nev = epoll_wait(server->epfd, events, sizeof(events) / sizeof(events[0]), -1);

		while (--nev >= 0) {
			fprintf(stderr, "Event on %p\n", events[nev].data.ptr);

			if (events[nev].data.ptr == server) {
				_xim_server_accept(server);
			} else {
				_handle_client_event((xim_client_t*)events[nev].data.ptr);
			}
		}
	}

	return NULL;
}

int xim_server_start(xim_server_t *server)
{
	if (!server) {
		return -EINVAL;
	}

	return thread_start(server->thread, (void*(*)(void*))_xim_server_run, server);
}

int xim_server_stop(xim_server_t *server)
{
	if (!server) {
		return -EINVAL;
	}

	if (!server->thread) {
		return -EBADFD;
	}

	return thread_stop(server->thread);
}
