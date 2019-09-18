#include <stdio.h>

int main(void)
{
    const char *s = "abcd";

    printf("%%s:   |%s|\n", s);

    printf("%%.2s: |%.2s|\n", s);
    printf("%%.*s: |%.*s|\n", 6, s);

    printf("%%6s:  |%6s|\n", s);
    printf("%%*s:  |%*s|\n", -6, s);

    return 0;
}
