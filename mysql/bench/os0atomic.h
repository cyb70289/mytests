/* Copied from MySQL source with C++11 optimization patch */

#ifndef os0atomic_h
#define os0atomic_h

#include <cstdlib>

#define UNIV_INLINEi    static inline
#define ulint           unsigned long
#define ut_a(c)         do { if (!(c)) std::abort(); } while (0)
#define ut_error        std::abort()
#define UT_RELAX_CPU()  __asm__ __volatile__("" ::: "memory")

#define os_compare_and_swap(ptr, old_val, new_val) \
  __sync_bool_compare_and_swap(ptr, old_val, new_val)

#define os_compare_and_swap_ulint(ptr, old_val, new_val) \
  os_compare_and_swap(ptr, old_val, new_val)

#define os_compare_and_swap_lint(ptr, old_val, new_val) \
  os_compare_and_swap(ptr, old_val, new_val)

#define os_compare_and_swap_uint32(ptr, old_val, new_val) \
  os_compare_and_swap(ptr, old_val, new_val)

#define os_compare_and_swap_uint64(ptr, old_val, new_val) \
  os_compare_and_swap(ptr, old_val, new_val)

#define os_atomic_increment(ptr, amount) __sync_add_and_fetch(ptr, amount)

#define os_atomic_increment_lint(ptr, amount) os_atomic_increment(ptr, amount)

#define os_atomic_increment_ulint(ptr, amount) os_atomic_increment(ptr, amount)

#define os_atomic_increment_uint32(ptr, amount) os_atomic_increment(ptr, amount)

#define os_atomic_increment_uint64(ptr, amount) os_atomic_increment(ptr, amount)

#define os_atomic_decrement(ptr, amount) __sync_sub_and_fetch(ptr, amount)

#define os_atomic_decrement_lint(ptr, amount) os_atomic_decrement(ptr, amount)

#define os_atomic_decrement_ulint(ptr, amount) os_atomic_decrement(ptr, amount)

#define os_atomic_decrement_uint32(ptr, amount) os_atomic_decrement(ptr, amount)

#define os_atomic_decrement_uint64(ptr, amount) os_atomic_decrement(ptr, amount)

#define os_atomic_inc_ulint(m, v, d) os_atomic_increment_ulint(v, d)
#define os_atomic_dec_ulint(m, v, d) os_atomic_decrement_ulint(v, d)
#define TAS(l, n) os_atomic_test_and_set((l), (n))
#define CAS(l, o, n) os_atomic_val_compare_and_swap((l), (o), (n))

#define os_rmb __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define os_wmb __atomic_thread_fence(__ATOMIC_RELEASE)

#include <atomic>

/** Shorter names for C++11 memory order options */
constexpr auto order_relaxed = std::memory_order_relaxed;
constexpr auto order_consume = std::memory_order_consume;
constexpr auto order_acquire = std::memory_order_acquire;
constexpr auto order_release = std::memory_order_release;
constexpr auto order_acq_rel = std::memory_order_acq_rel;
constexpr auto order_seq_cst = std::memory_order_seq_cst;

template <typename T>
struct os_atomic_t : std::atomic<T> {
  /** Disable implicit load/store operators as they enforces strongest memory
  order. Use member functions with explicit memory order options instead. */
  operator T() const	= delete;
  T operator=(T arg)	= delete;
  T operator+=(T arg)	= delete;
  T operator-=(T arg)	= delete;
  T operator&=(T arg)	= delete;
  T operator|=(T arg)   = delete;
  T operator^=(T arg)   = delete;
  T operator++()        = delete;
  T operator--()        = delete;
};

#endif /* !os0atomic_h */
