# Lab 8 – Full explanations

Course: Operating Systems, FCIM / FAF, UTM, 2026-2027. Lab 8: The Same Calls, Inside Your Own OS.

[`README.md`](README.md) tells what was done in the lab. This file goes slower. It explains every line of the three programs (`run.c`, `pingpong.c`, `primes.c`), my lines in the `Makefile`, every command typed in the terminal, and the kernel code of xv6 behind `fork`, `exec`, `exit`, `wait` and pipes. It is written for someone who has never used Linux or xv6. The figure numbers are the same as in the README and in the Word report.

Lab 7 ([`../lab7/full_explanations.md`](../lab7/full_explanations.md)) used the same calls on Linux. This lab looks at them inside a kernel small enough to read.

## Contents

1. [Words you need first](#1-words-you-need-first)
2. [Before the lab](#2-before-the-lab)
3. [Part 1: run.c inside xv6](#3-part-1-runc-inside-xv6)
4. [Inside the kernel: fork, exec, exit, wait](#4-inside-the-kernel-fork-exec-exit-wait)
5. [Part 2: pingpong](#5-part-2-pingpong)
6. [Inside the kernel: the pipe](#6-inside-the-kernel-the-pipe)
7. [Optional: primes](#7-optional-primes)
8. [The Makefile and the commit](#8-the-makefile-and-the-commit)
9. [xv6 and Linux side by side](#9-xv6-and-linux-side-by-side)
10. [Test yourself](#10-test-yourself)

## 1. Words you need first

| Word | Meaning |
|---|---|
| kernel | the core of the operating system. In xv6 it is the folder `kernel/` |
| user program | everything outside the kernel: the shell, `ls`, `echo`, `run`, `pingpong`. In xv6 they are in `user/` |
| system call | a request from a user program to the kernel: `fork`, `exec`, `wait`, `exit`, `pipe`, `read`, `write`, `close`, `getpid`... |
| QEMU | a program that pretends to be a RISC-V computer, so xv6 has something to run on |
| `fs.img` | a Linux file that is the whole disk of the pretend computer, built by `mkfs` |
| `UPROGS` | the list of programs in the Makefile that are built and put on the disk |
| process table | the array `proc[NPROC]` in `kernel/proc.c`: one `struct proc` per process, at most 64 |
| file descriptor | a small number that stands for an open file or pipe, at most 16 per process (`NOFILE`) |
| zombie | a process that has ended but has not been collected by `wait`. xv6 has this state too: `ZOMBIE` |

There are four computers inside each other:

```
Windows 11                        the real laptop
└── VirtualBox                    a pretend PC
    └── Ubuntu Linux, with bash       prompt:  vboxuser@os:~/xv6-riscv$
        └── QEMU                      a pretend RISC-V computer, its disk is the file fs.img
            └── xv6, with its shell   prompt:  $
```

To leave xv6 and come back to bash: press Ctrl-A, let go, then press X.

## 2. Before the lab

### 2.1 The state of the tree (Figure 0.1)

```bash
cd ~/xv6-riscv
git status -s ; git branch --show-current ; git log --oneline -3
```

- `git status -s` prints nothing: no uncommitted change.
- The branch is `lab`, and the last commits are mine: Lab 6 (`8cab251`) on top of Lab 4 (`5186256`) on top of the course revision `06aad25`.

```bash
ls user/run.c user/pingpong.c user/primes.c
sed -n '/^UPROGS/,/^$/p' Makefile | cat -A
```

- `ls` complains three times: none of the files of this lab exists yet.
- `sed -n '/A/,/B/p'` prints the lines from the first one matching `A` to the next one matching `B`: from `UPROGS=` to the first empty line (`^$` is "start, then end": an empty line).
- `cat -A` shows invisible characters: `^I` is a TAB, `$` the end of a line.

### 2.2 How a program gets onto the xv6 disk

```
user/run.c ──gcc──► user/run.o ──ld──► user/_run ──mkfs──► the file "run" inside fs.img
```

`make` does all of it, but only for the programs listed in `UPROGS`. A program missing from the list is never compiled and never put on the disk, so the xv6 shell answers `exec run failed`.

Each line of the list has a strict form: a TAB, `$U/_name`, a backslash, the end of the line. The backslash glues the lines into one long list; a space after it would break the Makefile.

## 3. Part 1: run.c inside xv6

### 3.1 Every line

```c
#include "kernel/types.h"
#include "user/user.h"
```

`kernel/types.h` defines short type names (`uint`, `uint64`...). `user/user.h` declares everything a user program may call: the system calls (`fork`, `exec`, `wait`...) and a few helper functions (`printf`, `strlen`...). There is no `stdio.h` or `unistd.h`: xv6 has no glibc.

```c
int
main(int argc, char *argv[])
{
  printf("parent: my pid is %d\n", getpid());
```

`getpid()` is a system call; `printf` is a small function in `user/printf.c`.

```c
  int pid = fork();                     // step 1: copy myself
  if(pid < 0){
    printf("fork failed\n");
    exit(1);
  }
```

Same as on Linux: the parent gets the child's pid, the child gets 0, -1 means failure (for example when all 64 slots of the process table are taken).

```c
  if(pid == 0){                         // only the child runs this
    printf("child: my pid is %d\n", getpid());
    char *args[] = { "echo", "hello", "from", "the", "child", 0 };
    exec("echo", args);                 // step 2: become echo
    printf("exec failed\n");            // runs only if exec failed
    exit(1);
  }
```

- `args` is an array of six pointers: five words and a 0 that marks the end. `echo` will receive `argc = 5` and these words as `argv`.
- `exec("echo", args)` is xv6's only exec: a path and an array (like `execv` on Linux). The kernel looks for the file `echo` starting from the **current directory**; there is no `PATH`. It works because the shell's current directory is `/`, where all the programs are.
- If `exec` works, it never returns. If it fails, `exit(1)`.

```c
  int status;
  wait(&status);                        // step 3: wait for the child
  printf("parent: child %d ended with status %d\n", pid, status);
  exit(0);
}
```

- `wait(&status)` sleeps until **any** child ends, writes its exit status into `status` and returns its pid.
- `status` is printed directly: xv6 does not pack it like Linux does (4.4).
- `exit(0)` ends the program; `exit` is declared `__attribute__((noreturn))`, "never comes back". In this revision of xv6, `return 0` from `main` would work too: every program really starts in `start()` (`user/ulib.c`), which calls `main` and then `exit` with what `main` returned ("wrapper so that it's OK if main() does not call exit()").

### 3.2 Why there is no fflush

In Lab 7 the program needed `fflush(stdout)` before `fork`. The xv6 `printf` has no buffer at all:

```c
static void
putc(int fd, char c)
{
  write(fd, &c, 1);
}
```

Every character is sent with its own `write` system call, at once. Nothing waits in memory, so `fork` cannot copy unwritten text and `exec` cannot lose any. (The price: one system call per character, which is slow; and two processes printing at the same time could mix their characters.)

### 3.3 The run, step by step (Figure 1.3)

| Pid | Who | What it does |
|---|---|---|
| 1 | `init` | the first process. It opens the console and starts the shell |
| 2 | `sh` | the shell. When I type `run`, it forks |
| 3 | `run` (the shell's child, after `exec("run")`) | prints "parent: my pid is 3", forks, waits |
| 4 | `run`'s child, then `echo` | prints "child: my pid is 4", becomes `echo`, prints "hello from the child", exits with 0 |

Then pid 3 wakes up in `wait`, gets status 0, prints the last line and exits; the shell, which was waiting for pid 3, prints the prompt `$` again.

xv6 gives pids from a counter (`nextpid` in `kernel/proc.c`) that only grows; a fresh boot always starts with `init` = 1 and `sh` = 2, so the first command is always 3. That is why my output is exactly the one on the sheet.

### 3.4 The three variants

| Variant | `args` | `exec` | Output | Status |
|---|---|---|---|---|
| sheet (handed in) | `{ "echo", "hello", "from", "the", "child", 0 }` | `exec("echo", args)` | hello from the child | 0 |
| 1.3 (Figure 1.5) | `{ "ls", 0 }` | `exec("ls", args)` | the listing of `/` | 0 |
| 1.4 (Figure 1.6) | `{ "ls", 0 }` | `exec("nosuchprog", args)` | exec failed | 1 |

The changes were made with `sed` on a copy of the original kept in `/tmp/run.c.orig`, and the original was copied back at the end (Figure 1.7):

```bash
sed -i 's|^    char \*args\[\] = .*|    char *args[] = { "ls", 0 };|; s|^    exec(.*|    exec("ls", args);                 // step 2: become ls|' user/run.c
```

| Piece | Meaning |
|---|---|
| `s\|A\|B\|` | replace `A` by `B`; two of them are separated by `;` |
| `^    char \*args\[\] = .*` | a line that starts with four spaces and `char *args[] = `, then anything (`.*`). `\*`, `\[`, `\]` are the plain characters, because alone they have special meanings in a pattern |
| `^    exec(.*` | the line of the `exec` call |

After each change, `make qemu` compiles `run.c` again (it changed) and builds a new `fs.img`.

## 4. Inside the kernel: fork, exec, exit, wait

All four live in `kernel/proc.c` and `kernel/exec.c`. The user-side names (`fork`) reach the kernel functions (`kfork`) through the system call table in `kernel/syscall.c`.

### 4.1 fork: kfork() in kernel/proc.c

```c
  // Allocate process.
  if ((np = allocproc()) == 0) {          // a free slot in proc[] and a new pid
    return -1;
  }

  // Copy user memory from parent to child.
  if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0) {
    ...
  }

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for (i = 0; i < NOFILE; i++)
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  ...
  np->parent = p;
  ...
  np->state = RUNNABLE;
  return pid;
```

- `allocproc` finds a free slot and gives the next pid.
- `uvmcopy` copies the parent's whole memory into the child's.
- The registers are copied too, so the child continues from the same instruction. Register `a0` holds the return value of a system call on RISC-V: setting it to 0 in the child is **why `fork` returns 0 in the child**. The parent gets `pid`.
- `filedup` gives the child the same open files (the same pipes, the same console). That is why pipes made before `fork` connect the two processes.

### 4.2 exec: kexec() in kernel/exec.c

```c
  // Open the executable file.
  if ((ip = namei(path)) == 0) {
    end_op();
    return -1;
  }
```

`namei` turns the path into an inode, starting from `/` if the path begins with `/`, otherwise from the current directory. If the file does not exist, `exec` returns -1 right here: that is the `exec failed` of Figure 1.6. If it exists, `kexec` checks that it is a program (an ELF file), builds a new memory with its code, puts the arguments on a new stack, and only then throws away the old memory. The pid, the parent and the open files are kept.

### 4.3 exit: kexit() in kernel/proc.c

```c
  // Close all open files.
  for (int fd = 0; fd < NOFILE; fd++) { ... fileclose(f); ... }
  ...
  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  ...
  p->xstate = status;
  p->state = ZOMBIE;
  ...
  // Jump into the scheduler, never to return.
  sched();
```

- All files are closed: this is what gives end of file to a pipe reader when a writer dies (6.3).
- The children of a dying process are given to `init`, which waits for them.
- The status is stored in the process (`xstate`) and the process becomes a **zombie**: it keeps its slot until its parent collects it. (xv6 even has a test program for this, `zombie`, in `UPROGS`.)

### 4.4 wait: kwait() in kernel/proc.c

```c
    for (pp = proc; pp < &proc[NPROC]; pp++) {
      if (pp->parent == p) {
        ...
        havekids = 1;
        if (pp->state == ZOMBIE) {
          // Found one.
          pid = pp->pid;
          if (addr != 0 &&
              copyout(p->pagetable, p->sz, addr, (char *)&pp->xstate,
                      sizeof(pp->xstate)) < 0) {
            ...
          }
          ...
          freeproc(pp);
          ...
          return pid;
        }
      }
    }
    // No point waiting if we don't have any children.
    if (!havekids || killed(p)) { ... return -1; }
    // Wait for a child to exit.
    sleep_prepare(p);
```

- It looks through the whole process table for its own children that are zombies.
- If it finds one, `copyout` writes the child's `xstate` into the parent's variable (`addr` is the address of `status`) **as it is**: `exit(1)` gives status 1. On Linux, `waitpid` packs it (`1 << 8`) and needs `WEXITSTATUS`.
- `freeproc` frees the slot: the zombie is gone.
- If there are children but none has ended, the parent sleeps; `kexit` of a child wakes it up.
- If there are no children at all, `wait` returns -1 at once.

## 5. Part 2: pingpong

### 5.1 Every line

```c
  int p2c[2], c2p[2];     // parent-to-child, child-to-parent
  char b;

  pipe(p2c);
  pipe(c2p);
```

Two pipes, so four new descriptors. In a fresh process with 0, 1, 2 open, they are 3, 4 (`p2c`) and 5, 6 (`c2p`). In each pair, `[0]` is the read end and `[1]` the write end.

```c
  if(fork() == 0){
    close(p2c[1]); close(c2p[0]);       // the ends the child does not use
    read(p2c[0], &b, 1);                // read 1 byte from p2c[0] into b
    printf("%d: received ping\n", getpid());
    write(c2p[1], &b, 1);               // write the byte b into c2p[1]
    exit(0);
  }
```

The child reads from `p2c` and writes into `c2p`, so it closes the write end of `p2c` and the read end of `c2p`. `read(p2c[0], &b, 1)`: read at most 1 byte from the pipe into the variable `b` (`&b` is its address). It waits until the byte arrives.

```c
  close(p2c[0]); close(c2p[1]);         // the ends the parent does not use
  b = 'x';
  write(p2c[1], &b, 1);                 // write 1 byte into p2c[1]
  read(c2p[0], &b, 1);                  // read 1 byte from c2p[0] into b
  printf("%d: received pong\n", getpid());
  wait(0);
  exit(0);
```

- The parent closes the opposite ends.
- `b = 'x';` is my addition: the sheet's skeleton sends `b` without giving it a value. Any byte would work for the game, but sending a variable that was never set is a bug in C, and it is cleaner to send a known byte.
- `wait(0)`: wait for the child; 0 means "I do not need the status".

| Process | `p2c[0]` (3) | `p2c[1]` (4) | `c2p[0]` (5) | `c2p[1]` (6) |
|---|---|---|---|---|
| parent | closed | **writes the ping** | **reads the pong** | closed |
| child | **reads the ping** | closed | closed | **writes the pong** |

### 5.2 Why the order is always the same

Nothing in the code says "wait for the child". The waiting comes from `read`:

1. The child may run first: `read(p2c[0])` finds the pipe empty and sleeps.
2. The parent writes the ping. The kernel wakes up the child.
3. The parent calls `read(c2p[0])`: empty, it sleeps.
4. The child prints "received ping" and writes the pong. The kernel wakes up the parent.
5. The parent prints "received pong".

Whichever process the scheduler runs first, the parent cannot print before step 4, so "ping" is always first.

### 5.3 Why two pipes

A pipe is one buffer with no idea who is reading. With a single pipe for both directions:

1. The parent writes the ping, then calls `read` to get the pong.
2. If the parent's `read` runs before the child's, it takes **its own** ping back.
3. The child then waits forever for a byte that is gone.

With two pipes, the parent only reads from `c2p`, where only the child writes.

### 5.4 The pids in Figure 2.2

| Run | `pingpong` (parent) | child | Lines printed |
|---|---|---|---|
| 1 | 3 | 4 | `4: received ping`, `3: received pong` |
| 2 | 5 | 6 | `6: received ping`, `5: received pong` |
| 3 | 7 | 8 | `8: received ping`, `7: received pong` |

Each run uses two new pids, because xv6 never reuses a pid during one boot.

## 6. Inside the kernel: the pipe

All of it is in `kernel/pipe.c`, about 140 lines.

### 6.1 The structure

```c
#define PIPESIZE 512

struct pipe {
  struct spinlock lock;
  char data[PIPESIZE];
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};
```

- The buffer is only 512 bytes (Linux: 64 KiB). It is used in a circle: byte number `n` is stored in `data[n % 512]`.
- `nwrite - nread` is how many bytes are waiting. Empty: `nread == nwrite`. Full: `nwrite == nread + 512`.
- `readopen` and `writeopen` say whether the two ends still exist.

### 6.2 read: piperead()

```c
  while (pi->nread == pi->nwrite && pi->writeopen) { //DOC: pipe-empty
    ...
    sleep();
    ...
  }
  for (i = 0; i < n; i++) { //DOC: piperead-copy
    if (pi->nread == pi->nwrite)
      break;
    ch = pi->data[pi->nread % PIPESIZE];
    ... copyout(..., &ch, 1) ...
    pi->nread++;
  }
  wakeup(&pi->nwrite); //DOC: piperead-wakeup
  return i;
```

- **While the pipe is empty and a writer exists, sleep.** This one loop is the answer to the second question of Part 2.
- Then copy as many bytes as are there (up to `n`) and wake up a writer that might be waiting for space.
- If the pipe is empty and `writeopen` is 0, the loop is skipped, nothing is copied, and `read` returns 0: **end of file**.

### 6.3 write and close

- `pipewrite` copies the bytes in one by one; when the buffer is full it wakes up the reader and sleeps. If the read end is closed (`readopen == 0`), it returns -1 (Linux would send the signal `SIGPIPE`).
- The descriptors of a pipe are counted: `fork` and `dup` add a reference (`filedup`), `close` removes one (`fileclose` in `kernel/file.c`). Only when the **last** reference to the write end is closed does `pipeclose` set `writeopen = 0` and wake up the reader. This is the rule from Lab 7, seen from inside: one forgotten write end, anywhere, and the reader never sees end of file.

## 7. Optional: primes

### 7.1 The idea

It is the "sieve" of the MIT 6.1810 util lab. Each process is one stage of a chain:

```
main            writes 2 3 4 5 6 7 8 9 10 11 12 ... 35
stage "2"       reads 2 → prints "prime 2", passes 3 5 7 9 11 13 15 ... 35
stage "3"       reads 3 → prints "prime 3", passes 5 7 11 13 17 19 23 25 29 31 35
stage "5"       reads 5 → prints "prime 5", passes 7 11 13 17 19 23 29 31
stage "7"       reads 7 → prints "prime 7", passes 11 13 17 19 23 29 31
...
stage "31"      reads 31 → prints "prime 31", passes nothing
last stage      reads nothing → ends
```

A number reaches stage `p` only if no smaller prime divides it. So the first number a stage sees is always a prime.

### 7.2 Every line

```c
void stage(int left) __attribute__((noreturn));
```

A declaration before the code, telling the compiler that `stage` never comes back (it always ends with `exit`), like `exit` itself in `user/user.h`.

```c
void
stage(int left)
{
  int p, n;

  if(read(left, &p, sizeof(p)) != sizeof(p)){  // nothing came: the chain ends here
    close(left);
    exit(0);
  }
  printf("prime %d\n", p);                     // the first number is always a prime
```

- The numbers travel as `int`s, 4 bytes each (`sizeof(p)`).
- If `read` does not return 4, the pipe gave end of file: no number at all reached this stage. This happens only to the last stage, after 31.

```c
  int right[2];
  pipe(right);
  if(fork() == 0){                             // the next stage reads from right[0]
    close(right[1]);
    close(left);
    stage(right[0]);
  }
```

The child becomes the next stage. It closes `right[1]` (it must not hold a write end of its own input, or it would never see end of file) and `left` (the input of the current stage, which it does not need).

```c
  close(right[0]);
  while(read(left, &n, sizeof(n)) == sizeof(n)){
    if(n % p != 0)                             // forward only what p does not divide
      write(right[1], &n, sizeof(n));
  }
  close(left);
  close(right[1]);                             // the next stage gets end-of-file
  wait(0);
  exit(0);
}
```

- The current stage keeps only `left` (to read) and `right[1]` (to write).
- `n % p` is the remainder of `n / p`; when it is not 0, `p` does not divide `n`, so `n` goes on.
- When `left` gives end of file, the stage closes `right[1]`: the next stage will get end of file after the last number. Then it waits for its child, so that the chain ends from the back to the front and the shell prints `$` only after `prime 31`.

```c
int
main(int argc, char *argv[])
{
  int p[2];

  pipe(p);
  if(fork() == 0){
    close(p[1]);
    stage(p[0]);
  }
  close(p[0]);
  for(int i = 2; i <= 35; i++)
    write(p[1], &i, sizeof(i));
  close(p[1]);
  wait(0);
  exit(0);
}
```

`main` starts the first stage, writes 34 numbers (136 bytes, well under the 512-byte buffer), closes the pipe and waits.

### 7.3 Counting processes and descriptors

- Processes: `main`, 11 prime stages, and the last stage that reads nothing: 13. The limit is 64.
- Open descriptors in one stage: 0, 1, 2, `left`, `right[1]`: 5. The limit is 16. If the children did not `close(left)`, every stage would also keep the input of all the stages before it, and the chain could not reach the end.

## 8. The Makefile and the commit

My three lines, right after `$U/_zombie\`, where the sheet's example puts them (Figure 3.3):

```
	$U/_zombie\
	$U/_run\
	$U/_pingpong\
	$U/_primes\
```

Each was added with the same kind of `sed` (Figure 1.2):

```bash
sed -i 's|^\t$U/_zombie\\$|&\n\t$U/_run\\|' Makefile
```

| Piece | Meaning |
|---|---|
| `^\t` | the start of the line, then a TAB |
| `$U/_zombie` | plain text (a `$` that is not at the end of the pattern is an ordinary character) |
| `\\$` | a backslash (`\\`), then the end of the line (`$`) |
| `&` | in the replacement: the line that was found |
| `\n\t$U/_run\\` | a new line, a TAB, `$U/_run`, a backslash |

The Makefile handed in is the one of the VM, so it also lists `sleep` and `args` (Lab 4) and `mystat` and `find` (Lab 6). To build it from this repository, copy those four sources too (see "How to run it" in the README); without them `make` stops with "No rule to make target".

The commit (Figure 4.3) changes 5 files: the three programs, 3 lines of the Makefile, and `notes.md`, which replaces the Lab 6 notes in the tree (hence "101 deletions"; the Lab 6 version is still in its own commit and in `../lab6/notes.md`).

## 9. xv6 and Linux side by side

| | Linux (Lab 7) | xv6 (Lab 8) |
|---|---|---|
| headers | `stdio.h`, `stdlib.h`, `unistd.h`, `sys/wait.h` | `kernel/types.h`, `user/user.h` |
| fork | `fork()`, done with the system call `clone` | `fork()` → `kfork()` |
| exec | `execlp` searches `PATH`, then `execve` | `exec(path, argv)`, no `PATH`: relative to the current directory |
| wait | `waitpid(pid, &status, 0)`, one given child | `wait(&status)`, any child |
| status | packed: `exit(127)` gives 32512, read with `WEXITSTATUS` | as it is: `exit(1)` gives 1 |
| exit code after a failed exec | 127, the shell's "command not found" | 1, as written in `run.c` |
| printf | buffered, needs `fflush` before `fork` | one `write` per character, no buffer |
| pipe buffer | 64 KiB | 512 bytes |
| write to a pipe with no reader | signal `SIGPIPE` | `write` returns -1 |
| zombie | state `Z` in `ps` | state `ZOMBIE` in `proc[]` |

## 10. Test yourself

Try to answer before opening each one.

<details>
<summary>1. Why does a new program need a line in UPROGS?</summary>

`make` compiles and puts on the disk only the programs in that list. Without the line, `run` is not on the xv6 disk and the shell cannot start it (2.2).
</details>

<details>
<summary>2. Why is the parent pid 3 and the child pid 4 on a fresh boot?</summary>

`init` is 1, the shell is 2. The shell forks 3 for `run`, which forks 4. Pids come from a counter that only grows (3.3).
</details>

<details>
<summary>3. How does the kernel make fork return 0 in the child?</summary>

`kfork` copies the parent's registers into the child and then sets the child's register `a0`, which holds the return value, to 0 (4.1).
</details>

<details>
<summary>4. Why does exec("echo", args) work from / but would fail after cd into another directory?</summary>

xv6 `exec` has no `PATH`: `namei` looks for `echo` in the current directory unless the path starts with `/`. The programs are only in `/` (3.1, 4.2).
</details>

<details>
<summary>5. Who prints "hello from the child"?</summary>

`echo` (`user/echo.c`). After `exec`, the child's memory holds `echo`, not `run` (3.3).
</details>

<details>
<summary>6. Why is the status 1 here and 127 in Lab 7, and why does xv6 not need WEXITSTATUS?</summary>

It is the number each `run.c` gives to `exit` after a failed exec. xv6 `wait` copies `xstate` as it is; Linux packs it into `status` (4.4).
</details>

<details>
<summary>7. Why does the xv6 run.c have no fflush?</summary>

The xv6 `printf` writes every character with its own `write`; there is no buffer to flush (3.2).
</details>

<details>
<summary>8. What does kexit do with the files and the children of a dying process?</summary>

It closes all the files (which can give end of file to pipe readers) and gives the children to `init` (4.3).
</details>

<details>
<summary>9. In pingpong, which call makes the parent wait for the pong?</summary>

`read(c2p[0], &b, 1)`. `piperead` sleeps while the pipe is empty and a writer exists (5.2, 6.2).
</details>

<details>
<summary>10. What could go wrong with one pipe for both directions?</summary>

The parent could read back its own ping before the child does, and the child would wait forever (5.3).
</details>

<details>
<summary>11. When does read on an xv6 pipe return 0?</summary>

When the pipe is empty and `writeopen` is 0: every write end, in every process, has been closed (6.2, 6.3).
</details>

<details>
<summary>12. How big is an xv6 pipe, and what happens when it is full?</summary>

512 bytes. `pipewrite` wakes up the reader and sleeps until there is space (6.1, 6.3).
</details>

<details>
<summary>13. In primes, why is the first number a stage reads always a prime?</summary>

A number reaches stage `p` only if no smaller prime divided it; the smallest such number is a prime (7.1).
</details>

<details>
<summary>14. In primes, why must the child close right[1]?</summary>

`right[0]` is its input. If it kept a write end of its own input, it would never see end of file, and the chain would never end (7.2).
</details>

<details>
<summary>15. How many processes does primes create in total, and is that a problem?</summary>

13 (main, 11 prime stages, one last empty stage). The limit is 64, so no (7.3).
</details>
