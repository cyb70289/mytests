/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */

#ifndef __MISC_H__
#define __MISC_H__

typedef signed char		__s8;
typedef unsigned char		__u8;
typedef signed short		__s16;
typedef unsigned short		__u16;
typedef signed int		__s32;
typedef unsigned int		__u32;
typedef signed long long	__s64;
typedef unsigned long long	__u64;

typedef __s8			s8;
typedef __u8			u8;
typedef __s16			s16;
typedef __u16			u16;
typedef __s32			s32;
typedef __u32			u32;
typedef __s64			s64;
typedef __u64			u64;

/* include/linux/stringify.h */
#define __stringify_1(x...)	#x
#define __stringify(x...)	__stringify_1(x)

/* <linux/types.h> */
typedef struct {
	int counter;
} atomic_t;

/* include/linux/compiler.h */
/* include/linux/compiler_types.h */
/* include/linux/compiler_attributes.h */
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#ifndef __always_inline
#define __always_inline	inline __attribute__((__always_inline__))
#endif
#define __pure		__attribute__((__pure__))
#define __aligned(x)	__attribute__((__aligned__(x)))
#define barrier()	__asm__ __volatile__("": : :"memory")
#define unreachable()	__builtin_unreachable()

/* include/asm-generic/rwonce.h */
#define READ_ONCE(x)	(*(const volatile typeof(x) *)&(x))
#define WRITE_ONCE(x, val)						\
do {									\
	*(volatile typeof(x) *)&(x) = (val);				\
} while (0)

/* arch/arm64/include/asm/processor.h */
static inline void prefetchw(const void *ptr)
{
	asm volatile("prfm pstl1keep, %a0\n" : : "p" (ptr));
}

/* arch/arm64/include/asm/vdso/processor.h */
static inline void cpu_relax(void)
{
	asm volatile("yield" ::: "memory");
}

#endif  /* __MISC_H__ */
