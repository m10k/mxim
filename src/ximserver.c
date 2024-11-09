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

#include "fd.h"
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
	fd_t *fd;
} xim_client_t;

struct xim_server {
	fd_t *fd;
	thread_t *thread;

	int epfd;
};

static int _watch_fd(int epfd, fd_t *fd)
{
	struct epoll_event ev;
	int err;

	ev.events = EPOLLIN;
	ev.data.ptr = fd;
	err = 0;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd->fd, &ev) < 0) {
		err = -errno;
	}

	return err;
}

static void _xim_client_in(fd_t *fd, fd_event_t event, xim_client_t *client, void *data)
{
	return;
}

int xim_client_free(xim_client_t **client)
{
	if (!client || !*client) {
		return -EINVAL;
	}

	if ((*client)->fd) {
		fd_free(&(*client)->fd);
	}
	free(*client);
	*client = NULL;

	return 0;
}

static void _xim_server_in(fd_t *fd, fd_event_t event, xim_server_t *server, void *data)
{
	xim_client_t *ximclient;
	fd_t *client;
	int err;

	if ((err = fd_accept(fd, &client)) < 0) {
		fprintf(stderr, "fd_accept: %s\n", strerror(-err));
		return;
	}

	if (!(ximclient = calloc(1, sizeof(*ximclient)))) {
		perror("calloc");
		fd_free(&fd);
		return;
	}

	ximclient->fd = client;
	client->userdata = ximclient;
	fd_set_callback(client, FD_EVENT_IN, (fd_callback_t*)_xim_client_in, ximclient);

	if ((err = _watch_fd(server->epfd, ximclient->fd)) < 0) {
		fprintf(stderr, "_watch_fd: %s\n", strerror(-err));
		xim_client_free(&ximclient);
	}

	return;
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

	if ((err = fd_open(&srv->fd, FD_DOM_IN4, addr, port)) < 0) {
		goto cleanup;
	}

	if ((err = _watch_fd(srv->epfd, srv->fd)) < 0) {
		goto cleanup;
	}

	srv->fd->userdata = srv;
	fd_set_callback(srv->fd, FD_EVENT_IN, (fd_callback_t*)_xim_server_in, srv);

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
		fd_free(&(*server)->fd);
	}

	free(*server);
	*server = NULL;

	return 0;
}

static void* _xim_server_run(xim_server_t *server)
{
	int nev;

	while (!thread_is_stopping(server->thread)) {
		struct epoll_event events[8];

		nev = epoll_wait(server->epfd, events, sizeof(events) / sizeof(events[0]), -1);

		while (--nev >= 0) {
			fd_t *fd;

			fd = events[nev].data.ptr;

			if (events[nev].events & EPOLLIN) {
				fd_notify(fd, FD_EVENT_IN, NULL);
			}
			if (events[nev].events & EPOLLERR) {
				fd_notify(fd, FD_EVENT_ERR, NULL);
			}
			if (events[nev].events & EPOLLHUP) {
				fd_notify(fd, FD_EVENT_HUP, NULL);
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
