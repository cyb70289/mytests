https://godbolt.org/z/dKcP65n5f

struct A {
    int x = 10;
    int y = 20;
};

// pointer to class member
int A::* ptrx = &A::x;  // ptrx = 0
int A::* ptry = &A::y;  // ptry = 4

A a;

// get member per pointer
int x = a.*ptrx;  // x = 10
int y = a.*ptry;  // y = 20
