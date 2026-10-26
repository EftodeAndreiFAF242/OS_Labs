# Lab 2 - Meet the OS You Will Build (xv6)

Student: Eftode Andrei, FAF-242
Tree: a fresh clone of mit-pdos/xv6-riscv in ~/xv6-lab2, revision f780d78
(the current one). Tools: riscv64-linux-gnu-gcc 15.2.0, QEMU 10.2.1.
I first did this lab in September on the revision before sleep() was renamed
to pause(); this is the same work redone on the current xv6.

## Part 2 - Using xv6

1. Three programs xv6 ships with: ls, cat, echo
   (the others on the disk: grep, wc, sh, mkdir, rm, ln, kill, init, usertests ...).
2. For "ls | grep c" to work the OS must have:
   - processes: the shell creates two processes (fork) and runs a different
     program in each (exec), so that ls and grep run at the same time;
   - communication between processes: the kernel's pipe, a buffer with a write
     end and a read end that are file descriptors. The write end replaces the
     standard output of ls, the read end the standard input of grep.
3. The xv6 shell works on the same idea as the Linux shell (commands, arguments,
   pipes, redirection) but it is much smaller: the prompt is only "$" and there
   is no history, no tab completion, no variables and no job control.

Output:

    $ echo hello xv6
    hello xv6
    $ ls | grep c
    cat            2 3 36760
    echo           2 4 35624
    wc             2 17 37760
    sync           2 22 34976
    console        3 23 0
    $ wc README
    48 336 2441 README

## Part 3 - Reading the source

1. user/cat.c uses these system calls:
   - read(fd, buf, sizeof(buf)) : copy up to 512 bytes from the open file into buf
   - write(1, buf, n)           : send n bytes from buf to descriptor 1 (the console)
   - open(argv[i], O_RDONLY)    : find the file by name, give back a descriptor
   - close(fd)                  : give the descriptor back
   - exit(status)               : end the process and report the status
   (fprintf(2, ...) is a library function; in the end it also calls write.)
2. sys_read is in kernel/sysfile.c at line 69; sys_write is at line 83.
3. kernel/ is the operating system itself and runs privileged, with access to
   all the hardware and memory; user/ holds ordinary programs that run
   unprivileged and can get anything done only by asking the kernel through a
   system call.

## Part 4 - sleep

The sheet's sleep.c calls sleep(), which the current xv6 does not have:

    user/sleep.c:11:3: error: implicit declaration of function 'sleep'

The same system call is now called pause() (user/user.h, line 25), so my
sleep.c calls pause(atoi(argv[1])).

    $ sleep 10        (pauses about one second, then the prompt comes back)
    $ sleep
    usage: sleep <ticks>

pause() is system call 13 (SYS_pause); the kernel runs sys_pause in
kernel/sysproc.c, line 68, which waits until the tick counter has advanced by n.
One tick is about a tenth of a second.

## Extra - pingpong

    $ pingpong
    4: received ping
    3: received pong
