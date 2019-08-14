#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wsign-conversion"

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

struct pagemap {
	uint64_t pfn;
	unsigned int soft_dirty;
	unsigned int exclusive;
	unsigned int file_page;
	unsigned int swapped;
	unsigned int present;
};

static int pagemap_get_entry(struct pagemap *page,
        int pagemap_fd, uintptr_t vaddr)
{
	size_t nread;
	ssize_t ret;
	uint64_t data;
	uintptr_t vpn;

	vpn = vaddr / sysconf(_SC_PAGE_SIZE);
	nread = 0;
	while (nread < sizeof(data)) {
		ret = pread(
			pagemap_fd,
			&data,
			sizeof(data) - nread,
			vpn * sizeof(data) + nread
		);
		nread += ret;
		if (ret <= 0) {
			return 1;
		}
	}
	page->pfn = data & (((uint64_t)1 << 55) - 1);
	page->soft_dirty = (data >> 55) & 1;
	page->exclusive = (data >> 56) & 1;
	page->file_page = (data >> 61) & 1;
	page->swapped = (data >> 62) & 1;
	page->present = (data >> 63) & 1;
	return 0;
}

static int virt_to_phys(const void *vaddr, struct pagemap *page)
{
	int pagemap_fd;

	pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
	if (pagemap_fd < 0) {
		return -1;
	}
	if (pagemap_get_entry(page, pagemap_fd, (uintptr_t)vaddr)) {
		return -1;
	}
	close(pagemap_fd);

    return 0;
}

static void print_phys_page(const void *vaddr)
{
    struct pagemap page;

    if (virt_to_phys(vaddr, &page) == -1) {
        printf("cannot find pa for %p\n", vaddr);
        return;
    }

    printf("pid: %u, ", getpid());
    printf("va: %p, ", vaddr);
    printf("pa: %p", (void *)(page.pfn * sysconf(_SC_PAGE_SIZE)));
    if (page.present)
        printf(", present");
    if (page.swapped)
        printf(", swapped");
    if (page.file_page)
        printf(", file");
    if (page.exclusive)
        printf(", exclusive");
    if (page.soft_dirty)
        printf(", dirty");
    printf("\n");
}

static void segv(int n)
{
    printf("SEGV, %u\n", getpid());
    _exit(1);
}

int main(int argc, char *argv[])
{
    static const size_t _sz = 17;

	char *buf1, *buf2;
    pid_t pid;
    int ws;

    signal(SIGSEGV, segv);

    buf1 = malloc(_sz);
    buf1[0] = 'a';
    print_phys_page(buf1);
    if ((pid = fork())) {
        /* parent */
        printf("%u->%u\n", getpid(), pid);
        wait(&ws);

        buf1[0] = 'a';
        print_phys_page(buf1);
        printf("%u exit\n", getpid());
        return 0;
    }

    buf2 = malloc(_sz);
    buf1[0] = 'a';
    buf2[0] = 'b';
    print_phys_page(buf1);
    if ((pid = fork())) {
        /* child */
        printf("%u->%u\n", getpid(), pid);
        wait(&ws);

        buf1[0] = 'a';
        buf2[0] = 'b';
        print_phys_page(buf1);
        printf("%u exit\n", getpid());
        return 0;
    }

    /* child of child */
    buf1[0] = 'a';
    buf2[0] = 'b';
    print_phys_page(buf1);
    free(buf1);

    printf("%u exit\n", getpid());
    return 0;
}
