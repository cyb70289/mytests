// 10 functions, in each function
// - 

#pragma GCC push_options
#pragma GCC optimize ("align-functions=2048")

void dummy1();
void dummy2();
void dummy3();
void dummy4();
void dummy5();
void dummy6();
void dummy7();
void dummy8();

volatile unsigned _v = 7;

// fabc
#define F1(a,b,c)             \
void f ## a ## b ## c(void) { \
    const unsigned v = _v;    \
                              \
    if (v <= 1) dummy1();     \
    if (v <= 2) dummy2();     \
    if (v <= 3) dummy3();     \
    if (v <= 4) dummy4();     \
    if (v <= 5) dummy5();     \
    if (v <= 6) dummy6();     \
    if (v <= 7) dummy7();     \
    if (v <= 8) dummy8();     \
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
