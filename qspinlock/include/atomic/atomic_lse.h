/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */

/* arch/arm64/include/asm/atomic_lse.h */

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/atomic.h
 *
 * Copyright (C) 1996 Russell King.
 * Copyright (C) 2002 Deep Blue Solutions Ltd.
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_ATOMIC_LSE_H
#define __ASM_ATOMIC_LSE_H

#define __lse_ll_sc_body(op, ...)	__lse_##op(__VA_ARGS__)

#define ATOMIC_OP(op, asm_op)						\
static inline void __lse_atomic_##op(int i, atomic_t *v)		\
{									\
	asm volatile(							\
        __LSE_PREAMBLE                                                  \
"	" #asm_op "	%w[i], %[v]\n"					\
	: [i] "+r" (i), [v] "+Q" (v->counter)				\
	: "r" (v));							\
}

ATOMIC_OP(andnot, stclr)
ATOMIC_OP(or, stset)
ATOMIC_OP(xor, steor)
ATOMIC_OP(add, stadd)

#undef ATOMIC_OP

#define ATOMIC_FETCH_OP(name, mb, op, asm_op, cl...)			\
static inline int __lse_atomic_fetch_##op##name(int i, atomic_t *v)	\
{									\
	asm volatile(							\
        __LSE_PREAMBLE                                                  \
"	" #asm_op #mb "	%w[i], %w[i], %[v]"				\
	: [i] "+r" (i), [v] "+Q" (v->counter)				\
	: "r" (v)							\
	: cl);								\
									\
	return i;							\
}

#define ATOMIC_FETCH_OPS(op, asm_op)					\
	ATOMIC_FETCH_OP(_relaxed,   , op, asm_op)			\
	ATOMIC_FETCH_OP(_acquire,  a, op, asm_op, "memory")		\
	ATOMIC_FETCH_OP(_release,  l, op, asm_op, "memory")		\
	ATOMIC_FETCH_OP(        , al, op, asm_op, "memory")

ATOMIC_FETCH_OPS(andnot, ldclr)
ATOMIC_FETCH_OPS(or, ldset)
ATOMIC_FETCH_OPS(xor, ldeor)
ATOMIC_FETCH_OPS(add, ldadd)

#undef ATOMIC_FETCH_OP
#undef ATOMIC_FETCH_OPS

#define ATOMIC_OP_ADD_RETURN(name, mb, cl...)				\
static inline int __lse_atomic_add_return##name(int i, atomic_t *v)	\
{									\
	u32 tmp;							\
									\
	asm volatile(							\
	__LSE_PREAMBLE							\
	"	ldadd" #mb "	%w[i], %w[tmp], %[v]\n"			\
	"	add	%w[i], %w[i], %w[tmp]"				\
	: [i] "+r" (i), [v] "+Q" (v->counter), [tmp] "=&r" (tmp)	\
	: "r" (v)							\
	: cl);								\
									\
	return i;							\
}

ATOMIC_OP_ADD_RETURN(_relaxed,   )
ATOMIC_OP_ADD_RETURN(_acquire,  a, "memory")
ATOMIC_OP_ADD_RETURN(_release,  l, "memory")
ATOMIC_OP_ADD_RETURN(        , al, "memory")

#undef ATOMIC_OP_ADD_RETURN

static inline void __lse_atomic_and(int i, atomic_t *v)
{
	asm volatile(
	__LSE_PREAMBLE
	"	mvn	%w[i], %w[i]\n"
	"	stclr	%w[i], %[v]"
	: [i] "+&r" (i), [v] "+Q" (v->counter)
	: "r" (v));
}

#define ATOMIC_FETCH_OP_AND(name, mb, cl...)				\
static inline int __lse_atomic_fetch_and##name(int i, atomic_t *v)	\
{									\
	asm volatile(							\
	__LSE_PREAMBLE							\
	"	mvn	%w[i], %w[i]\n"					\
	"	ldclr" #mb "	%w[i], %w[i], %[v]"			\
	: [i] "+&r" (i), [v] "+Q" (v->counter)				\
	: "r" (v)							\
	: cl);								\
									\
	return i;							\
}

ATOMIC_FETCH_OP_AND(_relaxed,   )
ATOMIC_FETCH_OP_AND(_acquire,  a, "memory")
ATOMIC_FETCH_OP_AND(_release,  l, "memory")
ATOMIC_FETCH_OP_AND(        , al, "memory")

#undef ATOMIC_FETCH_OP_AND

static inline void __lse_atomic_sub(int i, atomic_t *v)
{
	asm volatile(
	__LSE_PREAMBLE
	"	neg	%w[i], %w[i]\n"
	"	stadd	%w[i], %[v]"
	: [i] "+&r" (i), [v] "+Q" (v->counter)
	: "r" (v));
}

#define ATOMIC_OP_SUB_RETURN(name, mb, cl...)				\
static inline int __lse_atomic_sub_return##name(int i, atomic_t *v)	\
{									\
	u32 tmp;							\
									\
	asm volatile(							\
	__LSE_PREAMBLE							\
	"	neg	%w[i], %w[i]\n"					\
	"	ldadd" #mb "	%w[i], %w[tmp], %[v]\n"			\
	"	add	%w[i], %w[i], %w[tmp]"				\
	: [i] "+&r" (i), [v] "+Q" (v->counter), [tmp] "=&r" (tmp)	\
	: "r" (v)							\
	: cl);							\
									\
	return i;							\
}

ATOMIC_OP_SUB_RETURN(_relaxed,   )
ATOMIC_OP_SUB_RETURN(_acquire,  a, "memory")
ATOMIC_OP_SUB_RETURN(_release,  l, "memory")
ATOMIC_OP_SUB_RETURN(        , al, "memory")

#undef ATOMIC_OP_SUB_RETURN

#define ATOMIC_FETCH_OP_SUB(name, mb, cl...)				\
static inline int __lse_atomic_fetch_sub##name(int i, atomic_t *v)	\
{									\
	asm volatile(							\
	__LSE_PREAMBLE							\
	"	neg	%w[i], %w[i]\n"					\
	"	ldadd" #mb "	%w[i], %w[i], %[v]"			\
	: [i] "+&r" (i), [v] "+Q" (v->counter)				\
	: "r" (v)							\
	: cl);								\
									\
	return i;							\
}

ATOMIC_FETCH_OP_SUB(_relaxed,   )
ATOMIC_FETCH_OP_SUB(_acquire,  a, "memory")
ATOMIC_FETCH_OP_SUB(_release,  l, "memory")
ATOMIC_FETCH_OP_SUB(        , al, "memory")

#undef ATOMIC_FETCH_OP_SUB

/* XXX: silent gcc-12 complains about array bounds */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"

#define __CMPXCHG_CASE(w, sfx, name, sz, mb, cl...)			\
static __always_inline u##sz						\
__lse__cmpxchg_case_##name##sz(volatile void *ptr,			\
					      u##sz old,		\
					      u##sz new)		\
{									\
	register unsigned long x0 asm ("x0") = (unsigned long)ptr;	\
	register u##sz x1 asm ("x1") = old;				\
	register u##sz x2 asm ("x2") = new;				\
	unsigned long tmp;						\
									\
	asm volatile(							\
	__LSE_PREAMBLE							\
	"	mov	%" #w "[tmp], %" #w "[old]\n"			\
	"	cas" #mb #sfx "\t%" #w "[tmp], %" #w "[new], %[v]\n"	\
	"	mov	%" #w "[ret], %" #w "[tmp]"			\
	: [ret] "+r" (x0), [v] "+Q" (*(unsigned long *)ptr),		\
	  [tmp] "=&r" (tmp)						\
	: [old] "r" (x1), [new] "r" (x2)				\
	: cl);								\
									\
	return x0;							\
}

__CMPXCHG_CASE(w, b,     ,  8,   )
__CMPXCHG_CASE(w, h,     , 16,   )
__CMPXCHG_CASE(w,  ,     , 32,   )
__CMPXCHG_CASE(x,  ,     , 64,   )
__CMPXCHG_CASE(w, b, acq_,  8,  a, "memory")
__CMPXCHG_CASE(w, h, acq_, 16,  a, "memory")
__CMPXCHG_CASE(w,  , acq_, 32,  a, "memory")
__CMPXCHG_CASE(x,  , acq_, 64,  a, "memory")
__CMPXCHG_CASE(w, b, rel_,  8,  l, "memory")
__CMPXCHG_CASE(w, h, rel_, 16,  l, "memory")
__CMPXCHG_CASE(w,  , rel_, 32,  l, "memory")
__CMPXCHG_CASE(x,  , rel_, 64,  l, "memory")
__CMPXCHG_CASE(w, b,  mb_,  8, al, "memory")
__CMPXCHG_CASE(w, h,  mb_, 16, al, "memory")
__CMPXCHG_CASE(w,  ,  mb_, 32, al, "memory")
__CMPXCHG_CASE(x,  ,  mb_, 64, al, "memory")

#pragma GCC diagnostic pop

#undef __CMPXCHG_CASE

#define __CMPXCHG_DBL(name, mb, cl...)					\
static __always_inline long						\
__lse__cmpxchg_double##name(unsigned long old1,				\
					 unsigned long old2,		\
					 unsigned long new1,		\
					 unsigned long new2,		\
					 volatile void *ptr)		\
{									\
	unsigned long oldval1 = old1;					\
	unsigned long oldval2 = old2;					\
	register unsigned long x0 asm ("x0") = old1;			\
	register unsigned long x1 asm ("x1") = old2;			\
	register unsigned long x2 asm ("x2") = new1;			\
	register unsigned long x3 asm ("x3") = new2;			\
	register unsigned long x4 asm ("x4") = (unsigned long)ptr;	\
									\
	asm volatile(							\
	__LSE_PREAMBLE							\
	"	casp" #mb "\t%[old1], %[old2], %[new1], %[new2], %[v]\n"\
	"	eor	%[old1], %[old1], %[oldval1]\n"			\
	"	eor	%[old2], %[old2], %[oldval2]\n"			\
	"	orr	%[old1], %[old1], %[old2]"			\
	: [old1] "+&r" (x0), [old2] "+&r" (x1),				\
	  [v] "+Q" (*(unsigned long *)ptr)				\
	: [new1] "r" (x2), [new2] "r" (x3), [ptr] "r" (x4),		\
	  [oldval1] "r" (oldval1), [oldval2] "r" (oldval2)		\
	: cl);								\
									\
	return x0;							\
}

__CMPXCHG_DBL(   ,   )
__CMPXCHG_DBL(_mb, al, "memory")

#undef __CMPXCHG_DBL

#endif	/* __ASM_ATOMIC_LSE_H */
