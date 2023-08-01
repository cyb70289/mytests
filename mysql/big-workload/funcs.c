#pragma GCC push_options
#pragma GCC optimize ("align-functions=1024")

volatile unsigned long _v = 0x1122334455667788;
volatile unsigned long _u = 0;

// define function: void f000(), ..., void f799()
// assign a dummy variable in each function to make sure the functions
// are different, otherwise compiler will merg them to one function
//
// IPC of big workload vs. small workload
// - caslake:     0.84, 2.21
// - milan:       1.57, 2.98
// - neoverse-n1: 0.41, 2.73
// - neoverse-n2: 1.13, 3.44
// - neoverse-v1: 1.07, 4.06
#define F1(a,b,c)                                    \
volatile int _u ## a ## b ## c;                      \
void f ## a ## b ## c() {                            \
    _u ## a ## b ## c = 1;                           \
                                                     \
    const unsigned long v = _v;                      \
    const unsigned u0 = 1ULL << ((v >> 56) & 63ULL); \
    const unsigned u1 = 1ULL << ((v >> 48) & 63ULL); \
    const unsigned u2 = 1ULL << ((v >> 40) & 63ULL); \
    const unsigned u3 = 1ULL << ((v >> 32) & 63ULL); \
    const unsigned u4 = 1ULL << ((v >> 24) & 63ULL); \
    const unsigned u5 = 1ULL << ((v >> 16) & 63ULL); \
    const unsigned u6 = 1ULL << ((v >>  8) & 63ULL); \
    const unsigned u7 = 1ULL << ((v >>  0) & 63ULL); \
    _u = u0 | u1 | u2 | u3 | u4 | u5 | u6 |u7;       \
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
