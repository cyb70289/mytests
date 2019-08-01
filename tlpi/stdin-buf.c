/*
 * stdin is line buffered by default. fread returns only if block full or 
 * return entered. fread(buf, 1, 10, stdin) won't return even if 10+ chars
 * are received.
 */

#include <stdio.h>

int main(void)
{
    int buf[10];
    size_t len;

    while (len = fread(buf, 1, 10, stdin))
        fwrite(buf, 1, len, stderr);

    return 0;
}
