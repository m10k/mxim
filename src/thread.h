/*
 * thread.h - This file is part of mxim
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

#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

typedef pthread_mutex_t mutex_t;
typedef sem_t           semaphore_t;
typedef struct thread   thread_t;

#ifdef _GNU_SOURCE
int mutex_init(mutex_t *mutex);
#else /* !_GNU_SOURCE */
#define mutex_init(mutex)       (-pthread_mutex_init(mutex, NULL))
#endif /* !_GNU_SOURCE */
#define mutex_destroy(mutex)    (-pthread_mutex_destroy(mutex))

#define mutex_lock(mutex)       assert(pthread_mutex_lock(mutex) == 0)
#define mutex_unlock(mutex)     assert(pthread_mutex_unlock(mutex) == 0)

#define semaphore_init(sem,val) assert(sem_init((sem), 0, (val)) == 0)
#define semaphore_destroy(sem)  assert(sem_destroy(sem) == 0)
#define semaphore_wait(sem)     assert(sem_wait(sem) == 0)
#define semaphore_post(sem)     assert(sem_post(sem) == 0)

int thread_new(thread_t **thr);
int thread_free(thread_t **thr);

int thread_start(thread_t *thr, void*(*func)(void*), void *arg);
int thread_stop(thread_t *thr);
int thread_cancel(thread_t *thr, void **retval);
int thread_join(thread_t *thr, void **retval);
int thread_is_stopping(thread_t *thr);

#endif /* THREAD_H */
