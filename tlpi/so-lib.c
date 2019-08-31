/*
 * shared lib call functions in main
 * $ gcc -shared -fPIC -o liba.so liba.c
 *
 * main program must link with "-export-dynamic" to export main_func to so
 */

#include <stdio.h>

extern void main_func(void);

void so_func(void)
{
    printf("shared lib\n");
    main_func();
}
