# Lab 4 – Full explanations

Course: Operating Systems, FCIM / FAF, UTM, 2026-2027. Lab 4: The xv6 Shell from the Outside.

[`README.md`](README.md) tells what was done in the lab. This file goes slower. It explains every line of the programs I wrote (`sleep.c`, `args.c`, `args_linux.c`), my lines in the `Makefile`, every command typed in the terminal, and then walks through `user/sh.c`, the xv6 shell, function by function. It is written for someone who has never used Linux or xv6. The figure numbers are the same as in the README and in the Word report.

Lab 3 ([`../lab3/full_explanations.md`](../lab3/full_explanations.md)) showed what bash does to a line before a program starts. This lab puts bash next to a shell that does almost none of that, and reads the code to see why.

## Contents

1. [Words you need first](#1-words-you-need-first)
2. [Before the lab](#2-before-the-lab)
3. [Part 1: fix sleep for the current xv6](#3-part-1-fix-sleep-for-the-current-xv6)
4. [Part 2: bash versus the xv6 shell](#4-part-2-bash-versus-the-xv6-shell)
5. [Part 3: who expands the star](#5-part-3-who-expands-the-star)
6. [Part 4: reading sh.c](#6-part-4-reading-shc)
7. [The xv6 tree at the end](#7-the-xv6-tree-at-the-end)
8. [bash and the xv6 shell side by side](#8-bash-and-the-xv6-shell-side-by-side)
9. [Test yourself](#9-test-yourself)

## 1. Words you need first

| Word | Meaning |
|---|---|
| kernel | the core of an operating system. It owns the hardware and runs the programs |
| user program | everything that is not the kernel: `ls`, `echo`, the shell itself |
| system call | a request from a user program to the kernel: "open this file", "start a process", "wait 20 ticks" |
| xv6 | a very small teaching operating system from MIT. Its kernel is about 6,600 lines of code, and with all its user programs it is about 12,000 |
| RISC-V | the kind of processor xv6 is written for. My laptop has a different kind |
| QEMU | a program that pretends to be a RISC-V computer, so that xv6 has something to run on |
| compile | turn a C file that people read (`sleep.c`) into a file the processor can run |
| `make`, `Makefile` | `make` builds a project by following the recipes written in the file `Makefile`. It rebuilds only what has changed |
| `fs.img` | one Linux file that is the whole disk of the xv6 computer |
| commit, branch | git words. A commit is a saved copy of all the files. A branch is a named row of commits |
| `argc`, `argv` | what every C program receives when it starts: the number of words on its command line, and the words |

There are four computers inside each other in this lab:

```
Windows 11                        the real laptop
└── VirtualBox                    a pretend PC
    └── Ubuntu Linux, with bash       prompt:  vboxuser@os:~/xv6-riscv$
        └── QEMU                      a pretend RISC-V computer
            └── xv6, with its shell   prompt:  $
```

So there are **two prompts** in the pictures. A long one with `vboxuser@os` is bash on Linux. A lonely `$` is the xv6 shell. To leave xv6 and come back to bash: press Ctrl-A, let go, then press X.

### How any shell starts a program

Every shell, bash and xv6 alike, is built on three system calls.

| Call | What it does |
|---|---|
| `fork()` | makes a copy of the process that calls it. Now there are two: the parent and the child |
| `exec(file, argv)` | replaces the program inside the current process with another one, and hands it `argv` |
| `wait()` | the parent sleeps until a child ends |

```
shell (parent)                         child (a copy of the shell)
   │
   ├── fork() ───────────────────────►  starts as a copy
   │                                    exec("ls", argv)   the copy turns into ls
   ├── wait()                           ls runs and prints
   │                                    exit(0)            the child ends
   ◄────────────────────────────────────
   prints the prompt again
```

Windows starts a program with one single call. Unix uses two, and the gap between them is useful: after `fork` and before `exec`, the child can connect its own output to a file or to a pipe. The program that comes after `exec` never knows. This is the reason redirection worked "without the program knowing" in Lab 3, and section 6.5 shows the exact lines.

## 2. Before the lab

```bash
cd ~/Downloads
bash os2026-setup.sh --check
```

![Figure 0.1 – The setup script in check mode](screenshots/fig-0-1.png)

This is the course script from Lab 3. With `--check` it installs nothing, which is why the output starts at step `2/4`: it checks the tools, finds xv6 in `~/xv6-riscv`, builds it and boots it once. The last line, **All set.**, means the machine is ready.

## 3. Part 1: fix sleep for the current xv6

### 3.1 What broke

In Lab 2 I wrote `user/sleep.c`, a program that pauses for some time. It called the system call `sleep()`. In August 2025 the authors of xv6 renamed that system call to `pause()`. A program that still calls `sleep()` does not build on the xv6 of today.

In Lab 2 I worked on an older copy of xv6, the last one before the rename. The course now uses a newer one, revision `06aad25`, so this lab moves to it.

### 3.2 Which name does the tree use?

![Figure 1.1 – The grep result in user.h](screenshots/fig-1-1.png)

The seven commands of the picture:

```bash
cd ~/xv6-riscv
git status -sb
grep -n "pause\|sleep" user/user.h
git add -A && git commit -q -m "Lab 2: sleep, pingpong, notes" && git log --oneline -1
git switch -c lab 06aad25
grep -n "pause\|sleep" user/user.h
git log --oneline -1
```

| No. | What it does | What it printed |
|---|---|---|
| 1 | `cd`: go into the xv6 folder | |
| 2 | `git status`: show the state of the folder. `-s` = short, `-b` = also show the branch | `## lab2`: I am on branch `lab2`. ` M Makefile`: this file was modified. `??`: three new files git does not know yet. This is my Lab 2 work |
| 3 | `grep`: search the file `user/user.h` for `pause` **or** `sleep`. `-n` prints the line number. The backslash and bar between the two words mean "or" for `grep` | `25:int sleep(int);`: this old tree has `sleep` |
| 4 | `git add -A`: take all changes. `git commit -m "..."`: save them as a commit with this message (`-q` = quietly). `git log --oneline -1`: show the last commit in one line | `951ca9c (HEAD -> lab2) Lab 2: sleep, pingpong, notes`: the Lab 2 work is saved |
| 5 | `git switch -c`: create a new branch named `lab` that starts at commit `06aad25`, and move to it. The files on the disk are replaced by the files of that commit | `Switched to a new branch 'lab'` |
| 6 | the same search as number 3, in the new files | `25:int pause(int);`: the new name |
| 7 | where am I now? | `06aad25 (HEAD -> lab, ...) Add killzero test` |

`user/user.h` is the file to ask because it is the list of everything a user program is allowed to call. Line 25 of it is a promise to the compiler: "there is a function `pause` that takes one whole number and gives one back".

Saving the Lab 2 work first (number 4) matters. `git switch` replaces the files on the disk, and work that is not in a commit could be lost.

### 3.3 Seeing the problem

To see the failure with my own eyes, I put the old `sleep.c` into the new tree and tried to build it.

![Figure 1.4 – The old sleep.c does not build](screenshots/fig-1-4.png)

**Line 1.** `git show lab2:user/sleep.c > user/sleep.c`

`git show BRANCH:FILE` prints a file as it is saved on another branch. `>` writes that into `user/sleep.c` here (redirection, Lab 3).

**Line 2.** `sed -i 's|^\t$U/_echo\\$|&\n\t$U/_sleep\\|' Makefile`

This adds my program to the list `UPROGS` in the Makefile, right under the line of `_echo`.

| Piece | Meaning |
|---|---|
| `sed` | a program that changes text by a rule |
| `-i` | change the file itself. Without it `sed` only prints the result |
| `'...'` | single quotes, so that bash does not touch `$U` or the backslashes (Lab 3) |
| `s\|A\|B\|` | the rule "substitute": replace what matches A with B. `\|` is used as the separator because the text itself has a `/` in it |
| A: `^\t$U/_echo\\$` | `^` = start of a line, `\t` = a Tab, then the text `$U/_echo`, `\\` = one real backslash, `$` = end of the line. Together: the whole `_echo` line |
| B: `&\n\t$U/_sleep\\` | `&` = the text that was matched, so the `_echo` line stays. `\n` = a new line. Then a Tab, `$U/_sleep` and a backslash |

So one line becomes two:

```
before                  after

<Tab>$U/_echo\          <Tab>$U/_echo\
                        <Tab>$U/_sleep\
```

**Line 3.** `grep -n "sleep" user/sleep.c Makefile` searches two files at once, so each result starts with the name of its file. It shows the call `sleep(atoi(argv[1]));` on line 11 of `sleep.c`, and my new line at 135 of the Makefile. (Line 8 of `sleep.c`, the usage message, and line 24 of the Makefile, `sleeplock.o`, only happen to contain the same letters.)

**Line 4.** `make qemu 2>&1 | tail -7` builds xv6. `2>&1` sends the errors into the same stream as the normal output, and `tail -7` keeps the last 7 lines (both from Lab 3).

Reading the error:

```
user/sleep.c:11:3: error: implicit declaration of function 'sleep'
     │       │  │
     │       │  └── column 3
     │       └───── line 11
     └───────────── the file
```

"Implicit declaration" means: the program calls a function that no header has announced. `user.h` has no `sleep` any more, so the compiler does not know what `sleep` is, and it refuses to go on. The last line, `make: *** [<builtin>: user/sleep.o] Error 1`, is `make` saying that one step failed and it stops. xv6 does not start.

The line that begins with `cc1: note:` is not the problem. It is a side remark of the compiler about one of the options in the Makefile, and it only shows up when there is a real error next to it.

### 3.4 The fix

One line of the program changes: the call on line 11.

![Figure 1.2 – The updated sleep.c](screenshots/fig-1-2.png)

The change was made with `sed` again:

```bash
sed -i 's/sleep(atoi/pause(atoi/; s|// system call into the kernel|// was sleep() before Aug 2025|' user/sleep.c
```

There are two rules between the quotes, separated by `;`. The first replaces `sleep(atoi` with `pause(atoi`: that is the fix. The second replaces the old comment with a new one. It uses `|` as its separator, because the comment itself contains `/`.

`cat -n user/sleep.c` then prints the file with line numbers, and `grep -n "_sleep" Makefile` confirms that `$U/_sleep\` is on line 135.

### 3.5 sleep.c, every line

[`sleep.c`](sleep.c):

```c
 1  #include "kernel/types.h"
 2  #include "user/user.h"
 3
 4  int
 5  main(int argc, char *argv[])
 6  {
 7    if(argc != 2){
 8      fprintf(2, "usage: sleep <ticks>\n");
 9      exit(1);
10    }
11    pause(atoi(argv[1]));   // was sleep() before Aug 2025
12    exit(0);
13  }
```

| Line | What it does |
|---|---|
| 1 | `#include` pastes another file here before compiling. `kernel/types.h` gives short names for number types, such as `uint`. The next file needs them |
| 2 | `user/user.h` announces everything a user program may call: the system calls (`pause`, `exit`) and the small library (`atoi`, `fprintf`) |
| 4-5 | every C program starts in a function called `main`. `int` in front means it gives back a whole number. xv6 code writes that word on its own line. `argc` is the number of words on the command line, `argv` is the list of those words |
| 6, 13 | `{` and `}` open and close the body of `main` |
| 7 | `!=` means "is not equal to". If the number of words is not 2, the user typed the command wrong |
| 8 | print a reminder of how to use the program. The first `2` is the channel: 2 is the error channel, as in Lab 3. `\n` is "new line" |
| 9 | stop the program with exit code 1, which means failure |
| 10 | closes the `if` |
| 11 | the real work. `argv[1]` is the text `"20"`. `atoi` turns that text into the number 20. `pause(20)` asks the kernel not to run this process for 20 ticks. Everything after `//` is a comment |
| 12 | stop the program with exit code 0, which means success |

What `argc` and `argv` hold for `sleep 20`:

```
$ sleep 20

argc = 2
argv[0] = "sleep"     the name of the program
argv[1] = "20"        text, not a number yet
```

Four cases, followed through the code:

| Typed in xv6 | `argc` | What happens |
|---|---|---|
| `sleep 20` | 2 | line 7 is false, line 11 runs: `pause(20)`, about 2 seconds, then `exit(0)` |
| `sleep` | 1 | line 7 is true: prints `usage: sleep <ticks>` and `exit(1)` |
| `sleep 1 2` | 3 | line 7 is true: the same usage message |
| `sleep abc` | 2 | `atoi("abc")` finds no digit and gives 0. `pause(0)` returns at once |

A tick is one beat of the xv6 clock, about a tenth of a second. So 20 ticks is about 2 seconds.

**What `pause(20)` really is.** It is not an ordinary function. It is a system call, a jump from my program into the kernel:

1. My program calls `pause(20)`.
2. A tiny piece of code, generated from `user/usys.pl`, puts the number of this system call (13) where the kernel can find it and jumps into the kernel.
3. The kernel sees the number 13 and runs its function `sys_pause`, in `kernel/sysproc.c`. That function puts my process to sleep until the clock has ticked 20 more times.
4. The kernel wakes the process, and my program continues at line 12.

The rename of August 2025 changed the name in step 1. That is the whole reason line 11 had to change.

### 3.6 My lines in the Makefile

[`Makefile`](Makefile) is the xv6 Makefile with two lines added by me, 135 and 136:

```
132  UPROGS=\
133      $U/_cat\
134      $U/_echo\
135      $U/_sleep\        <- mine, Part 1
136      $U/_args\         <- mine, Part 3
137      $U/_forktest\
...
154      $U/_sync\
```

The numbers on the left are the line numbers of the file, and the two arrows are my notes. In the real file each of these lines starts with a Tab and ends right after the backslash.

| Piece | Meaning |
|---|---|
| `UPROGS=` | a variable: the list of user programs to build and to put on the xv6 disk |
| `\` at the end of a line | "the list goes on in the next line". Nothing may come after it, not even a space, or the list is cut there |
| the Tab at the start | each line of the list starts with one Tab, like the lines around it |
| `$U` | another variable. Line 2 of the Makefile says `U=user`, so `$U/_sleep` is `user/_sleep` |
| `_sleep` | the built program. The `_` in front stops Linux from mixing it up with its own `sleep`. It is taken off when the file is copied to the xv6 disk, so inside xv6 the name is `sleep` |

Line 135 was added in Part 1 and line 136 in Part 3. A program that is not in this list is never built, and can never be run inside xv6.

**What one new line sets in motion.** `make qemu` works backwards from what I ask for to what it needs:

| Step | `make` reasons | Where in the Makefile |
|---|---|---|
| 1 | to run `qemu` I need the kernel and `fs.img` | line 184 |
| 2 | `fs.img` needs every name in `UPROGS`, so also `user/_sleep` | line 156 |
| 3 | `user/_sleep` is made from `user/sleep.o` and the small library | line 106 |
| 4 | `user/sleep.o` is made from `user/sleep.c` | a rule built into `make` |

Then it runs the recipes in the opposite order. They can be seen at the top of the next picture.

### 3.7 Build, boot, test

![Figure 1.3 – sleep running in xv6](screenshots/fig-1-3.png)

Reading the picture from the top:

| Line on the screen | What it is |
|---|---|
| `riscv64-linux-gnu-gcc ... -c -o user/sleep.o user/sleep.c` | **compile.** `sleep.c` becomes `sleep.o`, machine code for RISC-V. The long list of options comes from the Makefile. No error this time |
| `riscv64-linux-gnu-ld ... -o user/_sleep user/sleep.o user/ulib.o user/usys.o user/printf.o user/umalloc.o` | **link.** My code is glued to the small xv6 library: `ulib.o` has `atoi`, `usys.o` has the system calls, `printf.o` has `fprintf`. The result is the program `user/_sleep` |
| the two `objdump` lines | they write `sleep.asm` and `sleep.sym`, listings for people who want to study the program. Not needed to run it |
| `mkfs/mkfs fs.img README user/_cat user/_echo user/_sleep ...` | **make the disk.** A new `fs.img` is built with the file `README` and all the programs on it. `user/_sleep` is in the list |
| `nmeta 47 ...`, `balloc: ...` | `mkfs` reports the layout of the disk: 2000 blocks |
| `qemu-system-riscv64 -machine virt ... -m 128M -smp 3 -nographic ... file=fs.img` | **start the pretend computer.** 128 MB of memory, 3 processors, no graphics window (the text comes into my terminal), and `fs.img` as its disk |
| `xv6 kernel is booting` | from here on it is xv6 that prints |
| `hart 2 starting`, `hart 1 starting` | the other two processors wake up. "hart" is the RISC-V word for a processor core |
| `init: starting sh` | `init`, the first user program, starts the shell |
| `$ sleep 20` | the xv6 prompt. The command waited about two seconds and printed nothing: line 11 ran |
| `$ sleep` then `usage: sleep <ticks>` | no number, so lines 8 and 9 ran |

In later pictures `make qemu` prints only the `qemu-system-riscv64` line. Nothing had changed, so `make` had nothing to rebuild.

## 4. Part 2: bash versus the xv6 shell

The same ten command lines were typed twice.

In bash, in a new folder that holds one empty file named `README`:

```bash
mkdir ~/shtest && cd ~/shtest && touch README
```

![Figure 2.1 – The commands in bash](screenshots/fig-2-1.png)

In xv6, after `make qemu`. The xv6 disk already has a file named `README`, which is why the bash folder was given one too:

![Figure 2.2 – The same commands in xv6](screenshots/fig-2-2.png)

Below, each line is explained with the place in `user/sh.c` that is responsible. Section 6 reads that code.

### 4.1 `echo hello > f ; cat < f` (redirection)

| | Printed |
|---|---|
| bash | `hello` |
| xv6 | `hello` |

`echo hello > f` writes the word into the file `f`. `cat < f` reads the file back through its input. Both shells can connect a program to a file. In xv6 this is `parseredirs` (line 381) and `case REDIR` (line 83).

### 4.2 `ls | grep READ` (pipe)

| | Printed |
|---|---|
| bash | `README` |
| xv6 | `README         2 2 2441` |

The output of `ls` becomes the input of `grep`, which keeps the lines that contain `READ`. The pipe works in both. The xv6 line is longer because the xv6 `ls` is a different, simpler program: it always prints three numbers after the name.

```
README         2    2    2441
  │            │    │     └── size in bytes
  │            │    └──────── inode number: the number of the file on the disk
  │            └───────────── type: 1 = folder, 2 = file, 3 = device
  └────────────────────────── name
```

### 4.3 `(echo a; echo b) | wc` (grouping)

| | Printed |
|---|---|
| bash | `2 2 4` (with wider gaps) |
| xv6 | `2 2 4` |

The brackets make one unit out of two commands, so the output of both goes into the pipe. `wc` counts what arrives: 2 lines, 2 words, 4 characters (`a`, new line, `b`, new line). In xv6 this is `parseblock` (line 406).

### 4.4 `echo one ; echo two` (sequence)

Both print `one` and then `two`. `;` runs one command after the other. In xv6: `case LIST` (line 93).

### 4.5 `echo bg &` (background)

| | Printed |
|---|---|
| bash | `[1] 8254`, then `bg`, then `[1]+  Done   echo bg` |
| xv6 | `bg` |

Both shells start the command and do not wait for it. bash also keeps a list of jobs: it gives the job a number (`[1]`), prints its process number (8254) and reports when it is done. The xv6 shell keeps no list: `case BACK` (line 125) is three lines that fork and do not wait.

### 4.6 `ls *` (globbing)

| | Printed |
|---|---|
| bash | `README  f` |
| xv6 | `ls: cannot open *` |

bash replaced `*` with the names in the folder before it started `ls` (Lab 3). The xv6 shell does not know that `*` is special, so `ls` received the one-character name `*`, tried to open a file with that name, and failed.

### 4.7 `echo $HOME` (variables)

| | Printed |
|---|---|
| bash | `/home/vboxuser` |
| xv6 | `$HOME` |

The xv6 shell has no variables. `$HOME` is five ordinary characters, passed to `echo` as they are.

### 4.8 `echo "a b"` (quotes)

| | Printed |
|---|---|
| bash | `a b` |
| xv6 | `"a b"` |

bash removes the quotes and keeps `a b` together as one argument. The xv6 shell does not know quotes. It cuts the line at the space and passes two arguments, `"a` and `b"`. The xv6 `echo` prints its arguments with one space between them, so the screen shows `"a b"`. It looks like what was typed, but it travelled as two pieces. Part 3 proves it.

### 4.9 `mkdir d ; cd d ; ls ..` (cd and PATH)

In bash it prints `README  d  f`, and the prompt changes to `~/shtest/d`: the folder was made, bash moved into it, and `ls ..` listed the folder above.

In xv6 it prints `exec cd failed` and then a long listing:

![Figure 2.4 – mkdir d ; cd d ; ls .. in xv6](screenshots/fig-2-4.png)

What happened, piece by piece:

1. `mkdir d` worked.
2. `cd d` failed. The xv6 shell treats `cd` in a special way only when the whole line **starts** with `cd ` (section 6.3). This line starts with `m`. So the shell looked for a program named `cd`, there is none, and `exec` failed.
3. The shell never moved, so it is still in the top folder `/`. `ls ..` lists the folder above the top folder, which is the top folder again.

The listing is the whole xv6 disk, and it is worth reading:

| Line | What it tells |
|---|---|
| `.` and `..`, both `1 1 1024` | "this folder" and "the folder above" have the same inode number, 1. At the top, the folder above is the folder itself |
| `README 2 2 2441` | the file found by `grep READ` in 4.2 |
| `cat`, `echo`, `ls`, `sh`, ... | every program of xv6 is an ordinary file in the top folder. This is the list `UPROGS` of the Makefile, without the `_` |
| `sleep 2 5 35280` | my program from Part 1 is on the disk. `args` is not there yet: it comes in Part 3 |
| `console 3 24 0` | type 3: a device, not a file. It is the screen and the keyboard |
| `f 2 25 6` | the file from 4.1. Its size is 6 bytes: the five letters of `hello` and one new line |
| `d 1 26 32` | type 1: the new folder |

Typed alone on a line, `cd d` works. Then comes the real surprise:

![Figure 2.3 – ls failing after cd d, and the working alternative](screenshots/fig-2-3.png)

| Typed | Result | Why |
|---|---|---|
| `cd d` | no message | the line starts with `cd `, so the shell itself moves into `d` |
| `ls` | `exec ls failed` | the xv6 shell has no `PATH`. It hands the name `ls` to `exec` exactly as typed, and `exec` looks for a file `ls` in the current folder, which is now `d`. The programs are in `/` |
| `/ls` | works | the name now says where the file is: in the top folder |
| `../ls` | works | "the file `ls` one folder up" |
| `cd ..` then `ls d` | works | back in `/`, where `ls` is found again |

The two lines that `ls` prints inside `d` are `.` (the folder `d` itself, inode 26) and `..` (the folder above, inode 1).

In bash this never happens, because bash searches the folders of `PATH` wherever I am (Lab 3).

One more thing can be seen here: this picture is a new start of xv6, and the folder `d` from the picture before still exists. `fs.img` is a real disk for xv6. What is written to it stays after xv6 is switched off.

### 4.10 Arrow up, Tab, Ctrl+R (history and completion)

In bash:

![Figure 2.5 – History, completion and Ctrl+R in bash](screenshots/fig-2-5.png)

- The up arrow brought back the earlier line `echo "a b"`, and Enter ran it again.
- I typed `cat RE` and pressed Tab: it became `cat README`. (The file is empty, so `cat` prints nothing.)
- Ctrl+R and then `one` found the earlier line `echo one ; echo two`.

In xv6 the same keys do nothing useful:

![Figure 2.6 – Arrow up, Tab and Ctrl+R in xv6](screenshots/fig-2-6.png)

The last lines of the picture, after `ls d`:

| What I pressed | What the xv6 shell received | Result |
|---|---|---|
| `echo one`, then up arrow, Enter | the arrow key sends three invisible characters. The shell takes them as the name of a command | `exec ... failed`. The same invisible characters also mean "cursor up" to my terminal, which is why `failed` landed one line higher, next to `one` |
| `ec`, Tab, `ho two` | Tab counts as white space, like a space. So the command is `ec` with the arguments `ho` and `two` | `exec ec failed` |
| Ctrl+R, then `one` | Ctrl+R is one more invisible character, glued to the front of `one` | `exec one failed` |

The xv6 shell reads plain characters, one after the other, until Enter. It has no list of old commands and nothing that looks at file names while I type.

## 5. Part 3: who expands the star

Part 2 suggests that bash changes the line and the xv6 shell does not. This part proves it with a program that prints exactly what it receives.

### 5.1 args.c, every line

[`args.c`](args.c), in xv6 `user/args.c`:

```c
 1  #include "kernel/types.h"
 2  #include "user/user.h"
 3
 4  int
 5  main(int argc, char *argv[])
 6  {
 7    printf("argc = %d\n", argc);
 8    for(int i = 0; i < argc; i++)
 9      printf("argv[%d] = [%s]\n", i, argv[i]);
10    exit(0);
11  }
```

| Line | What it does |
|---|---|
| 1-2 | the same two files as in `sleep.c`. `printf` and `exit` are announced in `user/user.h` |
| 4-6 | `main`, with the number of words (`argc`) and the words (`argv`) |
| 7 | print the number of words. `printf` prints the text between the quotes, and each `%d` is replaced by the next value after the comma, written as a number. `\n` ends the line |
| 8 | a loop. `int i = 0` starts a counter at 0. `i < argc` says "go on while the counter is smaller than the number of words". `i++` adds 1 after each turn. So `i` is 0, 1, 2, ... up to the last word |
| 9 | the line that is repeated. `%d` is replaced by `i`, and `%s` by the text `argv[i]`. The square brackets around `%s` are plain characters that are printed |
| 10 | end with exit code 0 |

The square brackets are the whole point. They show where each argument begins and ends, so a space or a quote inside an argument cannot hide.

![Figure 3.5 – Writing user/args.c](screenshots/fig-3-5.png)

The file was written in `nano`, the terminal editor (`^O` saves, `^X` leaves).

![Figure 3.1 – args.c and the UPROGS line](screenshots/fig-3-1.png)

The four commands of the picture:

```bash
cat user/args.c
sed -i 's|^\t$U/_sleep\\$|&\n\t$U/_args\\|' Makefile
grep -n -B1 -A1 "_args" Makefile | cat -A
git status -s
```

| No. | What it does |
|---|---|
| 1 | `cat`: print the file |
| 2 | `sed`: the same rule as in 3.3, one line lower. After the `_sleep` line, add a line for `_args` |
| 3 | `grep`: find the new line, and also show one line **b**efore it (`-B1`) and one **a**fter it (`-A1`). The result goes through a pipe to `cat -A`, which makes invisible characters visible |
| 4 | `git status -s`: what changed in the tree. The Makefile is modified, and there are two new files |

`cat -A` writes a Tab as `^I` and marks the end of every line with `$`. So `136:^I$U/_args\$` proves three things at once: the line starts with a Tab, ends with a backslash, and has nothing after the backslash.

### 5.2 args_linux.c, and the two differences

[`args_linux.c`](args_linux.c):

```c
 1  #include <stdio.h>
 2
 3  int
 4  main(int argc, char *argv[])
 5  {
 6    printf("argc = %d\n", argc);
 7    for(int i = 0; i < argc; i++)
 8      printf("argv[%d] = [%s]\n", i, argv[i]);
 9    return 0;
10  }
```

| | xv6 version | Linux version | Why |
|---|---|---|---|
| headers | `"kernel/types.h"` and `"user/user.h"` | `<stdio.h>` | on Linux `printf` comes from the big standard C library, and `stdio.h` announces it. xv6 does not have that library. It has its own small `printf`, announced in `user/user.h` |
| the end | `exit(0);` | `return 0;` | on Linux, giving 0 back from `main` ends the program with exit code 0. xv6 programs end by calling the system call `exit` themselves |

Lines 6 to 8 are the same, character for character.

![Figure 3.4 – args_linux.c compiled with gcc](screenshots/fig-3-4.png)

```bash
gcc -o args args_linux.c
```

| Piece | Meaning |
|---|---|
| `gcc` | the C compiler of Linux |
| `-o args` | name the result `args` |
| `args_linux.c` | the file to compile |

`ls` then shows `args` in green, the colour of files that can be run. The folder `~/os-lab4` also holds copies of `args.c` and `sleep.c`, so that the pattern `*.c` has something to match in the test.

The two programs come from the same idea but are not the same file. `gcc` built one for the processor of my laptop and for Linux. `riscv64-linux-gnu-gcc` (through `make qemu`) built the other for RISC-V and for xv6. Neither can run on the other system.

### 5.3 The same four lines in both shells

![Figure 3.2 – args output in xv6](screenshots/fig-3-2.png)

![Figure 3.3 – args_linux output in bash](screenshots/fig-3-3.png)

In bash the program is started as `./args`, because the current folder is not in `PATH` (Lab 3).

**Line 1: `args one   two   three`** (with three spaces between the words)

| | `argc` | Arguments |
|---|---|---|
| bash | 4 | `[./args]` `[one]` `[two]` `[three]` |
| xv6 | 4 | `[args]` `[one]` `[two]` `[three]` |

The same in both. Any amount of white space only separates words. The extra spaces are never given to the program. The only difference is `argv[0]`, which is the name as it was typed.

**Line 2: `args *.c "x y"`**

| | `argc` | Arguments |
|---|---|---|
| bash | 5 | `[./args]` `[args.c]` `[args_linux.c]` `[sleep.c]` `[x y]` |
| xv6 | 4 | `[args]` `[*.c]` `["x]` `[y"]` |

This is the most important line of the lab. What each shell did with it:

```
typed:        args  *.c  "x y"

bash:    1. replaces *.c with the matching names   →  args.c  args_linux.c  sleep.c
         2. keeps "x y" as one word, removes the quotes  →  x y
         result: 5 words

xv6:     cuts at every space, changes nothing
         result: 4 words:  args   *.c   "x   y"
```

**Line 3: `args $HOME`**

| | `argc` | Arguments |
|---|---|---|
| bash | 2 | `[./args]` `[/home/vboxuser]` |
| xv6 | 2 | `[args]` `[$HOME]` |

bash replaced the variable with its value. The xv6 shell passed the five characters.

**Line 4: `args 'single quoted'`**

| | `argc` | Arguments |
|---|---|---|
| bash | 2 | `[./args]` `[single quoted]` |
| xv6 | 3 | `[args]` `['single]` `[quoted']` |

bash kept the text between the quotes as one argument and removed the quotes. The xv6 shell cut at the space, and the quote characters stayed on the two pieces.

### 5.4 What it proves

The program is the same on both sides, and it prints exactly what it is given. The kernel does not change the arguments either: `exec` copies the words into the new program as they are. So every difference in the tables above was made **before** the program started, by the shell.

- **Who turns `*.c` into file names and removes the quotes?** The shell. bash did it before calling `exec`. The xv6 shell has no code for it.
- **What happened to the extra spaces?** Both shells cut the line into words at white space. Only the words reach the program.

## 6. Part 4: reading sh.c

The last part opens `user/sh.c`, the xv6 shell, to find the reason for everything seen above. The file is one ordinary user program.

### 6.1 The commands of the sheet

```bash
wc -l user/sh.c
grep -n "#define" user/sh.c
```

![Figure 4.1 – The command kinds defined in sh.c](screenshots/fig-4-1.png)

`wc -l` counts the lines: **499**. The whole user interface of xv6 fits in that. `grep -n "#define"` lists the constants at the top. A `#define` gives a name to a number. Five of them are the kinds of command the shell knows:

| Line | Name | Produced by | Example |
|---|---|---|---|
| 8 | `EXEC` | no special character: a plain command | `ls` |
| 9 | `REDIR` | `<`, `>`, `>>` | `echo hello > f` |
| 10 | `PIPE` | `\|` | `ls \| grep READ` |
| 11 | `LIST` | `;` | `echo one ; echo two` |
| 12 | `BACK` | `&` | `echo bg &` |

`MAXARGS 10` on line 14 is not a kind. It is the size of the list of words of one command. The last place is kept for the mark that ends the list, so one command can have at most 9 words.

```bash
grep -n "case " user/sh.c
```

![Figure 4.2 – Where each kind is run](screenshots/fig-4-2.png)

A `case` is one branch of a `switch`, the C way of saying "if the value is this, do that". The results fall into four groups:

| Lines | Function | What the cases are |
|---|---|---|
| 75 to 125 | `runcmd` | one case for each of the five kinds. This is where commands are really run |
| 284 to 294 | `gettoken` | the characters the shell recognises while it cuts the line into pieces |
| 391 to 397 | `parseredirs` | the three redirections. `'+'` is the code's own name for `>>` |
| 469 to 493 | `nulterminate` | the five kinds again, for a small clean-up job |

```bash
grep -n "cd" user/sh.c | head
sed -n '/Read and run input commands/,/^}/p' user/sh.c
```

![Figure 4.3 – The code that handles cd](screenshots/fig-4-3.png)

`grep -n "cd"` finds only one line, the error message at line 170. The test for `cd` itself compares single letters (`'c'`, `'d'`), so the two letters never stand together in it. To see the code around line 170, I added the `sed` line: `-n` means "print nothing unless told", and `/A/,/B/p` means "print from the line that contains A to the next line that matches B". Here B is `^}`, a line that starts with `}`: the end of the function.

### 6.2 A map of the file

| Lines | What is there |
|---|---|
| 1-5 | a comment and three `#include` |
| 8-14 | the constants: five kinds and `MAXARGS` |
| 16-50 | five `struct`: the shape of each kind of command in memory |
| 58-132 | `runcmd`: run a command |
| 134-143 | `getcmd`: print the prompt and read one line |
| 145-178 | `main`: the loop of the shell |
| 180-196 | `panic` (print a message and stop) and `fork1` (`fork`, and stop if it failed) |
| 201-264 | five small functions that create the five kinds |
| 268-269 | the white space characters and the special symbols |
| 271-326 | `gettoken` and `peek`: cut the line into pieces |
| 333-452 | `parsecmd`, `parseline`, `parsepipe`, `parseredirs`, `parseblock`, `parseexec`: understand the pieces |
| 455-499 | `nulterminate`: end every word properly |

The shell works in three steps, again and again: **read** a line (`getcmd`), **understand** it (`parsecmd`), **run** it (`runcmd`).

### 6.3 `main`: the loop of the shell

```c
145  int
146  main(void)
147  {
148    static char buf[100];
149    int fd;
150
151    // Ensure that three file descriptors are open.
152    while ((fd = open("console", O_RDWR)) >= 0) {
153      if (fd >= 3) {
154        close(fd);
155        break;
156      }
157    }
158
159    // Read and run input commands.
160    while (getcmd(buf, sizeof(buf)) >= 0) {
161      char *cmd = buf;
162      while (*cmd == ' ' || *cmd == '\t')
163        cmd++;
164      if (*cmd == '\n') // is a blank command
165        continue;
166      if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ') {
167        // Chdir must be called by the parent, not the child.
168        cmd[strlen(cmd) - 1] = 0; // chop \n
169        if (chdir(cmd + 3) < 0)
170          fprintf(2, "cannot cd %s\n", cmd + 3);
171      } else {
172        if (fork1() == 0)
173          runcmd(parsecmd(cmd));
174        wait(0);
175      }
176    }
177    exit(0);
178  }
```

| Lines | What they do |
|---|---|
| 148 | `buf` is the place where the typed line is kept: 100 characters. A longer line does not fit |
| 152-157 | make sure channels 0, 1 and 2 are open, all three connected to the console (screen and keyboard). `open` always gives the smallest free number, so the loop opens the console until the number it gets is 3 or more, and closes that extra one |
| 160 | read one line into `buf`. The loop ends when there is nothing more to read |
| 161-163 | skip spaces and Tabs at the start of the line |
| 164-165 | an empty line: do nothing, go back and read the next line |
| 166 | is the line `c`, `d`, space? This is the test for the built-in `cd` |
| 168 | the line ends with the Enter character. Cut it off: `cd d` + Enter becomes `cd d` |
| 169 | `cmd + 3` is the text after the first three characters: `d`. `chdir` is the system call that changes the current folder. The shell calls it **itself** |
| 170 | if it failed, print `cannot cd d` on channel 2 |
| 172 | every other line: `fork1()` makes a copy of the shell. In the copy (the child) `fork1` gives back 0 |
| 173 | the child understands the line (`parsecmd`) and runs it (`runcmd`). `runcmd` never comes back |
| 174 | the parent, the real shell, waits until the child has ended |
| 176 | back to line 160: print the prompt, read the next line |

**Why line 166 explains Part 2.** The test looks at the first three characters of the line, nothing else.

| Typed | First three characters | Result |
|---|---|---|
| `cd d` | `c`, `d`, space | lines 168-170: the shell changes folder |
| `mkdir d ; cd d ; ls ..` | `m`, `k`, `d` | lines 172-174: `cd d` is later run like a program, and `exec cd failed` |

**Why `cd` is not a separate program.** The comment on line 167 says it: `chdir` must be called by the parent, not the child. Every process has its own current folder. If `cd` ran in a child, the child would move, then end, and the shell would still be where it was. It is the same reason as in bash (Lab 3).

### 6.4 `getcmd`: the prompt

```c
134  int
135  getcmd(char *buf, int nbuf)
136  {
137    write(2, "$ ", 2);
138    memset(buf, 0, nbuf);
139    gets(buf, nbuf);
140    if (buf[0] == 0) // EOF
141      return -1;
142    return 0;
143  }
```

| Line | What it does |
|---|---|
| 137 | write the two characters `$` and space to channel 2. This is the prompt. It is the only `$` in the whole file |
| 138 | fill the buffer with zeros, to erase the line before |
| 139 | read characters from the keyboard, one by one, until Enter |
| 140-142 | nothing was read: tell `main` to stop. If not, tell it there is a line |

`gets` keeps every character it is given. There is no code for arrow keys, Tab or Ctrl+R here, which explains 4.10.

### 6.5 `runcmd`: the five kinds

`runcmd` receives a command and looks at its kind (`switch (cmd->type)`, line 71).

**EXEC: a plain command**

```c
 75    case EXEC:
 76      ecmd = (struct execcmd *)cmd;
 77      if (ecmd->argv[0] == 0)
 78        exit(1);
 79      exec(ecmd->argv[0], ecmd->argv);
 80      fprintf(2, "exec %s failed\n", ecmd->argv[0]);
 81      break;
```

| Line | What it does |
|---|---|
| 76 | look at the command as a plain command, to reach its list of words `argv` |
| 77-78 | no words at all: nothing to do |
| 79 | `exec(name, words)`: replace this process with the program whose file is called `argv[0]`, and give it the words. If it works, this line never comes back: the process is now `ls` or `echo` |
| 80 | so this line runs only when `exec` failed. It is the message seen in Part 2: `exec ls failed`, `exec cd failed` |

Line 79 is where `PATH` is missing. The name goes to `exec` exactly as it was typed. A name without `/` is looked for in the current folder only. bash would try each folder of `PATH` here.

**REDIR: `<`, `>`, `>>`**

```c
 83    case REDIR:
 84      rcmd = (struct redircmd *)cmd;
 85      close(rcmd->fd);
 86      if (open(rcmd->file, rcmd->mode) < 0) {
 87        fprintf(2, "open %s failed\n", rcmd->file);
 88        exit(1);
 89      }
 90      runcmd(rcmd->cmd);
 91      break;
```

| Line | What it does |
|---|---|
| 85 | close one channel: 1 for `>` and `>>`, 0 for `<` |
| 86 | open the file. `open` always gives the smallest free number, and that is the number just closed. So the file is now channel 1 (or 0) |
| 90 | run the command inside. It writes to "channel 1" as always, and the text lands in the file |

For `echo hello > f`: close channel 1, open `f` (it becomes channel 1), run `echo hello`. `echo` does not know. This is the gap between `fork` and `exec` from section 1.

**LIST: `;`**

```c
 93    case LIST:
 94      lcmd = (struct listcmd *)cmd;
 95      if (fork1() == 0)
 96        runcmd(lcmd->left);
 97      wait(0);
 98      runcmd(lcmd->right);
 99      break;
```

Run the left side in a child (95-96), wait for it to end (97), then run the right side (98).

**PIPE: `|`**

```c
101    case PIPE:
102      pcmd = (struct pipecmd *)cmd;
103      if (pipe(p) < 0)
104        panic("pipe");
105      if (fork1() == 0) {
106        close(1);
107        dup(p[1]);
108        close(p[0]);
109        close(p[1]);
110        runcmd(pcmd->left);
111      }
112      if (fork1() == 0) {
113        close(0);
114        dup(p[0]);
115        close(p[0]);
116        close(p[1]);
117        runcmd(pcmd->right);
118      }
119      close(p[0]);
120      close(p[1]);
121      wait(0);
122      wait(0);
123      break;
```

| Lines | What they do |
|---|---|
| 103 | ask the kernel for a pipe: a tube with two ends. `p[1]` is the end to write into, `p[0]` the end to read from |
| 105-111 | first child, the **left** command. Close channel 1, and `dup(p[1])` makes a copy of the write end, which gets the free number 1. Now its output goes into the tube. Then run the left command |
| 112-118 | second child, the **right** command. The same with channel 0 and the read end. Now its input comes out of the tube |
| 119-120 | the parent does not use the tube, so it closes both ends |
| 121-122 | wait for both children |

For `ls | grep READ`: `ls` writes its listing into the tube, `grep` reads it from the tube. Neither program knows about the other.

**BACK: `&`**

```c
125    case BACK:
126      bcmd = (struct backcmd *)cmd;
127      if (fork1() == 0)
128        runcmd(bcmd->cmd);
129      break;
```

Start the command in a child and do **not** wait. There is no `wait(0)` here, and no list where the job is written down. That is the whole background feature of xv6, and the reason bash printed `[1] 8254` in 4.5 and xv6 printed nothing.

### 6.6 From a line to a command: the parser

Before `runcmd` can run anything, `parsecmd` must understand the line. It works in two layers.

**Layer 1: cut the line into pieces (`gettoken`, lines 271-314).** Two lists decide everything:

```c
268  char whitespace[] = " \t\r\n\v";
269  char symbols[] = "<|>&;()";
```

- Line 268: the characters that separate words: space, Tab, and three kinds of line end.
- Line 269: the seven characters with a special meaning.

`gettoken` skips white space, then looks at the next character. If it is one of the symbols, that symbol is the piece. If it is anything else, a word begins, and the word goes on until the next white space or symbol (line 303).

**Every other character is an ordinary letter of a word.** That includes `*`, `$`, `"` and `'`. Following `args *.c "x y"` through it:

```
args *.c "x y"
│    │   │  │
│    │   │  └── word 4:  y"     (the quote is an ordinary character)
│    │   └───── word 3:  "x     (the space ended the word)
│    └───────── word 2:  *.c    (the star is an ordinary character)
└────────────── word 1:  args
```

Four words: exactly `argc = 4` and `[*.c] ["x] [y"]` from Part 3.

**Layer 2: build the command (lines 333-452).** Each function handles one level and calls the next for the level below.

| Function | Lines | Handles |
|---|---|---|
| `parseline` | 350-365 | `&` at the end, and `;` between commands |
| `parsepipe` | 367-378 | `\|` between commands |
| `parseexec` | 421-452 | one plain command: collects its words into `argv` |
| `parseredirs` | 380-403 | `<`, `>`, `>>` and the file name after them |
| `parseblock` | 405-419 | a group in brackets `( )` |

The result is a small tree. The lines of Part 2 as trees:

```
ls | grep READ                    PIPE
                                  ├── EXEC  ls
                                  └── EXEC  grep READ

echo hello > f ; cat < f          LIST
                                  ├── REDIR  > f   (channel 1)
                                  │   └── EXEC  echo hello
                                  └── REDIR  < f   (channel 0)
                                      └── EXEC  cat

(echo a; echo b) | wc             PIPE
                                  ├── LIST
                                  │   ├── EXEC  echo a
                                  │   └── EXEC  echo b
                                  └── EXEC  wc

echo bg &                         BACK
                                  └── EXEC  echo bg

mkdir d ; cd d ; ls ..            LIST
                                  ├── EXEC  mkdir d
                                  └── LIST
                                      ├── EXEC  cd d      ← a plain command, so exec("cd") is tried
                                      └── EXEC  ls ..
```

`runcmd` then walks the tree from the top, with the cases of 6.5. The last tree shows once more why `cd` failed inside a list: for the parser, `cd d` is a plain command like any other.

The three redirections in `parseredirs`:

| Typed | Line | File is opened | Channel |
|---|---|---|---|
| `< file` | 392 | for reading | 0 |
| `> file` | 395 | for writing, created if missing, emptied first | 1 |
| `>> file` | 398 | for writing, created if missing, not emptied | 1 |

**`nulterminate` (lines 455-499).** The words found by the parser are not copied anywhere. They are places inside the typed line. In C a text must end with a zero byte, so this function walks the tree and writes a zero after every word, right into the line. After it, each word is a proper text that `exec` can pass on.

### 6.7 What is not in the file

![Figure 4.4 – No code for * or $](screenshots/fig-4-4.png)

These three searches are mine, added to answer the last question of the sheet:

```bash
grep -n "symbols\[\]\|whitespace\[\]" user/sh.c
grep -n '\$' user/sh.c
grep -c "glob\|getenv\|HOME" user/sh.c
```

| No. | What it searches | Result |
|---|---|---|
| 1 | the two lists of 6.6. `\[` and `\]` are plain square brackets for `grep`, and the backslash and bar mean "or" | lines 268 and 269 |
| 2 | a dollar sign. `\$` is a plain dollar: without the backslash, `$` means "end of the line" for `grep`. The single quotes keep bash away from it | only line 137, the prompt |
| 3 | `-c` counts the lines that contain any of the three words `glob`, `getenv`, `HOME` | 0 |

Line 269 is the complete list of special characters: `< | > & ; ( )`. There is no `*`, no `$`, no quote. So the xv6 shell has no globbing, no variables and no quotes at all, and that is why `ls *`, `echo $HOME` and `echo "a b"` reached the programs exactly as they were typed.

## 7. The xv6 tree at the end

![Figure 5.1 – The tree at the end of the lab](screenshots/fig-5-1.png)

| Command | What it does | Result |
|---|---|---|
| `git add -A && git commit -q -m "Lab 4: sleep calls pause(), args program"` | save all the changes as one commit | |
| `git status -sb` | the state of the folder | `## lab` and nothing else: everything is saved |
| `git log --oneline -3` | the last three commits, newest first | my commit `5186256` sits on top of `06aad25`, the course revision |
| `git diff --stat 06aad25` | what is different from the original xv6, counted per file | `Makefile` 2 lines, `user/args.c` 11, `user/sleep.c` 13: 26 lines added |

Those 26 lines are the whole change to xv6 in this lab: two lines in `UPROGS` and two small programs. The three files were then copied into this folder, next to `args_linux.c`.

## 8. bash and the xv6 shell side by side

| Feature | bash | xv6 shell | Where in `sh.c` |
|---|---|---|---|
| run a program with arguments | yes | yes | `case EXEC`, line 75 |
| `<`, `>`, `>>` | yes | yes | `parseredirs` line 381, `case REDIR` line 83 |
| pipe `\|` | yes | yes | `parsepipe` line 368, `case PIPE` line 101 |
| `;` | yes | yes | `parseline` line 360, `case LIST` line 93 |
| grouping `( )` | yes | yes | `parseblock` line 406 |
| background `&` | yes, with job numbers | yes, with no list of jobs | `case BACK` line 125 |
| `cd` | built in, works anywhere on the line | built in, only at the start of a line | `main` line 166 |
| `*`, `?` | yes | no | not in `symbols`, line 269 |
| variables `$HOME` | yes | no | no code |
| quotes | yes | no | no code |
| `PATH` | yes | no: the name goes to `exec` as typed | line 79 |
| history, Tab, Ctrl+R | yes | no | `getcmd` reads plain characters, line 139 |

What this lab shows about the lab before it: everything Lab 3 was about (patterns, variables, quotes, `PATH`, history, completion, jobs) is not part of the operating system. It is bash. A shell needs very little: read a line, cut it into words, `fork`, connect the files and pipes, `exec`. xv6 does all of that in 499 lines.

## 9. Test yourself

Try to answer before opening each one.

<details>
<summary>1. Why did the old <code>sleep.c</code> stop building?</summary>

The system call was renamed from `sleep()` to `pause()` in August 2025. `user/user.h` no longer announces `sleep`, so the compiler stops with "implicit declaration of function" (3.3).
</details>

<details>
<summary>2. What are <code>argc</code> and <code>argv</code> for <code>sleep 20</code>?</summary>

`argc` is 2. `argv[0]` is `sleep` and `argv[1]` is the text `20`. `atoi` turns that text into the number 20 (3.5).
</details>

<details>
<summary>3. Why does <code>sleep</code> print its usage message on channel 2 and end with <code>exit(1)</code>?</summary>

Channel 2 is the error channel, so the message does not mix with normal output. Exit code 1 tells whoever started the program that it failed. It is the same idea as `>&2` and `exit 1` in `report.sh` of Lab 3 (3.5).
</details>

<details>
<summary>4. What does the line <code>$U/_sleep\</code> in the Makefile do?</summary>

It adds the program to the list `UPROGS`. Only the programs in that list are built and copied onto the xv6 disk (3.6).
</details>

<details>
<summary>5. Name the five kinds of command of the xv6 shell and the character that makes each.</summary>

`EXEC` (no special character), `REDIR` (`<`, `>`, `>>`), `PIPE` (`|`), `LIST` (`;`), `BACK` (`&`) (6.1).
</details>

<details>
<summary>6. In xv6, why does <code>ls</code> fail after <code>cd d</code>, and how can it still be run?</summary>

The xv6 shell has no `PATH`. `exec` looks for a file `ls` in the current folder, and the programs are in `/`. Give the place: `/ls` or `../ls` (4.9).
</details>

<details>
<summary>7. Why does <code>mkdir d ; cd d ; ls ..</code> print <code>exec cd failed</code>?</summary>

The shell handles `cd` itself only when the line starts with `cd ` (line 166). This line starts with `m`, so `cd d` is run like a program, and there is no program named `cd` (6.3).
</details>

<details>
<summary>8. Why can <code>cd</code> not be a separate program?</summary>

Each process has its own current folder. A child that changes folder changes only its own, then ends. The shell must call `chdir` itself (6.3).
</details>

<details>
<summary>9. <code>args *.c "x y"</code>: how many arguments in bash, how many in xv6?</summary>

In bash `argc` is 5: the name, three file names, and `x y` as one argument. In xv6 `argc` is 4: the name, `*.c`, `"x` and `y"` (5.3).
</details>

<details>
<summary>10. So who replaces <code>*.c</code> and removes the quotes: the program, the kernel or the shell?</summary>

The shell. The program is the same on both sides, and the kernel copies the arguments as they are (5.4).
</details>

<details>
<summary>11. How does the xv6 shell send the output of a command into a file?</summary>

In the child, before `exec`: it closes channel 1 and opens the file. `open` gives the smallest free number, which is 1. Then the command runs and writes to channel 1 as always (6.5).
</details>

<details>
<summary>12. Is there any code in <code>sh.c</code> for <code>*</code> or <code>$</code>?</summary>

No. The list of special characters on line 269 is `< | > & ; ( )`. The only `$` in the file is the prompt on line 137 (6.7).
</details>

<details>
<summary>13. What happens in xv6 when I press Tab in the middle of a word?</summary>

Tab is in the list of white space (line 268), so it cuts the word in two. `ec` + Tab + `ho two` becomes the command `ec`, and `exec ec failed` (4.10).
</details>

<details>
<summary>14. Which feature of bash would be the easiest to add to the xv6 shell?</summary>

A `PATH` with one folder: when `exec` fails for a name without `/`, try again with `/` in front of the name. It is a few lines after line 79 (6.5).
</details>
