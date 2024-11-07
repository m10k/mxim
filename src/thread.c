/*
 * thread.c - This file is part of mxim
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
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#define FLAG_RUNNING (1 << 8)
#define FLAG_STOP    (1 << 9)

struct thread {
	mutex_t lock;
	unsigned int flags;
	pthread_t thread;

	void* (*func)(void*);
	void *arg;
	void *ret_val;
};

#define LOCK(th)   mutex_lock(&(th)->lock)
#define UNLOCK(th) mutex_unlock(&(th)->lock)

#ifdef _GNU_SOURCE
int mutex_init(mutex_t *mutex)
{
	pthread_mutexattr_t attr;
	int err;

	if (!(err = -pthread_mutexattr_init(&attr))) {
		if (!(err = -pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP))) {
			err = -pthread_mutex_init(mutex, &attr);
		}

		pthread_mutexattr_destroy(&attr);
	}

	return err;
}
#endif /* _GNU_SOURCE */

int thread_new(thread_t **thr)
{
	thread_t *t;

	if (!thr) {
		return -EINVAL;
	}

	if (!(t = calloc(1, sizeof(*t)))) {
		return -ENOMEM;
	}

	assert(mutex_init(&t->lock) == 0);

	*thr = t;
	return 0;
}

int thread_free(thread_t **thr)
{
	if (!thr || !*thr) {
		return -EINVAL;
	}

	thread_join(*thr, NULL);

	assert(mutex_destroy(&(*thr)->lock) == 0);
	free(*thr);
	*thr = NULL;

	return 0;
}

static void *_thread_run(thread_t *thr)
{
	void* (*func)(void*);
	void *arg;
	void *ret_val;

	assert(thr);

	LOCK(thr);
	thr->flags = FLAG_RUNNING;
	func = thr->func;
	arg = thr->arg;
	UNLOCK(thr);

	assert(func);
        ret_val = func(arg);

        LOCK(thr);
        thr->flags = 0;
        UNLOCK(thr);

        return ret_val;
}

int thread_start(thread_t *thr, void* (*func)(void*), void *arg)
{
	int err;

	if (!thr || !func) {
		return -EINVAL;
	}

	LOCK(thr);

	if (thr->flags & FLAG_RUNNING) {
		err = -EALREADY;
	} else {
		thr->func = func;
		thr->arg = arg;
		err = -pthread_create(&thr->thread, NULL, (void*(*)(void*))_thread_run, thr);
	}

	UNLOCK(thr);

	return err;
}

int thread_stop(thread_t *thr)
{
	int err;

	if (!thr) {
		return -EINVAL;
	}

	err = -EALREADY;

	LOCK(thr);
	if (thr->flags & FLAG_RUNNING) {
		thr->flags |= FLAG_STOP;
		err = 0;
	}
	UNLOCK(thr);

	return err;
}

int thread_cancel(thread_t *thr, void **ret)
{
	int err;

	if (!thr) {
		return -EINVAL;
	}

	LOCK(thr);
	if (thr->flags & FLAG_RUNNING) {
		err = -pthread_cancel(thr->thread);
	} else {
		err = -EALREADY;
	}
	UNLOCK(thr);

	if (err >= 0) {
		err = thread_join(thr, ret);
	}

	return err;
}

int thread_join(thread_t *thr, void **ret)
{
	int err;

	if (!thr) {
		return -EINVAL;
	}

	LOCK(thr);
	err = -pthread_join(thr->thread, &thr->ret_val);
	if (!err && ret) {
		*ret = thr->ret_val;
	}
	UNLOCK(thr);

	return err;
}

int thread_is_stopping(thread_t *thr)
{
	int err;

	if (!thr) {
		return -EINVAL;
	}

	LOCK(thr);
	err = thr->flags & FLAG_STOP ? 1 : 0;
	UNLOCK(thr);

	return err;
}
