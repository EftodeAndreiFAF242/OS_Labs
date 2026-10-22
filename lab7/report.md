# Lab 7 - Three Small Programs: Processes, Redirection, a Pipe

Student: Eftode Andrei, FAF-242
Machine: Ubuntu 26.04.1 LTS in VirtualBox, kernel 7.0.0-38-generic, gcc 15.2.0
Folder: ~/os-lab7

## Part 1 - fork + exec + wait

    $ gcc -Wall -Wextra -o run run.c && ./run
    parent: my PID is 9044
    child:  my PID is 9046, my parent is 9044
    total 24
    -rwxrwxr-x 1 vboxuser vboxuser 16392 Oct 22 20:32 run
    -rw-rw-r-- 1 vboxuser vboxuser   911 Oct 22 20:31 run.c
    parent: child 9046 ended with status 0

With date the child printed `Thu Oct 22 08:32:40 PM UTC 2026` (status 0).
With nosuchprog:

    exec: No such file or directory
    parent: child 9099 ended with status 127

1. When execlp works it never returns: the child's program is replaced by
   ls, so the lines after it (perror, exit(127)) are gone. They run only
   when exec fails. With nosuchprog, strace shows execve tried in every
   folder of PATH, each one answered ENOENT, and only then perror printed
   "exec: No such file or directory".
2. The child prints as "my parent" the PID of the ./run process itself
   (9044 = "parent: my PID is 9044"). It is not my shell: my shell is 8877.
3. With ls the parent sees status 0 (ls worked). With nosuchprog it sees
   127, the number the child gave to exit(127) after the failed exec
   (bash uses the same 127 for "command not found").

## Part 2 - A zombie, on purpose

    $ ./zombie > /dev/null &
    [1] 9646
    $ sleep 2; ps -o pid,ppid,stat,cmd
        PID    PPID STAT CMD
       8877    8681 Ss   /usr/bin/bash
       9646    8877 S    ./zombie
       9648    9646 Z    [ls] <defunct>
       9651    8877 R+   ps -o pid,ppid,stat,cmd
    $ grep State /proc/$(pgrep -n -x ls)/status
    State:  Z (zombie)

1. ls has Z: zombie (defunct). It has finished and its memory is freed,
   but its entry in the process table (PID and exit status) stays until
   the parent collects it. The parent ./zombie is S, asleep in sleep(20).
2. After 20 seconds the [ls] <defunct> line is gone and bash printed
   "[1]+ Done". The line that removed it is
   `waitpid(pid, &status, 0);`: it collected the exit status.

## Part 3 - Redirection: ls -l > out.txt

    $ gcc -Wall -Wextra -o redirect redirect.c && ./redirect
    open gave descriptor 1
    done: the listing is in out.txt
    $ head -3 out.txt
    total 72
    -rw-r--r-- 1 vboxuser vboxuser     0 Oct 22 20:37 out.txt
    -rwxrwxr-x 1 vboxuser vboxuser 16336 Oct 22 20:37 redirect

1. open gave 1. The kernel always gives the smallest free descriptor:
   0 was still open and 1 had just been closed.
2. ls writes to descriptor 1 as always, but in this child 1 is now the
   file out.txt, not the terminal, and exec keeps the open descriptors.
   ("open gave descriptor 1" is on the screen because it went to 2.)
3. Descriptor 2. I tried it: close(2), open("err.txt"), exec ls nosuchfile.
   err.txt got "open gave descriptor 2" and
   "ls: cannot access 'nosuchfile': No such file or directory".

## Part 4 - A pipe: ls /etc | wc -l

    $ ./pipeline
    237
    $ ls /etc | wc -l
    237

1. Yes, both printed 237.
2. wc never ends. ls finished and was collected by the first wait; ps
   showed only ./pipeline and wc -l, both S, and fd 0 of wc was a pipe.
3. wc reads until end of file, and a pipe gives end of file only when
   every write end is closed; the parent still had p[1] open. When I
   killed only the parent (kill $!), wc printed 237 at once.

## Optional - pipeline3.c: ls /usr/bin | grep x | wc -l

    $ ./pipeline3
    224
    $ ls /usr/bin | grep x | wc -l
    224

Two pipes (p: ls -> grep, q: grep -> wc) and three children. Each child
does its dup and then closes all four pipe ends, and so does the parent,
which then waits three times. strace shows pipe2 giving [3, 4] and [5, 6],
then ls dup(4) = 1, grep dup(3) = 0 and dup(6) = 1, wc dup(5) = 0.

## Conclusion

The shell runs a command with three system calls: fork makes a copy, exec
turns the copy into the program, and wait collects its exit status; a
child that nobody waits for stays a zombie. Redirection and pipes need no
change in the programs: before exec the child closes 0 or 1 and puts a
file or a pipe end in its place, because open and dup always take the
smallest free number. A reader of a pipe stops only when every write end
is closed, so every unused end must be closed.
