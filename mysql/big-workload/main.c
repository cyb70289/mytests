#define F1(a,b,c) void f ## a ## b ## c(void);

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

F3(0)
F3(1)
F3(2)
F3(3)
F3(4)
F3(5)
F3(6)
F3(7)

#define CALL_F1(a,b,c) f ## a ## b ## c();

// fab0(); ...; fab9();
#define CALL_F2(a,b) \
    CALL_F1(a,b,4);  \
    CALL_F1(a,b,1);  \
    CALL_F1(a,b,3);  \
    CALL_F1(a,b,9);  \
    CALL_F1(a,b,5);  \
    CALL_F1(a,b,7);  \
    CALL_F1(a,b,2);  \
    CALL_F1(a,b,6);  \
    CALL_F1(a,b,8);  \
    CALL_F1(a,b,0);

// fa00(); ...; fa99();
#define CALL_F3(a) \
    CALL_F2(a,3);  \
    CALL_F2(a,7);  \
    CALL_F2(a,0);  \
    CALL_F2(a,1);  \
    CALL_F2(a,6);  \
    CALL_F2(a,4);  \
    CALL_F2(a,9);  \
    CALL_F2(a,2);  \
    CALL_F2(a,8);  \
    CALL_F2(a,5);

int main() {
    for (int i = 0; i < 100; ++i) {
        CALL_F3(5);
        CALL_F3(0);
        CALL_F3(3);
        CALL_F3(2);
        CALL_F3(4);
        CALL_F3(7);
        CALL_F3(1);
        CALL_F3(6);
    }
    return 0;
}
