#include <linux/module.h>
#include <linux/kallsyms.h>

/* https://onlinedisassembler.com/odaweb/ */
/* objdump -D -b binary -maarch64 binfile */

static int __init dumpk_init(void)
{
    int i;
    const int len = 64;
    char buf[len*3+1];
    const char *b = (const char *)kallsyms_lookup_name("flush_thread");
    if (b == NULL) {
        printk("EEEEEEEEEEEEEEE\n");
        return -EINVAL;
    }

    for (i = 0; i < len; ++i) {
        sprintf(buf+i*3, "%02x", b[i]);
        buf[i*3+2] = ' ';
    }
    buf[len*3-1] = '\n';
    buf[len*3] = '\0';

    printk(buf);

    return -EINVAL;
}
 
module_init(dumpk_init);

MODULE_LICENSE("GPL");
