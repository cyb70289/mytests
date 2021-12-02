int main() {
  atomic_int x = 0;
  atomic_int y = 0;

  {{{ { x.store(1); }
  ||| { y.store(1); }
  ||| { x.load().readsvalue(1); y.load().readsvalue(0); }
  ||| { y.load().readsvalue(1); x.load().readsvalue(0); }
  }}};
  return 0;
}
