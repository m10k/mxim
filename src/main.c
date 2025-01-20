/*
 * main.c - This file is part of mxim
 * Copyright (C) 2024-2025 Matthias Kruk
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

#include "aide.h"
#include "xhandler.h"
#include "ximserver.h"
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

#define MXIM_ADDR "127.0.0.1"
#define MXIM_PORT 1234
static const char *_cmd_flags = "h";

static struct option _cmd_opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ 0, 0, 0, 0 }
};

static void _print_usage(const char *name)
{
	fprintf(stderr,
	        "Usage: %s options\n"
	        "\n"
	        " -h  --help    Display this text\n",
	        name);
}

x_handler_t *xhandler;

int main(int argc, char *argv[])
{
	xim_server_t *server;
	int ret;

	do {
		ret = getopt_long(argc, argv, _cmd_flags, _cmd_opts, NULL);

		switch (ret) {
		case 'h':
			_print_usage(argv[0]);
			return 1;

		case '?':
			fprintf(stderr, "Unrecognized command line option '%s'\n", optarg);
			return 1;

		default:
			ret = -1;
			break;
		}
	} while (ret >= 0);

	ret = aide_init();
	if (ret < 0) {
		fprintf(stderr, "Could not initialize aide: %s\n", strerror(-ret));
		return 5;
	}
#if MXIM_DEBUG
	fprintf(stderr, "Aide initialized\n");
#endif /* MXIM_DEBUG */

	ret = x_handler_init(&xhandler);
	if (ret < 0) {
		fprintf(stderr, "Could not initialize IM handler: %s\n", strerror(-ret));
		return 2;
	}

	ret = xim_server_init(&server, MXIM_ADDR, MXIM_PORT);
	if (ret < 0) {
		fprintf(stderr, "Could not initialize XIM server: %s\n", strerror(-ret));
		return 3;
	}

	ret = xim_server_start(server);
	if (ret < 0) {
		fprintf(stderr, "Could not start XIM server: %s\n", strerror(-ret));
		return 4;
	}

	x_handler_run(xhandler);
	ret = xim_server_stop(server);

	x_handler_free(&xhandler);
	xim_server_free(&server);

	return ret;
}
