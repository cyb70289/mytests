int main() {
  atomic_int x = 0;
  atomic_int y = 0;

  {{{ {
        x.store(1, mo_relaxed);
        atomic_thread_fence(mo_seq_cst);
        y.load(mo_relaxed).readsvalue(0);
      }
  ||| {
        y.store(1, mo_relaxed);
        atomic_thread_fence(mo_seq_cst);
        x.load(mo_relaxed).readsvalue(0);
      }
  }}};
  return 0;
}
