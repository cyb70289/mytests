int main() {
  atomic_int flag = 0;
  atomic_int data = 0;

  {{{ {
        data.store(1, mo_relaxed);
        flag.store(1, mo_release);
      }
  ||| {
        flag.load(mo_acquire).readsvalue(1);
        data.load(mo_relaxed).readsvalue(0);
      }
  }}};
  return 0;
}
