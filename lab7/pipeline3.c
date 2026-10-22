#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/* does the same as the shell command:   ls /usr/bin | grep x | wc -l   */

int p[2], q[2];                                  /* p: ls -> grep,  q: grep -> wc */

void close_all(void)                             /* close all four pipe ends */
{
    close(p[0]); close(p[1]);
    close(q[0]); close(q[1]);
}

int main(void)
{
    if (pipe(p) < 0 || pipe(q) < 0) { perror("pipe"); return 1; }

    if (fork() == 0) {                           /* ls: its 1 becomes p[1] */
        close(1);
        dup(p[1]);                               /* smallest free number: 1 */
        close_all();
        execlp("ls", "ls", "/usr/bin", (char *)NULL);
        perror("exec ls");
        exit(127);
    }

    if (fork() == 0) {                           /* grep: its 0 is p[0], its 1 is q[1] */
        close(0);
        dup(p[0]);                               /* smallest free number: 0 */
        close(1);
        dup(q[1]);                               /* smallest free number: 1 */
        close_all();
        execlp("grep", "grep", "x", (char *)NULL);
        perror("exec grep");
        exit(127);
    }

    if (fork() == 0) {                           /* wc: its 0 becomes q[0] */
        close(0);
        dup(q[0]);                               /* smallest free number: 0 */
        close_all();
        execlp("wc", "wc", "-l", (char *)NULL);
        perror("exec wc");
        exit(127);
    }

    close_all();                                 /* the parent uses no end */
    wait(NULL);
    wait(NULL);
    wait(NULL);
    return 0;
}
