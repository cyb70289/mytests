/* Implement "tail" command: tail [-n NUM] filename */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

static int get_lines_fn(int argc, char *argv[], const char **fn)
{
    int lines = 10;

    if (argc == 2) {
        *fn = argv[1];
    } else if (argc == 4 && strcmp(argv[1], "-n") == 0) {
        lines = atoi(argv[2]);
        *fn = argv[3];
    } else {
        printf("invalid arguments\n");
        exit(1);
    }

    if (lines < 1)
        lines = 10;

    return lines;
}

static void parse_newline(const char *buf, int len, char *last_char, int offset,
        int *newline_seeks, int lines, int *pnewline_first, int *pnewline_last)
{
    int newline_first = *pnewline_first, newline_last = *pnewline_last;

    for (int i = 0; i < len; ++i) {
        if (*last_char == '\n') {
            newline_last = (newline_last + 1) % lines;
            if (newline_last == newline_first) {
                newline_first = (newline_first + 1) % lines;
            }
            newline_seeks[newline_last] = offset + i;
        }
        *last_char = buf[i];
    }

    *pnewline_first = newline_first;
    *pnewline_last = newline_last;
}

int main(int argc, char *argv[])
{
    int fd, lines;
    int *newline_seeks, newline_first, newline_last, offset;
    char buf[256], last_char;
    const char *fn;
    ssize_t len;

    lines = get_lines_fn(argc, argv, &fn);

    fd = open(fn, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    newline_seeks = malloc(sizeof(int) * lines);
    newline_seeks[0] = 0;

    newline_first = newline_last = 0;
    offset = 0;
    last_char = '\0';

    /* read all new inputs */
    while (1) {
        len = read(fd, buf, 256);
        if (len == -1 && errno != EINTR) {
            perror("read");
            return 1;
        }
        if (len == 0) {
            break;
        }
        parse_newline(buf, len, &last_char, offset,
                newline_seeks, lines, &newline_first, &newline_last);
        offset += len;
    }

    /* seek to print start position */
    if (lseek(fd, newline_seeks[newline_first], SEEK_SET) == (off_t)-1) {
        perror("lseek");
        return 1;
    }

    /* output */
    while ((len = read(fd, buf, 256)) > 0) {
        write(STDOUT_FILENO, buf, len);
    }

    return 0;
}
