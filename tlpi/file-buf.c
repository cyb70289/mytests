/*
 * stdout: line buffer. normal file: full buffer
 * Output lines are reversed if directed stdout to a file
 */

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("If I had more time, \n");
    write(STDOUT_FILENO, "I would have written you a shorter letter.\n", 43);

    return 0;
}
