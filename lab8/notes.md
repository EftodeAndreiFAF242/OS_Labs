# Lab 8 - The Same Calls, Inside Your Own OS (xv6)

Student: Eftode Andrei, FAF-242
Tree: ~/xv6-riscv, branch lab (xv6 revision 06aad25 plus Labs 4 and 6)
New files: user/run.c, user/pingpong.c, user/primes.c
UPROGS: three new lines after $U/_zombie\ (TAB at the start):

	$U/_run\
	$U/_pingpong\
	$U/_primes\

## Part 1 - run, inside xv6

    $ run
    parent: my pid is 3
    child: my pid is 4
    hello from the child
    parent: child 4 ended with status 0

With exec("ls", args) the child listed the root directory (README, cat,
echo, ... console) and the parent saw status 0.
With exec("nosuchprog", args):

    exec failed
    parent: child 4 ended with status 1

1. xv6 run.c:   fork(), exec("echo", args), wait(&status)
   Linux run.c: fork(), execlp("ls", "ls", "-l", NULL), waitpid(pid, &status, 0)
   The same: the three steps, in the same order. fork copies the process
   and returns 0 in the child and the child's pid in the parent; exec
   replaces the child's program and returns only if it fails; wait puts
   the parent to sleep until the child ends and gives its exit status.
   Only the library around them differs: execlp searches PATH and waitpid
   waits for one given child; xv6 exec takes the path as written and wait
   takes any child.
2. echo prints it. After exec the child's memory holds the program echo
   (user/echo.c), which writes its arguments with write(1, ...). The code
   of run after exec never runs when exec works.
3. The status is the number the child gave to exit. In the xv6 run.c the
   line after exec is exit(1); in the Linux run.c it was exit(127). xv6
   wait copies that number as it is (p->xstate, kernel/proc.c); on Linux
   waitpid packs it into status and WEXITSTATUS takes it out.

## Part 2 - pingpong

    $ pingpong
    4: received ping
    3: received pong

The TODO lines:

    read(p2c[0], &b, 1);       // child: wait for the ping
    write(c2p[1], &b, 1);      // child: send it back
    b = 'x';
    write(p2c[1], &b, 1);      // parent: send the ping
    read(c2p[0], &b, 1);       // parent: wait for the pong

(b gets a value before it is sent, so the parent never sends an
uninitialised byte.)

1. A pipe carries bytes in one direction and does not know who reads them.
   With one pipe for both directions, the parent could read back its own
   ping before the child takes it, and the child would wait forever. With
   two pipes, p2c only goes parent -> child and c2p only child -> parent.
2. read(c2p[0], &b, 1) in the parent. A read on an empty pipe blocks until
   someone writes into it (or every write end is closed), so the parent
   sleeps until the child has replied. The child's read(p2c[0]) waits for
   the ping in the same way, which is why "ping" always comes first.

## Optional - primes

    $ primes
    prime 2
    prime 3
    prime 5
    prime 7
    prime 11
    prime 13
    prime 17
    prime 19
    prime 23
    prime 29
    prime 31

main writes 2 to 35 into the first pipe. Each stage reads its first number
p, which is always a prime, prints it, makes a pipe and a child (the next
stage), and forwards only the numbers p does not divide. When its input
ends it closes its write end, so the next stage gets end of file, and
waits for it. Every process closes the pipe ends it does not use.

## Conclusion

The calls of Lab 7 are the same in xv6: fork, exec, wait, pipe, read,
write and close. A new program reaches the xv6 disk only through a line in
UPROGS. A pipe is a one-way buffer and a read on an empty pipe blocks,
and that alone keeps the two processes of pingpong in step.
