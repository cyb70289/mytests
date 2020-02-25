// https://stackoverflow.com/questions/6636793/what-are-the-general-rules-for-comparing-different-data-types-in-c

#include <stdio.h>

void test1(void)
{
    printf("(char)127 - (char)-128 = (int)255\n");
    char c1 = 127, c2 = -128;
    int i1 = c1 - c2;
    printf("%d\n", i1);
}

void test2(void)
{
    printf("(uchar)127 - (uchar)128 = (int)-1\n");
    unsigned char c1 = 127, c2 = -128;
    int i1 = c1 - c2;
    printf("%d\n", i1);
}

void test3(void)
{
    printf("(int)(2^31-1) - (int)(-2^31) = (int)-1\n");
    int c1 = 1 << 31;
    int c2 = ~c1;
    int i1 = c2 - c1;
    printf("%d\n", i1);
}

void test4(void)
{
    printf("(char)127 - (char)-128 = (uint)255\n");
    char c1 = 127, c2 = 128;
    unsigned int i1 = c1 - c2;
    printf("%u\n", i1);
}

void test5(void)
{
    printf("(char)127 - (char)-128 + (char)1 = int(256)\n");
    char c1 = 127, c2 = 128, c3 = 1;
    int i1 = c1 - c2 + c3;
    printf("%u\n", i1);
}

int main(void)
{
    test1();
    test2();
    test3();
    test4();
    test5();

    return 0;
}
