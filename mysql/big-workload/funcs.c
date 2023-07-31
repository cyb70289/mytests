#pragma GCC push_options
#pragma GCC optimize ("align-functions=1024")

#define NOP   __asm__ __volatile("nop" : : : "memory")
#define NOP8  NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP
#define NOP64 NOP8; NOP8; NOP8; NOP8; NOP8; NOP8; NOP8; NOP8

// do a dummy store/load in the function body to make sure the code
// is different for each "n", otherwise compiler will combine the nop block
volatile int _d[8];
#define dummy(n)   \
do {               \
    int t = _d[n]; \
    NOP64;         \
    _d[n] = t;     \
} while (0)

// define function: void f000(), ..., void f799()
// assign a dummy variable in each function to make sure the functions
// are different, otherwise compiler will merg them to one function
volatile int  _v = 4;
#define F1(a,b,c)               \
volatile int _u ## a ## b ## c; \
void f ## a ## b ## c() {       \
    _u ## a ## b ## c = 1;      \
                                \
    const int v = _v;           \
    if (v <= 0) dummy(0);       \
    else if (v <= 1) dummy(1);  \
    else if (v <= 2) dummy(2);  \
    else if (v <= 3) dummy(3);  \
    else if (v <= 4) dummy(4);  \
    else if (v <= 5) dummy(5);  \
    else if (v <= 6) dummy(6);  \
    else if (v <= 7) dummy(7);  \
}

// define 10 funcs: fab0, ..., fab9
#define F2(a,b) \
    F1(a,b,0)   \
    F1(a,b,1)   \
    F1(a,b,2)   \
    F1(a,b,3)   \
    F1(a,b,4)   \
    F1(a,b,5)   \
    F1(a,b,6)   \
    F1(a,b,7)   \
    F1(a,b,8)   \
    F1(a,b,9)

// define 100 funs: fa00, ..., fa99
#define F3(a) \
    F2(a,0)   \
    F2(a,1)   \
    F2(a,2)   \
    F2(a,3)   \
    F2(a,4)   \
    F2(a,5)   \
    F2(a,6)   \
    F2(a,7)   \
    F2(a,8)   \
    F2(a,9)

// define 800 functions: f000, ..., f799
F3(0)
F3(1)
F3(2)
F3(3)
F3(4)
F3(5)
F3(6)
F3(7)

#pragma GCC pop_options
