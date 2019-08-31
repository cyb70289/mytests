/*
 * call shared lib which callbacks main func
 * $ gcc -export-dynamic -o so-main so-main.c -ldl
 *
 * w/o "-export-dynamic": ./so-lib.so: undefined symbol: main_func
 */

#include <stdio.h>
#include <dlfcn.h>

void main_func(void)
{
    printf("main func\n");
}

int main(void)
{
    void *handle;
    void (*so_func)(void);

    handle = dlopen("./so-lib.so", RTLD_LAZY);
    if (handle == NULL) {
        printf("dlopen: %s\n", dlerror());
        return 1;
    }

    *(void **)&so_func = dlsym(handle, "so_func");
    if (so_func == NULL) {
        printf("dlsym: %s\n", dlerror());
        return 1;
    }

    so_func();

    return 0;
}
