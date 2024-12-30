/*
 * cmd.h - This file is part of mxim
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

#ifndef CMD_H
#define CMD_H

typedef enum {
	CMD_NOP = 0,          /* none          */
	CMD_LANG_CYCLE,       /* int      dir  */
	CMD_LANG_SELECT,      /* unsigned idx  */
	CMD_CURSOR_MOVE,      /* int      dir  */
	CMD_CANDIDATE_MOVE,   /* int      dir  */
	CMD_CANDIDATE_SELECT, /* unsigned idx  */
	CMD_SEGMENT_MOVE,     /* int      dir  */
	CMD_SEGMENT_RESIZE,   /* int      size */
	CMD_SEGMENT_NEW,      /* none          */
	CMD_DELETE,           /* int      dir  */
	CMD_COMMIT,           /* none          */
	CMD_LAST
} cmd_t;

typedef union {
	void *p;
	int i;
	unsigned u;
} cmd_arg_t;

typedef struct {
	cmd_t cmd;
	cmd_arg_t arg;
} cmd_def_t;

typedef int (cmd_func_t)(void *, void *, cmd_arg_t *);

#endif /* CMD_H */
