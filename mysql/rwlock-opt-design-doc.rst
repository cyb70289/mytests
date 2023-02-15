MySQL rwlock optimization
=========================

MySQL designes a user mode read/write lock for efficient storage operations.
Few document is available about its implementation.

This section tries to explain how it works internally and justify possible
optimization points.

functionality
-------------
Reference `official document <https://dev.mysql.com/doc/refman/8.0/en/glossary.html#glos_rw_lock>`_.

rwlock can have three states:

- S - shared (read access)
- SX - shared exclusive (write access, permits read)
- X - exclusive (write access, prohibits read)

Multiple threads can have S locks concurrently, it blocks X lock. Only one
thread can get SX lock, it doesn't block S locks, inconsistent reading is
allowed. Only one thread can get X lock, it blocks S and SX locks. Same thread
can acquire same lock recursively. Moreover, X lock holder can get SX lock,
vice versa.

State compatibility table(1 - compatible, 0 - conflict):

+----+----+----+---+
|    | S  | SX | X |
+====+====+====+===+
| S  | 1  | 1  | 0 |
+----+----+----+---+
| SX | 1  | 0  | 0 |
+----+----+----+---+
| X  | 0  | 0  | 0 |
+----+----+----+---+

Lock state transform
~~~~~~~~~~~~~~~~~~~~

+---------------+--------------+-----------------------------------------------------------+
| Current state | Lock request | Action                                                    |
+===============+==============+===========================================================+
| S             | S            | succeed                                                   |
+---------------+--------------+-----------------------------------------------------------+
| S             | SX           | succeed                                                   |
+---------------+--------------+-----------------------------------------------------------+
| S             | X            | spin/sleep for all S done, block further S/SX requests    |
+---------------+--------------+-----------------------------------------------------------+
| SX            | S            | succeed                                                   |
+---------------+--------------+-----------------------------------------------------------+
| SX            | SX           | succeed if same thread, otherwise spin/wait for SX done   |
+---------------+--------------+-----------------------------------------------------------+
| SX            | X            | spin/sleep for all S/SX done                              |
+---------------+--------------+-----------------------------------------------------------+
| X             | S            | spin/sleep for X done                                     |
+---------------+--------------+-----------------------------------------------------------+
| X             | SX           | succeed if same thread, otherwise spin/wait for X done    |
+---------------+--------------+-----------------------------------------------------------+
| X             | X            | succeed if same thread, otherwise spin/wait for X done    |
+---------------+--------------+-----------------------------------------------------------+

spin/sleep and unlock
~~~~~~~~~~~~~~~~~~~~~

If lock cannot be granted immediately, the request thread will spin to retry
locking in a tight loop for a predefined time, hoping to get the lock quickly.
On timeout, the request thread is put to sleep to wait for a conditional
variable which will be signalled by lock holder when it releases the lock.

Data structure
--------------
rwlock struct is defined at `sync0rw.h
<https://github.com/mysql/mysql-server/blob/mysql-cluster-8.0.17/storage/innobase/include/sync0rw.h#L555>`_.

``lock_word``, ``waiters``, ``recursive`` are three most important members for
rwlock synchronization.

.. code-block:: c

  struct rw_lock_t {
      volatile lint lock_word;
      volatile ulint waiters;
      volatile bool recursive;
      // ...
  };

New atomic data type
~~~~~~~~~~~~~~~~~~~~

Current atomic API os_atomic_xxx are based on GCC __sync_xxx or __atomic_xxx on
Linux/Solaris and MSVC InterlockedXXX on Windows. They lacks memory order
parameters. Extending them to support all platforms and all use cases may lead
to messy code like exploding #ifdef.

Per MySQL8.0 source installation prerequisites [1]_, at least C++11 compiler is
required. So std::atomic can be used to consistently handle atomic operations
for all platforms and compilers.

Add a new atomic data type ``os_atomic_t`` which wraps std::atomic and blocks
overridden operators. std::atomic overrides some operators for convenience. The
problem is they imply strongest memory order. os_atomic_t disables them and
forces using member functions with explicit memory order options. It also helps
to leverage compiler to automatically check all related codes when porting to
new atomic data type.

.. code-block:: c

  // std::atomic overrides operatos
  std::atomic<int> i;

  // Implicit ordering: memory_order_seq_cst
  i = 1;
  i == 1;

  // Explicit ordering options
  i.store(std::memory_order_relaxed);
  i.load(std::memory_order_relaxed) == 1;

.. code-block:: c

  // Disable operators
  template <typename T>
  struct os_atomic_t : std::atomic<T> {
    operator T() const	= delete;
    T operator=(T arg)	= delete;
    // ......
  };

  os_atomic_t<int> i;

  // Compile error
  i = 1;
  i == 1;

rwlock->recursive
-----------------

recursive_ is a bool value, it publishes writer_thread_ which identifies owner
of X locks. A `memory order bug <https://bugs.mysql.com/bug.php?id=94699>`_ was
found and fixed.

.. _recursive: https://github.com/mysql/mysql-server/blob/mysql-cluster-8.0.17/storage/innobase/include/sync0rw.h#L575

.. _writer_thread: https://github.com/mysql/mysql-server/blob/mysql-cluster-8.0.17/storage/innobase/include/sync0rw.h#L588

.. code-block:: c

  // producer (full barrier from __sync_xxx)
  local_thread = lock->writer_thread;
  success = os_compare_and_swap_thread_id(&lock->writer_thread, local_thread,
                                          curr_thread);
  lock->recursive = recursive;

  // consumer (full barrier)
  recursive = lock->recursive;
  os_rmb;
  writer_thread = lock->writer_thread;

Optimization
~~~~~~~~~~~~
Current fix uses full memory barrier, it's more suitable to leverage C11
store-release and load-acquire model.

rwlock->lock_word
-----------------
lock_word_ is core of rwlock implementation. It is defined as a signed long
integer, its value reflects current lock status(S, SX, X).

.. _lock_word: https://github.com/mysql/mysql-server/blob/mysql-cluster-8.0.17/storage/innobase/include/sync0rw.h#L561

- Initialize

  - lock_word is initialized to a very large number ``X_LOCK_DECR = 0x20000000``

- Lock

  - lock_word is decremented by ``one`` for each successful S lock request
  - lock_word is decremented by ``X_LOCK_DECR / 2`` for an initial SX lock
    request (unlocked or S state to SX)
  - lock_word is decremented by ``X_LOCK_DECR`` for an initial X lock request
    (unlocked state to X), or ``one`` for recursive X lock
  - Spin/sleep if failed to get lock

- Unlock

  - Reverse of Lock operation
  - Wake up waiting threads

lock_word and lock status
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

  lock_word               Lock Status

  0x20000000 --> +-----------------------------+
                 |                             |
                 |          S locked           |
                 |                             |
                 |                             |
  0x10000000 --> +----------SX locked----------+
                 |                             |
                 |        S & SX locked        |
                 |                             |
                 |                             |
           0 --> +-----------X locked----------+
                 |                             |
                 |  S locked, X lock waiting   |
                 |                             |
                 |                             |
 -0x10000000 --> +--------SX & X locked--------+
                 |                             |
                 | S locked, X lock waiting    |
                 | X lock requester own SX lock|
                 |                             |
 -0x20000000 --> +----------2 X locked---------+
                 |                             |
                 |    X locked recursively     |
                 |                             |
                 |                             |
 -0x30000000 --> +--------SX & 2X locked-------+
                 |                             |
                 |  SX & X locked recursively  |
                 .             .               .
                 .             .               .

Acquire lock
~~~~~~~~~~~~

The core function of acquiring a lock is rw_lock_lock_word_decr_, which
compares lock_word with a threshold and decrement it if comparison succeeds,
all done in atomic.

.. _rw_lock_lock_word_decr: https://github.com/mysql/mysql-server/blob/mysql-cluster-8.0.17/storage/innobase/include/sync0rw.ic#L226

.. code-block:: c

  // Full barrier from __sync_val_compare _and_swap()
  local_lock_word = lock->lock_word;
  while (local_lock_word > threshold) {
    if (os_compare_and_swap_lint(&lock->lock_word, local_lock_word,
                                 local_lock_word - amount)) {
      return (true);
    }
    local_lock_word = lock->lock_word;
  }
  return (false);

  // Code explanation
  //
  // if (lock->lock_word > threshold) {
  //    lock->lock_word -= amount;
  //    return true;
  // }
  // return false;

Optimization
""""""""""""
Current code user GCC ``__sync_val_compare_and_swap`` which enforces full
memory barrier, C11 ``load acquire`` memory model is more suitable for this use
case.

Release lock
~~~~~~~~~~~~

The core function of releasing a lock is rw_lock_lock_word_incr_, which
increments lock_word atomicly. According to return value, lock holder may wake
up threads waiting for lock.

.. _rw_lock_lock_word_incr: https://github.com/mysql/mysql-server/blob/mysql-cluster-8.0.17/storage/innobase/include/sync0rw.ic#L258

.. code-block:: c

  // Full barrier from __sync_add_and_fetch()
  return (os_atomic_increment_lint(&lock->lock_word, amount));

  // Code explanation
  //
  // lock->lock_word += amount;
  // return lock->lock_word;

Optimization
""""""""""""
Current code user GCC ``__sync_add_and_fetch`` which enforces full memory
barrier, C11 ``store release`` memory model is more suitable for this use case.

rwlock->waiters
---------------

waiters_ is an unsigned integer(used as bool actually) to tell the lock holder
that some threads are sleeping and waiting for lock released. The lock holder
must check it and signal a conditional variable to wake them up. Memory order
is important to make sure sleeping threads will be woken up in time.

.. code-block:: c

  // Simplified code snippet

  //////////////////////////////////////////////////////////////////////
  // Producer code, during lock acquiring

  // set lock->waiters = 1, enforce a wmb()
  rw_lock_set_waiter_flag(lock);

  // If failed, lock is held by other thread. That thread must see lock->waiters = 1
  // to wake me up. Enforces an mb().
  if (rw_lock_s_lock_low(lock, pass, file_name, line)) {
    return; /* Success */
  }

  // put current thread into sleep and wait for event
  sync_array_wait_event(sync_arr, cell);


  /////////////////////////////////////////////////////////////////////
  // Consumer code, during lock releasing

  // Enforces an mb()
  if (rw_lock_lock_word_incr(lock, X_LOCK_DECR) <= 0) {
      ut_error;
  }

  if (lock->waiters) {
    // reset lock->waiters = 0
    rw_lock_reset_waiter_flag(lock);
    // wake up waiting threads
    os_event_set(lock->event);
    sync_array_object_signalled();
  }

.. _waiters: https://github.com/mysql/mysql-server/blob/mysql-cluster-8.0.17/storage/innobase/include/sync0rw.h#L564

analysis
~~~~~~~~

Handling ``waiters`` correctly is tricky. Current code enforces several memroy
barriers. How to deal it with C11 memory model is not obvious.

It boils down to below problem:

.. code-block:: c

  // initial state: lock_word = 0; waiter = 0;

  /* thread1 */                         /* thread2 */
  waiter = 1;                           lock_word = 1;
  mb(); /* full mb required? */         mb(); /* full mb required? */
  if (lock_word == 0) {                 if (waiter == 1) {
      wait(); /* sleep */                 signal(); /* wakeup thread1 */
  }                                     }

  // We want to make sure that if thread1 calls wait(), thread2 must call signal() to wake it up.
  // It's allowed that thread2 calls signal() before thread1 calls wait().
  // MySQL implements an event lib to ensure signal isn't lost.

My understanding is that *full memory barrier is required for both threads*.

Other issues
------------

Below are some possible issues of MySQL rwlock implementation. No concrete
profiling and benchmark is done yet. It's just listed here for further
studying.

X lock request blocks S/S lock
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is an inherent problem of read/write lock, which outweighs writer than
reader. To prevent reader starves writer, all new readers must be blocked when
writer request is pending.

In below chart, time elapses from left to right.

1. S1 holds S lock and runs.
2. X1 requests X lock, waits for S1 finishes, and blocks further S requests.
3. S2 requests S lock, blocked by X1 request. From the chart, S2 would finish
   before S1, it can run without interfering X1.
4. S1 finishes. X1 gets X lock and runs.
5. X1 finishes. S2, S3 get S lock and run. Their running is unnecessary
   postponed.

This is a reader/reader block trigger by writer. In typical RWLock use case
with many readers and few writers, this issue may be much more serious than
expected. See [2]_ for reference.

.. code-block:: text

  S1 |-----------running-------------|

  X1      |..... wait, block S ......|--running--|
          ^                          ^           ^
      Request X                   Grant X     Release X

  S2             |bbbbbbbbbbbbb|                 |---running---|

  S3               |bbbbbbbbbbbbbbb|             |----running----|
                   ^                             ^
       False blocked by X request          Unnecessary delay

SX lock delays X lock
~~~~~~~~~~~~~~~~~~~~~

:NOTE: This issue is inferred purely from source code, without testing. May be
       invalid.

MySQL introduces SX lock to improve performance. Under some conditions SX lock
may delay X lock request.

In below chart, time elapses from left to right.

1. SX1 holds SX lock and runs.
2. X1 request X lock, waits for SX1 finishes, does *not* block new S requests.
3. S1 gets S lock(before SX1 finishes) and runs.
4. SX1 finishes and wakes up X1.
5. X1 request X lock again, blocks new S request, but has to wait for S1.
6. S1 finishes, X1 gets X lock and runs.

X1 is supposed to run after SX1 finishes, but it's blocked by reader S1.

.. code-block:: text

  SX1 |-----------running-------------|

   X1       |..........wait...........|....wait S, block new S....|-----running---|
            ^                         ^                           ^
        Request X                   Wakeup                  X1 delayed by S1

   S1                              |-----------running------------|


.. [1] https://dev.mysql.com/doc/refman/8.0/en/source-installation-prerequisites.html
.. [2] https://blog.nelhage.com/post/rwlock-contention/
