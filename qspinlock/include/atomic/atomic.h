/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */

/* arch/arm64/include/asm/atomic.h */
/* include/linux/atomic.h */
/* include/linux/atomic_fallback.h */

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/atomic.h
 *
 * Copyright (C) 1996 Russell King.
 * Copyright (C) 2002 Deep Blue Solutions Ltd.
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_ATOMIC_H
#define __ASM_ATOMIC_H

#include <stdbool.h>

#define __LSE_PREAMBLE  ".arch_extension lse\n"

#include "barrier.h"
#ifdef CONFIG_ARM64_LSE_ATOMICS
#include "atomic_lse.h"
#else
#include "atomic_ll_sc.h"
#endif
#include "cmpxchg.h"

/* arch/arm64/include/asm/atomic.h */

#define ATOMIC_OP(op)							\
static inline void arm_##op(int i, atomic_t *v)				\
{									\
	__lse_ll_sc_body(op, i, v);					\
}

ATOMIC_OP(atomic_andnot)
ATOMIC_OP(atomic_or)
ATOMIC_OP(atomic_xor)
ATOMIC_OP(atomic_add)
ATOMIC_OP(atomic_and)
ATOMIC_OP(atomic_sub)
#define atomic_andnot	arm_atomic_andnot
#define atomic_or	arm_atomic_or
#define atomic_xor	arm_atomic_xor
#define atomic_add	arm_atomic_add
#define atomic_and	arm_atomic_and
#define atomic_sub	arm_atomic_sub

#undef ATOMIC_OP

#define ATOMIC_FETCH_OP(name, op)					\
static inline int arm_##op##name(int i, atomic_t *v)			\
{									\
	return __lse_ll_sc_body(op##name, i, v);			\
}

#define ATOMIC_FETCH_OPS(op)						\
	ATOMIC_FETCH_OP(_relaxed, op)					\
	ATOMIC_FETCH_OP(_acquire, op)					\
	ATOMIC_FETCH_OP(_release, op)					\
	ATOMIC_FETCH_OP(        , op)

ATOMIC_FETCH_OPS(atomic_fetch_andnot)
ATOMIC_FETCH_OPS(atomic_fetch_or)
ATOMIC_FETCH_OPS(atomic_fetch_xor)
ATOMIC_FETCH_OPS(atomic_fetch_add)
ATOMIC_FETCH_OPS(atomic_fetch_and)
ATOMIC_FETCH_OPS(atomic_fetch_sub)
ATOMIC_FETCH_OPS(atomic_add_return)
ATOMIC_FETCH_OPS(atomic_sub_return)

#define atomic_fetch_andnot		arm_atomic_fetch_andnot
#define atomic_fetch_andnot_relaxed	arm_atomic_fetch_andnot_relaxed
#define atomic_fetch_andnot_release	arm_atomic_fetch_andnot_release
#define atomic_fetch_andnot_acquire	arm_atomic_fetch_andnot_acquire
#define atomic_fetch_or			arm_atomic_fetch_or
#define atomic_fetch_or_relaxed		arm_atomic_fetch_or_relaxed
#define atomic_fetch_or_release		arm_atomic_fetch_or_release
#define atomic_fetch_or_acquire		arm_atomic_fetch_or_acquire
#define atomic_fetch_xor		arm_atomic_fetch_xor
#define atomic_fetch_xor_relaxed	arm_atomic_fetch_xor_relaxed
#define atomic_fetch_xor_release	arm_atomic_fetch_xor_release
#define atomic_fetch_xor_acquire	arm_atomic_fetch_xor_acquire
#define atomic_fetch_add		arm_atomic_fetch_add
#define atomic_fetch_add_relaxed	arm_atomic_fetch_add_relaxed
#define atomic_fetch_add_release	arm_atomic_fetch_add_release
#define atomic_fetch_add_acquire	arm_atomic_fetch_add_acquire
#define atomic_fetch_and		arm_atomic_fetch_and
#define atomic_fetch_and_relaxed	arm_atomic_fetch_and_relaxed
#define atomic_fetch_and_release	arm_atomic_fetch_and_release
#define atomic_fetch_and_acquire	arm_atomic_fetch_and_acquire
#define atomic_fetch_sub		arm_atomic_fetch_sub
#define atomic_fetch_sub_relaxed	arm_atomic_fetch_sub_relaxed
#define atomic_fetch_sub_release	arm_atomic_fetch_sub_release
#define atomic_fetch_sub_acquire	arm_atomic_fetch_sub_acquire
#define atomic_add_return		arm_atomic_add_return
#define atomic_add_return_relaxed	arm_atomic_add_return_relaxed
#define atomic_add_return_release	arm_atomic_add_return_release
#define atomic_add_return_acquire	arm_atomic_add_return_acquire
#define atomic_sub_return		arm_atomic_sub_return
#define atomic_sub_return_relaxed	arm_atomic_sub_return_relaxed
#define atomic_sub_return_release	arm_atomic_sub_return_release
#define atomic_sub_return_acquire	arm_atomic_sub_return_acquire

#undef ATOMIC_FETCH_OP
#undef ATOMIC_FETCH_OPS

#define atomic_read(v)			READ_ONCE((v)->counter)
#define atomic_set(v, i)		WRITE_ONCE(((v)->counter), (i))

#define atomic_xchg_relaxed(v, new) \
	xchg_relaxed(&((v)->counter), (new))
#define atomic_xchg_acquire(v, new) \
	xchg_acquire(&((v)->counter), (new))
#define atomic_xchg_release(v, new) \
	xchg_release(&((v)->counter), (new))
#define atomic_xchg(v, new) \
	xchg(&((v)->counter), (new))

#define atomic_cmpxchg_relaxed(v, old, new) \
	cmpxchg_relaxed(&((v)->counter), (old), (new))
#define atomic_cmpxchg_acquire(v, old, new) \
	cmpxchg_acquire(&((v)->counter), (old), (new))
#define atomic_cmpxchg_release(v, old, new) \
	cmpxchg_release(&((v)->counter), (old), (new))
#define atomic_cmpxchg(v, old, new) \
	cmpxchg(&((v)->counter), (old), (new))


/* include/linux/atomic.h */
#define atomic_cond_read_acquire(v, c) smp_cond_load_acquire(&(v)->counter, (c))
#define atomic_cond_read_relaxed(v, c) smp_cond_load_relaxed(&(v)->counter, (c))

/* include/linux/atomic_fallback.h */
static __always_inline bool
atomic_try_cmpxchg(atomic_t *v, int *old, int new)
{
	int r, o = *old;
	r = atomic_cmpxchg(v, o, new);
	if (unlikely(r != o))
		*old = r;
	return likely(r == o);
}

static __always_inline bool
atomic_try_cmpxchg_acquire(atomic_t *v, int *old, int new)
{
	int r, o = *old;
	r = atomic_cmpxchg_acquire(v, o, new);
	if (unlikely(r != o))
		*old = r;
	return likely(r == o);
}

static __always_inline bool
atomic_try_cmpxchg_release(atomic_t *v, int *old, int new)
{
	int r, o = *old;
	r = atomic_cmpxchg_release(v, o, new);
	if (unlikely(r != o))
		*old = r;
	return likely(r == o);
}

static __always_inline bool
atomic_try_cmpxchg_relaxed(atomic_t *v, int *old, int new)
{
	int r, o = *old;
	r = atomic_cmpxchg_relaxed(v, o, new);
	if (unlikely(r != o))
		*old = r;
	return likely(r == o);
}

#endif /* __ASM_ATOMIC_H */
