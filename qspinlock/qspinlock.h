/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */

/* include/asm-generic/qspinlock.h */

/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Queued spinlock
 *
 * (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
 * (C) Copyright 2015 Hewlett-Packard Enterprise Development LP
 *
 * Authors: Waiman Long <waiman.long@hpe.com>
 */
#ifndef __ASM_GENERIC_QSPINLOCK_H
#define __ASM_GENERIC_QSPINLOCK_H

/* XXX: user thread index, stored in the cpu field of lock->tail */
#define MAX_USER_THREADS	64	/* can be (16384 - 2) at most */

#include "include/misc.h"
#include "include/atomic/atomic.h"
#include "include/qspinlock_types.h"
#include "include/mcs_spinlock.h"

struct qnode {
	struct mcs_spinlock mcs;
	// thread index for debugging purpose
	int thread_index;
} __aligned(64);

/**
 * queued_spin_is_locked - is the spinlock locked?
 * @lock: Pointer to queued spinlock structure
 * Return: 1 if it is locked, 0 otherwise
 */
static __always_inline int queued_spin_is_locked(struct qspinlock *lock)
{
	/*
	 * Any !0 state indicates it is locked, even if _Q_LOCKED_VAL
	 * isn't immediately observable.
	 */
	return atomic_read(&lock->val);
}

/**
 * queued_spin_value_unlocked - is the spinlock structure unlocked?
 * @lock: queued spinlock structure
 * Return: 1 if it is unlocked, 0 otherwise
 *
 * N.B. Whenever there are tasks waiting for the lock, it is considered
 *      locked wrt the lockref code to avoid lock stealing by the lockref
 *      code and change things underneath the lock. This also allows some
 *      optimizations to be applied without conflict with lockref.
 */
static __always_inline int queued_spin_value_unlocked(struct qspinlock lock)
{
	return !atomic_read(&lock.val);
}

/**
 * queued_spin_is_contended - check if the lock is contended
 * @lock : Pointer to queued spinlock structure
 * Return: 1 if lock contended, 0 otherwise
 */
static __always_inline int queued_spin_is_contended(struct qspinlock *lock)
{
	return atomic_read(&lock->val) & ~_Q_LOCKED_MASK;
}

/**
 * queued_spin_trylock - try to acquire the queued spinlock
 * @lock : Pointer to queued spinlock structure
 * Return: 1 if lock acquired, 0 if failed
 */
static __always_inline int queued_spin_trylock(struct qspinlock *lock)
{
	u32 val = atomic_read(&lock->val);

	if (unlikely(val))
		return 0;

	return likely(atomic_try_cmpxchg_acquire(&lock->val, &val, _Q_LOCKED_VAL));
}

extern void queued_spin_lock_slowpath(struct qspinlock *lock, u32 val);

/**
 * queued_spin_lock - acquire a queued spinlock
 * @lock: Pointer to queued spinlock structure
 */
static __always_inline void queued_spin_lock(struct qspinlock *lock)
{
	u32 val = 0;

	if (likely(atomic_try_cmpxchg_acquire(&lock->val, &val, _Q_LOCKED_VAL)))
		return;

	queued_spin_lock_slowpath(lock, val);
}

/**
 * queued_spin_unlock - release a queued spinlock
 * @lock : Pointer to queued spinlock structure
 */
static __always_inline void queued_spin_unlock(struct qspinlock *lock)
{
	/*
	 * unlock() needs release semantics:
	 */
	smp_store_release(&lock->locked, 0);
}

#endif /* __ASM_GENERIC_QSPINLOCK_H */
