# Lab 7 – Full explanations

Course: Operating Systems, FCIM / FAF, UTM, 2026-2027. Lab 7: Three Small Programs: Processes, Redirection, a Pipe.

[`README.md`](README.md) tells what was done in the lab. This file goes slower. It explains every line of the five programs (`run.c`, `zombie.c`, `redirect.c`, `pipeline.c`, `pipeline3.c`), every command typed in the terminal, and what the kernel does behind each system call. It is written for someone who has never used Linux or C system calls. The figure numbers are the same as in the README and in the Word report.

Lab 8 ([`../lab8/full_explanations.md`](../lab8/full_explanations.md)) runs the same calls inside xv6.

## Contents

1. [Words you need first](#1-words-you-need-first)
2. [Before the lab: compiling a C program](#2-before-the-lab-compiling-a-c-program)
3. [Part 1: run.c, fork + exec + wait](#3-part-1-runc-fork--exec--wait)
4. [Part 2: the zombie](#4-part-2-the-zombie)
5. [Part 3: redirection](#5-part-3-redirection)
6. [Part 4: the pipe](#6-part-4-the-pipe)
7. [Optional: three stages](#7-optional-three-stages)
8. [What the shell does, in one table](#8-what-the-shell-does-in-one-table)
9. [Test yourself](#9-test-yourself)

## 1. Words you need first

| Word | Meaning |
|---|---|
| kernel | the core of the operating system. It owns the processor, the memory, the files; programs ask it for things through system calls |
| system call | a request to the kernel: `fork`, `execve`, `wait4`, `exit`, `open`, `close`, `dup`, `pipe`, `read`, `write`... |
| library function | a normal C function that may call the kernel inside: `printf`, `perror`, `execlp`, `waitpid` are in the C library (glibc) |
| process | a running program: its memory, its registers, its open files, its PID |
| PID / PPID | the number of a process / the number of its parent |
| exit status | the small number (0 to 255) a process gives to `exit()`. 0 = success |
| file descriptor | a small number that stands for an open file (or pipe, or terminal) inside one process |
| pipe | a buffer in the kernel with a write end and a read end |
| zombie | a process that has ended but has not been collected by its parent's `wait` |
| `strace` | a tool that prints the system calls of a program as they happen |

Where everything runs:

```
Windows 11                     the real laptop
└── VirtualBox                 a pretend PC
    └── Ubuntu Linux           prompt:  vboxuser@os:~/os-lab7$
        ├── bash (PID 8877)    the shell I type into
        │   └── ./run          my program, a child of bash
        │       └── ls -l      the child of ./run, after exec
```

## 2. Before the lab: compiling a C program

```bash
mkdir -p ~/os-lab7 && cd ~/os-lab7
gcc --version | head -1
```

- `mkdir -p` creates the folder (and does not complain if it already exists). `cd` goes into it.
- `gcc --version` prints several lines; `| head -1` keeps the first: `gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0`.

Every program of the lab is built with the same line:

```bash
gcc -Wall -Wextra -o run run.c && ./run
```

| Piece | Meaning |
|---|---|
| `gcc` | the C compiler. It turns the text `run.c` into a program the processor can run |
| `-Wall -Wextra` | print warnings about suspicious code. All the programs of the lab compile with no warning at all |
| `-o run` | name the result `run` (without it, it would be `a.out`) |
| `run.c` | the source file |
| `&&` | run the next command only if this one succeeded |
| `./run` | run the program `run` from **this** folder. `./` is needed because the current folder is not in `PATH` |

## 3. Part 1: run.c, fork + exec + wait

### 3.1 Every line

```c
#include <stdio.h>        // printf, fflush, perror
#include <stdlib.h>       // exit
#include <unistd.h>       // fork, execlp, getpid, getppid
#include <sys/wait.h>     // waitpid, WEXITSTATUS
```

The `#include` lines bring in the declarations of the functions used below, so that the compiler knows their arguments.

```c
int main(void)
{
    printf("parent: my PID is %d\n", getpid());
    fflush(stdout);
```

- `getpid()` asks the kernel "what is my PID?". `%d` in `printf` is replaced by that number.
- `fflush(stdout)`: see 3.3. In short: empty the output buffer **before** `fork`, so it is not copied into the child.

```c
    pid_t pid = fork();                          /* step 1: copy myself */
    if (pid < 0) { perror("fork"); return 1; }
```

- `fork()` creates a second process, an exact copy of this one: same code, same variables, same open files, and it continues from the same point. Both processes return from `fork`, with different values:
  - in the **parent**, `fork` returns the PID of the child (a positive number);
  - in the **child**, `fork` returns **0**;
  - if the kernel cannot create a process, it returns -1 (only in the parent, since there is no child).
- `pid_t` is the type for PIDs (an integer).

```c
    if (pid == 0) {                              /* only the child runs this */
        printf("child:  my PID is %d, my parent is %d\n", getpid(), getppid());
        fflush(stdout);
        execlp("ls", "ls", "-l", (char *)NULL);  /* step 2: become ls */
        perror("exec");                          /* runs only if exec failed */
        exit(127);
    }
```

- Only the child enters this block, because only in the child `pid` is 0.
- `getppid()` gives the PID of the parent.
- `execlp(...)` asks the kernel to throw away the program of this process and load `ls` in its place (3.4). The process keeps its PID, its parent and its open files.
- If `execlp` works, it **never returns**: the lines below it are part of the old program, which no longer exists.
- If it fails, it returns -1. Then `perror("exec")` prints `exec: ` followed by the reason, and `exit(127)` ends the child with status 127.

```c
    int status;
    waitpid(pid, &status, 0);                    /* step 3: wait for the child */
    printf("parent: child %d ended with status %d\n", pid, WEXITSTATUS(status));
    return 0;
}
```

- Only the parent gets here: the child either became `ls` or called `exit`.
- `waitpid(pid, &status, 0)` puts the parent to sleep until the child with that PID ends. Then the kernel writes into `status` how it ended. `0` means "no special options".
- `WEXITSTATUS(status)` takes the exit code out of `status` (3.6).
- `return 0` from `main` ends the parent with status 0.

### 3.2 The two processes, step by step (Figure 1.2)

| Time | Parent (PID 9044) | Child (PID 9046) |
|---|---|---|
| 1 | prints "parent: my PID is 9044" | (does not exist yet) |
| 2 | `fork()` returns 9046 | `fork()` returns 0 |
| 3 | `waitpid(9046, ...)`: sleeps | prints "child: my PID is 9046, my parent is 9044" |
| 4 | still sleeping | `execlp("ls", ...)`: the process is now `ls` |
| 5 | still sleeping | `ls` prints the listing and calls `exit(0)` |
| 6 | wakes up, `status` says 0 | is gone |
| 7 | prints "parent: child 9046 ended with status 0" | |

The PIDs are different on every run (9044 here, 9205 in Figure 1.7), but the order of the lines is always the same, because the parent waits.

### 3.3 Why fflush

`printf` does not write to the screen at once. It collects the text in a buffer inside the process and writes it later with the system call `write`:

- when the output is a terminal, the buffer is written at every `\n`;
- when the output is a file or a pipe (for example `./run > file`), it is written only when it is full or when the program ends.

`fork` copies the whole memory of the process, buffer included. Suppose `./run > file` without the first `fflush`:

1. The parent's buffer holds "parent: my PID is 9044\n" (not yet written, because the output is a file).
2. `fork` copies it: now the child's buffer holds it too.
3. The child adds "child: ..." and calls `fflush`: both lines go into the file.
4. Later the parent ends and writes its own copy: "parent: my PID is 9044" appears a **second** time.

With `fflush(stdout)` before `fork`, the buffer is empty when it is copied. The child's `fflush` before `execlp` is needed for another reason: `exec` throws away the old program's memory, including any text still in its buffer.

### 3.4 What execlp means

The `exec` functions differ by the letters after `exec`:

| Letter | Meaning |
|---|---|
| `l` | the arguments are given as a **l**ist, one by one, ending with `NULL` |
| `v` | the arguments are given as a **v**ector (an array), like `argv` |
| `p` | if the name has no `/`, search for it in the folders of the **P**ATH variable |
| `e` | an **e**nvironment is given too |

So `execlp("ls", "ls", "-l", (char *)NULL)` means: find `ls` in `PATH`, start it with `argv[0] = "ls"`, `argv[1] = "-l"`. The first `"ls"` is where to find the program; the second is what the program sees as its own name. `(char *)NULL` ends the list; the cast is needed because `execlp` takes a variable number of arguments and must receive a real null pointer.

All of these are library functions. The real system call is `execve(path, argv, envp)`, which takes a full path. `execlp` builds each path from `PATH` and calls `execve` for each one until one works (Figure 1.6: the fourth try, `/usr/bin/ls`, returns 0).

### 3.5 When exec fails (Figures 1.4 and 1.5)

With `nosuchprog`, `strace` shows ten tries:

```
execve("/usr/local/sbin/nosuchprog", ...) = -1 ENOENT (No such file or directory)
execve("/usr/local/bin/nosuchprog", ...)  = -1 ENOENT
...
execve("/snap/bin/nosuchprog", ...)       = -1 ENOENT
```

one for each folder of `PATH` (`/snap/bin` appears twice in this `PATH`). After the last one, `execlp` returns -1 and the C library variable `errno` holds `ENOENT`. `perror("exec")` prints `exec: ` and the text of that error: `No such file or directory`.

127 is not an accident: it is the code bash itself uses for "command not found". When I type a name that does not exist, bash forks, the child's exec fails, and the child exits with 127, exactly like `run.c`.

`echo "exit code of ./run: $?"` printed 0: `$?` is the status of the last command, `./run`, and `./run` always ends with `return 0`. The 127 belongs to the child, not to `./run`.

### 3.6 What is inside status

`waitpid` does not store just the exit code in `status`. It packs several facts in one integer: did the child end normally or was it killed by a signal, and with which code. For a normal exit, the code is in the second byte:

```
exit(127)  ->  status = 127 * 256 = 32512
WEXITSTATUS(32512) = (32512 >> 8) & 0xff = 127
```

That is why the program prints `WEXITSTATUS(status)` and not `status`. `strace` shows the same test: `[{WIFEXITED(s) && WEXITSTATUS(s) == 127}]`.

### 3.7 Reading the strace lines (Figure 1.6)

```bash
strace -f -e trace=execve,clone,clone3,wait4,exit_group ./run > /dev/null
```

| Line | Meaning |
|---|---|
| `execve("./run", ["./run"], ... /* 55 vars */) = 0` | bash's child becomes `./run`. "55 vars" are the environment variables passed along |
| `clone(child_stack=NULL, flags=...\|SIGCHLD, ...) = 9135` | this is `fork()`. glibc implements it with the system call `clone`, which returns the child's PID |
| `strace: Process 9135 attached` | `-f` starts following the child |
| `[pid 9133] wait4(9135 <unfinished ...>` | the parent's `waitpid` is the system call `wait4`; it blocks |
| `[pid 9135] execve("/usr/local/sbin/ls", ...) = -1 ENOENT` | the child tries the folders of `PATH` |
| `[pid 9135] execve("/usr/bin/ls", ...) = 0` | found: the child is `ls` now |
| `[pid 9135] exit_group(0)` | `ls` ends with 0 |
| `<... wait4 resumed>, [{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 9135` | the parent wakes up with the status |
| `--- SIGCHLD {si_pid=9135, si_status=0, ...} ---` | the kernel also tells the parent with the signal SIGCHLD |
| `exit_group(0)` | the parent ends |

`> /dev/null` sends the normal output into nothing (`/dev/null` is a file that swallows everything), so only `strace`'s lines, written on descriptor 2, are on the screen.

### 3.8 Who is the parent (Figure 1.7)

```bash
./run ; echo "my shell has PID $$"
```

`$$` is the PID of the shell. The chain is: bash (8877) → `./run` (9205) → child (9206). The child prints `getppid()` = 9205, its direct parent. The shell is the grandparent.

## 4. Part 2: the zombie

### 4.1 The change

```c
    int status;
    sleep(20);                                   /* the new line */
    waitpid(pid, &status, 0);                    /* step 3: wait for the child */
```

`sleep(20)` stops the parent for 20 seconds. The child is not affected: it becomes `ls`, lists the folder, and ends at once. For 20 seconds the child has ended but the parent has not called `waitpid`.

The `sed` command that made the change:

```bash
sed -i 's|^    waitpid(pid, &status, 0);|    sleep(20);                                   /* the new line */\n&|' zombie.c
```

| Piece | Meaning |
|---|---|
| `-i` | change the file in place |
| `s\|A\|B\|` | replace `A` by `B`; `\|` is used as separator instead of `/` |
| `^    waitpid(pid, &status, 0);` | the start of the line (`^`), four spaces, then the call. In the pattern `&` is an ordinary character |
| `\n` | a new line |
| `&` in the replacement | "the text that was found". So the result is: the new line, a line break, then the old text |

### 4.2 The life of a process

```
fork()           exit() / end of main          parent calls wait()
  │                     │                              │
  ▼                     ▼                              ▼
created ──► running ──► ZOMBIE ───────────────────────► gone
                      (only PID and status left)
```

Why keep a zombie at all? Because the parent may still want to know the exit status, and it may ask late. The kernel frees the memory and closes the files of the dead child, but keeps the small entry with its PID and status until somebody collects it.

If the parent dies without waiting, its children are given to another process (PID 1, or a "subreaper" such as `systemd --user` on Ubuntu), which collects them. So zombies are a problem only when a parent lives long and forgets to wait.

### 4.3 Reading ps (Figure 2.1)

```bash
./zombie > /dev/null &
sleep 2; ps -o pid,ppid,stat,cmd
```

- `&` starts the program in the background. bash prints `[1] 9646`: job number 1, PID 9646.
- `sleep 2;` gives the child time to finish `ls`.
- `ps -o pid,ppid,stat,cmd` lists the processes of this terminal, with the four chosen columns.

The letters of `STAT`:

| Letter | Meaning |
|---|---|
| `R` | running, or ready to run |
| `S` | sleeping: waiting for something (a timer, a key, a child) |
| `Z` | zombie |
| `s` | session leader (the shell of the terminal) |
| `+` | in the foreground of the terminal |

```bash
grep State /proc/$(pgrep -n -x ls)/status
```

- `pgrep -n -x ls`: the PID of the newest (`-n`) process whose name is exactly (`-x`) `ls`: the zombie, 9648.
- `$( ... )` puts the output of the inner command into the line, so the path becomes `/proc/9648/status`.
- `/proc` is not on the disk; it is a window into the kernel's tables (Lab 5). The answer: `State: Z (zombie)`.

### 4.4 After 20 seconds (Figure 2.2)

The parent woke up, `waitpid` collected the status, the parent printed (into `/dev/null`) and ended. bash noticed that job 1 was over and printed `[1]+ Done ./zombie > /dev/null` before the next prompt. `ps` shows only the shell and `ps`.

## 5. Part 3: redirection

### 5.1 The descriptor table

Every process has a small table: descriptor number → open file. Here is the child of `redirect.c`, step by step:

| Moment | 0 | 1 | 2 |
|---|---|---|---|
| after `fork` (copied from the parent) | terminal | terminal | terminal |
| after `close(1)` | terminal | **free** | terminal |
| after `open("out.txt", ...)` returns 1 | terminal | **out.txt** | terminal |
| after `execlp("ls", ...)` | terminal | out.txt | terminal |

The rule that makes it work: **the kernel always gives the lowest free descriptor number.** 0 is taken, 1 is free, so `open` returns 1.

`exec` replaces the program but keeps this table, so `ls` starts with descriptor 1 already pointing to `out.txt`. `ls` does not know anything happened: it writes to 1, as every program does.

### 5.2 The open line

```c
int fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
```

| Piece | Meaning |
|---|---|
| `O_WRONLY` | open for writing only |
| `O_CREAT` | create the file if it does not exist |
| `O_TRUNC` | if it exists, empty it first (this is what `>` does; `>>` would use `O_APPEND` instead) |
| `\|` | combines the flags into one number |
| `0644` | permissions for a new file: rw- for me, r-- for the group, r-- for others. The `umask` (0002) removes nothing here, so the file is `-rw-r--r--` (Figure 3.2) |

### 5.3 What Figure 3.2 shows

```
open gave descriptor 1
done: the listing is in out.txt
```

- The first line is written by the child on descriptor 2 (`stderr`), which is still the terminal.
- The second is written by the parent, whose descriptor 1 was never touched: `close(1)` happened only in the child.
- `head -3 out.txt` shows `total 72` and the first entries. `out.txt` itself is listed with **0 bytes**: `open` had just created it empty, and `ls` read the folder before writing its output. At the end of the lab (Figure 6.2) it has 474 bytes, the whole listing.

### 5.4 The extra experiment (Figure 3.3)

`/tmp/redirect2.c` is `redirect.c` with six replacements made by one `sed` command (each `s/A/B/` separated by `;`): `close(2)` instead of `close(1)`, `err.txt` instead of `out.txt`, `ls nosuchfile` instead of `ls -l`, and the comments and messages adapted. The descriptor table of the child:

| Moment | 0 | 1 | 2 |
|---|---|---|---|
| after `close(2)` | terminal | terminal | **free** |
| after `open("err.txt")` returns 2 | terminal | terminal | **err.txt** |

Now `fprintf(stderr, ...)` and the error message of `ls` both go into `err.txt`. That is the shell's `ls nosuchfile 2> err.txt`.

## 6. Part 4: the pipe

### 6.1 What a pipe is

`pipe(p)` asks the kernel for a buffer and two new descriptors:

- `p[0]`: the **read** end;
- `p[1]`: the **write** end.

Bytes written into `p[1]` come out of `p[0]`, in the same order. On Linux the buffer holds 64 KiB. Three rules decide how processes behave around it:

1. `read` on an **empty** pipe **waits**, as long as some write end is still open.
2. `read` on an empty pipe whose write ends are **all closed** returns 0: **end of file**.
3. `write` into a **full** pipe waits until the reader takes some bytes.

`wc -l` reads until end of file and only then prints its count. So it prints only when every write end of its pipe is closed, by everyone.

### 6.2 Every line of pipeline.c

```c
    int p[2];
    if (pipe(p) < 0) { perror("pipe"); return 1; }
```

After this, the parent has descriptors 0, 1, 2 (terminal), 3 = `p[0]`, 4 = `p[1]`.

```c
    if (fork() == 0) {                           /* ls: its 1 becomes p[1] */
        close(1);
        dup(p[1]);                               /* smallest free number: 1 */
        close(p[0]);
        close(p[1]);
        execlp("ls", "ls", "/etc", (char *)NULL);
        perror("exec ls");
        exit(127);
    }
```

`fork` copies the table, so the child has 3 and 4 too. `close(1)` frees 1; `dup(p[1])` copies descriptor 4 into the lowest free number, 1. Now 1 and 4 both point to the write end. `close(p[0])` and `close(p[1])` remove 3 and 4: the child keeps only what it needs.

```c
    if (fork() == 0) {                           /* wc: its 0 becomes p[0] */
        close(0);
        dup(p[0]);                               /* smallest free number: 0 */
        close(p[0]);
        close(p[1]);
        execlp("wc", "wc", "-l", (char *)NULL);
        perror("exec wc");
        exit(127);
    }
```

The same idea for `wc`: descriptor 0 becomes the read end.

```c
    close(p[0]);                                 /* the parent uses neither end */
    close(p[1]);
    wait(NULL);
    wait(NULL);
    return 0;
```

The parent closes both ends and waits for two children. `wait(NULL)` waits for any one child and ignores its status.

### 6.3 The descriptor tables, worked out

| Process | 0 | 1 | 3 (`p[0]`) | 4 (`p[1]`) |
|---|---|---|---|---|
| parent after `pipe` | terminal | terminal | read end | write end |
| `ls` before `exec` | terminal | **write end** | closed | closed |
| `wc` before `exec` | **read end** | terminal | closed | closed |
| parent before `wait` | terminal | terminal | closed | closed |

Count the write ends that stay open: exactly one, descriptor 1 of `ls`. When `ls` ends, it is closed, and `wc` gets end of file. That is Figure 4.2: `237`, the same as the shell.

### 6.4 The broken version (Figures 4.3 and 4.4)

With `/* close(p[1]); */` in the parent, the last row of the table changes:

| Process | 0 | 1 | 3 (`p[0]`) | 4 (`p[1]`) |
|---|---|---|---|---|
| parent before `wait` | terminal | terminal | closed | **write end, still open** |

Now two write ends exist. `ls` ends and closes its own, but the parent's stays open. So:

- `wc` has read all 237 lines, the pipe is empty, a write end is open: rule 1, `wc` **waits** for more.
- The parent is in its second `wait(NULL)`, waiting for `wc` to end.
- Each waits for the other, and nothing moves: a **deadlock**.

`time ./pipeline` showed `real 0m9.265s` (the time until Ctrl-C) but `user 0m0.007s` and `sys 0m0.012s`: in 9 seconds the processes used almost no processor time. They were asleep, not busy. Ctrl-C sends the signal SIGINT to every process in the foreground of the terminal, so the parent and `wc` were both stopped.

In Figure 4.4 the program runs in the background:

- `ps` shows `./pipeline` (10014) and `wc -l` (10017, its parent is 10014), both `S`. `ls` is gone: the parent's first `wait` collected it.
- `ls -l /proc/10017/fd` shows `0 -> pipe:[33792]`: `wc` is reading from a pipe.
- `kill $!` sends the signal SIGTERM to the PID in `$!`, the last program started with `&`: only the parent. When a process dies, the kernel closes all its descriptors, so the last write end disappears, and `wc` at once gets end of file and prints `237`.

A note on `kill %1`: `%1` means "job 1", and bash sends the signal to the whole job, the parent **and** its children. I tried it first: it killed `wc` too, so nothing was printed. `kill $!` kills only the parent, which shows the real cause.

### 6.5 What if a child forgot its close

The sheet's version also needs the `close(p[1])` inside the `wc` child. If `wc` kept a write end of its own pipe, it would wait for itself: the pipe could never have all its write ends closed while `wc` runs. The rule is the same everywhere: **every process closes every pipe end it does not use.**

## 7. Optional: three stages

### 7.1 The design

```
ls /usr/bin ──p──► grep x ──q──► wc -l
```

```c
int p[2], q[2];                                  /* p: ls -> grep,  q: grep -> wc */

void close_all(void)                             /* close all four pipe ends */
{
    close(p[0]); close(p[1]);
    close(q[0]); close(q[1]);
}
```

`p` and `q` are global (outside `main`) so that the small function `close_all` can reach them. Every child calls it after its `dup`s, and so does the parent: one line instead of four, and impossible to forget one.

```c
    if (pipe(p) < 0 || pipe(q) < 0) { perror("pipe"); return 1; }
```

`||` stops at the first failure.

The middle child, `grep`, changes both 0 and 1:

```c
        close(0);
        dup(p[0]);                               /* smallest free number: 0 */
        close(1);
        dup(q[1]);                               /* smallest free number: 1 */
        close_all();
        execlp("grep", "grep", "x", (char *)NULL);
```

The order matters: after `close(0)` the lowest free number is 0, so the first `dup` lands there; after `close(1)` it is 1.

The parent waits three times, once per child.

### 7.2 The tables, and what strace saw (Figure 5.3)

The pipes are `p` = [3, 4] and `q` = [5, 6] (`pipe2([3, 4], 0)` and `pipe2([5, 6], 0)`; `pipe2` is the system call behind `pipe`).

| Child | `dup` calls seen by strace | 0 | 1 |
|---|---|---|---|
| `ls` (10116) | `dup(4) = 1` | terminal | write end of `p` |
| `grep` (10117) | `dup(3) = 0`, `dup(6) = 1` | read end of `p` | write end of `q` |
| `wc` (10118) | `dup(5) = 0` | read end of `q` | terminal |

The ends that stay open after `close_all`: one write end of `p` (in `ls`), one write end of `q` (in `grep`). When `ls` ends, `grep` gets end of file and ends; then `wc` gets end of file and prints 224.

`2>&1` in the `strace` line sends `strace`'s output (descriptor 2) into the pipe too, so `grep -v ENOENT` can remove the failed tries of `execlp`. `-e signal=none` hides the SIGCHLD lines.

## 8. What the shell does, in one table

| I type | What bash does |
|---|---|
| `ls -l` | `fork`; the child does `execve("/usr/bin/ls", ...)`; the parent `wait`s (`run.c`) |
| `ls -l &` | the same, but the parent does not wait; it prints `[1] PID` and the prompt (Part 2) |
| `ls -l > out.txt` | in the child, before `exec`: `close(1)`, `open("out.txt", O_WRONLY\|O_CREAT\|O_TRUNC)` (`redirect.c`) |
| `ls x 2> err.txt` | the same with descriptor 2 (Figure 3.3) |
| `ls /etc \| wc -l` | `pipe`, two `fork`s, `dup` on 1 and 0, every unused end closed, two `wait`s (`pipeline.c`) |
| `a \| b \| c` | two pipes, three children (`pipeline3.c`) |
| a name that does not exist | the child's `exec` fails, it exits with 127, bash says "command not found" |

## 9. Test yourself

Try to answer before opening each one.

<details>
<summary>1. What does fork return in the parent, and what in the child?</summary>

In the parent: the PID of the child. In the child: 0. On failure: -1, in the parent (3.1).
</details>

<details>
<summary>2. Why is the line after execlp an error handler?</summary>

Because `execlp` returns only when it fails. When it works, the old program, with all its lines, is replaced by the new one (3.1).
</details>

<details>
<summary>3. In Figure 1.5, why are there ten execve lines for one execlp?</summary>

`execlp` tries every folder of `PATH`, one after the other, and each try is one `execve` system call. All ten failed with `ENOENT` (3.5).
</details>

<details>
<summary>4. Why did echo $? print 0 when the child ended with 127?</summary>

`$?` is the status of `./run`, the parent, which always ends with `return 0`. The 127 is the child's status, which only the parent sees through `waitpid` (3.5).
</details>

<details>
<summary>5. exit(127) — what number does waitpid put into status, and how do you get 127 back?</summary>

127 × 256 = 32512. `WEXITSTATUS(32512)` = 127 (3.6).
</details>

<details>
<summary>6. What would go wrong without the first fflush, with ./run > file?</summary>

"parent: my PID is ..." would be in the file twice: the unwritten buffer is copied by `fork`, and both processes write their copy (3.3).
</details>

<details>
<summary>7. What is a zombie, and what does it still hold?</summary>

A process that has ended but has not been collected by its parent's `wait`. Only its PID and exit status are left in the kernel's table (4.2).
</details>

<details>
<summary>8. What makes the zombie disappear in zombie.c?</summary>

`waitpid(pid, &status, 0);` after the 20 seconds (4.4).
</details>

<details>
<summary>9. What happens to the children of a parent that dies without waiting?</summary>

They are given to PID 1 or to a "subreaper" such as `systemd --user`, which waits for them (4.2).
</details>

<details>
<summary>10. In redirect.c, why does open return 1?</summary>

The kernel gives the lowest free descriptor. 0 is open, 1 was just closed (5.1).
</details>

<details>
<summary>11. Why does the message "open gave descriptor 1" still reach the screen?</summary>

It is written on descriptor 2, which still points to the terminal (5.3).
</details>

<details>
<summary>12. Why is out.txt 0 bytes in its own listing, and 474 bytes at the end?</summary>

`open` created it empty just before `ls` read the folder; then `ls` wrote the 474-byte listing into it (5.3).
</details>

<details>
<summary>13. When does a pipe give end of file?</summary>

When it is empty and every write end, in every process, is closed (6.1).
</details>

<details>
<summary>14. Without the parent's close(p[1]), who waits for whom?</summary>

`wc` waits for end of file, which needs the parent's write end closed; the parent waits in `wait(NULL)` for `wc` to end. A deadlock (6.4).
</details>

<details>
<summary>15. Why did time show 9 seconds of real time but almost no user or sys time?</summary>

The processes were asleep, waiting, not using the processor (6.4).
</details>

<details>
<summary>16. Why kill $! and not kill %1 to show the cause?</summary>

`%1` is the whole job, so `wc` would be killed too. `$!` is only the parent; when it dies its write end closes and `wc` finishes with 237 (6.4).
</details>

<details>
<summary>17. In pipeline3.c, which descriptors does grep end up with?</summary>

0 = read end of `p` (`dup(3) = 0`), 1 = write end of `q` (`dup(6) = 1`) (7.2).
</details>
