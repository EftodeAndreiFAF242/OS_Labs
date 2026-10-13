# Lab 6 – Full explanations

Course: Operating Systems, FCIM / FAF, UTM, 2026-2027. Lab 6: Inside the xv6 File System.

[`README.md`](README.md) tells what was done in the lab. This file goes slower. It explains every line of the two programs I wrote (`mystat.c`, `find.c`), my lines in the `Makefile`, every command typed in the terminal, and the kernel code behind what the programs show: how `stat`, `link` and `unlink` work inside xv6, where the blocks of a file are kept, how the disk is laid out, and why the limits are what they are. It is written for someone who has never used Linux or xv6. The figure numbers are the same as in the README and in the Word report.

Lab 5 ([`../lab5/full_explanations.md`](../lab5/full_explanations.md)) did the same experiments on Linux from the outside. This lab opens the box.

## Contents

1. [Words you need first](#1-words-you-need-first)
2. [Before the lab](#2-before-the-lab)
3. [Part 1: what xv6 shows you](#3-part-1-what-xv6-shows-you)
4. [Part 2: mystat and hard links](#4-part-2-mystat-and-hard-links)
5. [Part 3: the limits](#5-part-3-the-limits)
6. [Part 4: find](#6-part-4-find)
7. [The xv6 tree at the end](#7-the-xv6-tree-at-the-end)
8. [xv6 and ext4 side by side](#8-xv6-and-ext4-side-by-side)
9. [Test yourself](#9-test-yourself)

## 1. Words you need first

| Word | Meaning |
|---|---|
| kernel | the core of the operating system. It owns the disk; programs ask it for files through system calls |
| system call | a request to the kernel: `open`, `read`, `write`, `close`, `fstat`, `link`, `unlink`, `mkdir`, `chdir`... |
| user program | everything outside the kernel: `ls`, `cat`, the shell, `mystat`, `find` |
| QEMU | a program that pretends to be a RISC-V computer, so xv6 has something to run on |
| `fs.img` | a 2,048,000-byte Linux file that is the entire disk of the pretend computer |
| `mkfs` | a Linux program that is part of xv6 (`mkfs/mkfs.c`). It builds `fs.img`: an empty file system plus `README` and the user programs |
| block | the disk is read and written in blocks of 1024 bytes (`BSIZE`) |
| inode | the record of one file: type, link count, size, and the numbers of the blocks that hold its content |
| directory entry (`struct dirent`) | 16 bytes: a 2-byte inode number and a 14-byte name |
| link count (`nlink`) | how many directory entries point to an inode |
| `UPROGS` | the list of programs in the Makefile that are built and put on the disk |

There are four computers inside each other:

```
Windows 11                        the real laptop
└── VirtualBox                    a pretend PC
    └── Ubuntu Linux, with bash       prompt:  vboxuser@os:~/xv6-riscv$
        └── QEMU                      a pretend RISC-V computer, its disk is the file fs.img
            └── xv6, with its shell   prompt:  $
```

To leave xv6 and come back to bash: press Ctrl-A, let go, then press X.

### The layers of the xv6 file system

The xv6 book describes the file system as seven layers. Each uses only the one below it:

```
file descriptors    sysfile.c, file.c   open, read, write, close, fstat, dup
path names          fs.c (namei)        "/a/b/r2" → walk the directories
directories         fs.c (dirlookup)    a directory is a file of struct dirent
inodes              fs.c (iget, iput)   one file: type, nlink, size, block list
logging             log.c               group the writes of one call, so a crash leaves no half change
buffer cache        bio.c               keep recently used blocks in memory
disk                virtio_disk.c       read and write 1024-byte blocks of fs.img
```

## 2. Before the lab

```bash
cd ~/Downloads
bash os2026-setup.sh --check     # toolchain, QEMU and ~/xv6-riscv are ready
```

![Figure 0.1 – bash os2026-setup.sh --check: the tools are there, xv6 builds and boots](screenshots/fig-0-1.png)

With `--check` the course script installs nothing (that is why it starts at step `2/4`). It checks the tools, finds `~/xv6-riscv` and leaves it as it is, builds xv6, boots it once with a hidden `ls`, and prints **All set.**

```bash
cd ~/xv6-riscv
git branch --show-current ; git log --oneline -2 ; git status -s
grep -n -A4 "^UPROGS" Makefile | cat -A
```

![Figure 0.2 – The UPROGS list before this lab, with the two lines from Lab 4](screenshots/fig-0-2.png)

| Piece | Meaning |
|---|---|
| `grep -n -A4 "^UPROGS" Makefile` | find the line that starts (`^`) with `UPROGS`, print it with its number (`-n`) and the 4 lines after it (`-A4`) |
| `\| cat -A` | show invisible characters: `^I` is a TAB, `$` is the end of the line |

`make` requires recipe and list lines to be exact: the program lines must start with a TAB, and the backslash at the end must be the very last character (it means "the list continues on the next line"). `cat -A` proves it. The tree is on branch `lab`, on my Lab 4 commit, with nothing uncommitted.

## 3. Part 1: what xv6 shows you

### 3.1 A fresh disk

```bash
make clean ; make qemu
```

![Figure 1.1 – make clean ; make qemu: a fresh disk is built and xv6 boots](screenshots/fig-1-1.png)

`make clean` deletes every built file, including `fs.img`. `make qemu` then:

1. compiles every user program (`riscv64-linux-gnu-gcc ... -c -o user/sync.o user/sync.c`) and links it (`riscv64-linux-gnu-ld ... -o user/_sync`);
2. runs `mkfs/mkfs fs.img README user/_cat user/_echo ...`, which makes the disk and prints its layout (the `nmeta` line, section 5.4);
3. starts QEMU with the kernel and `fs.img` as its disk. xv6 boots: `hart 1 starting`, `hart 2 starting` (three processors), `init: starting sh`, and the prompt `$`.

The programs are called `_cat`, `_ls` on Linux so that nobody runs them by mistake on Linux; `mkfs` drops the `_` when it writes them onto the xv6 disk.

### 3.2 The four columns of `ls`

```
$ ls
.              1 1 1024
..             1 1 1024
README         2 2 2441
cat            2 3 36728
...
console        3 25 0
```

![Figure 1.2 – ls, mkdir a and ls a in xv6](screenshots/fig-1-2.png)

The line in `user/ls.c` that prints them is:

```c
printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, (int)st.size);
```

So the columns are **name** (padded with spaces to 14 characters by `fmtname`), **type**, **inode number**, **size**. The inode numbers follow the order in which things were created:

- `mkfs` creates the root first: inode 1.
- Then it copies `README` and the programs in the order of the command line: `README` 2, `cat` 3, `echo` 4, `sleep` 5, `args` 6 (my Lab 4 programs), and so on up to `sync` 24.
- `console` is not made by `mkfs`. At the first boot, `init` (`user/init.c`, line 20) cannot open `console`, so it creates it with `mknod("console", CONSOLE, 0)`, and it gets the next free number: 25. (Once `mystat` is also on the disk, everything after it moves up by one, and `console` becomes 26.)
- `mkdir a` takes the next number: 26.

### 3.3 The types

```c
#define T_DIR    1 // Directory
#define T_FILE   2 // File
#define T_DEVICE 3 // Device
```

![Figure 1.3 – The types in kernel/stat.h, the root inode, a directory entry, and how mkfs makes the root](screenshots/fig-1-3.png)

`struct stat` is what a program receives from `stat()`/`fstat()`: the disk device (`dev`), the inode number (`ino`), the type, the link count (`nlink`) and the size. Five fields; Linux's `stat` (Lab 5) has about fifteen.

A device (type 3) has no content on the disk. Its inode holds a `major` number (`CONSOLE` is 1), and reads and writes go to that driver: typing on the keyboard, printing on the screen. That is why `console` has size 0.

### 3.4 The root, and why `.` and `..` are the same there

```c
#define ROOTINO 1 // root i-number
```

`mkfs` (lines 121 to 132) allocates the first inode as a directory, checks that it got number 1, and writes two entries into it:

```c
rootino = ialloc(T_DIR);
assert(rootino == ROOTINO);

bzero(&de, sizeof(de));
de.inum = xshort(rootino);     // "." → the root itself
strcpy(de.name, ".");
iappend(rootino, &de, sizeof(de));

bzero(&de, sizeof(de));
de.inum = xshort(rootino);     // ".." → also the root
strcpy(de.name, "..");
iappend(rootino, &de, sizeof(de));
```

Every other directory's `..` points to its parent. The root has no parent, so its `..` points to itself; `cd ..` in `/` stays in `/`. (`xshort` only puts the number in the byte order of the disk.)

### 3.5 Directory entries and directory sizes

```c
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ] __attribute__((nonstring));
};
```

A directory is an ordinary file whose content is a row of these 16-byte records (2 bytes `ushort` + 14 bytes name). `nonstring` tells the compiler that the name is *not* always ended by a zero byte: a 14-character name fills the whole field.

```
directory a (inode 26), 32 bytes:
offset 0   [ inum 26 ][ ".\0\0\0\0\0\0\0\0\0\0\0\0\0" ]
offset 16  [ inum 1  ][ "..\0\0\0\0\0\0\0\0\0\0\0\0" ]
```

So `a` is 32 bytes = 2 entries. The root shows 1024 bytes although it has only 26 entries (416 bytes): at the end, `mkfs` rounds the root's size up to a whole block ("fix size of root inode dir", lines 170 to 174). The rest of the block is zeros, that is entries with `inum == 0`: **free slots**. `ls` skips them (`if (de.inum == 0) continue;`), and new entries made later (`a`, `r2`) are written into the first free slot, so the root stays 1024 bytes.

### 3.6 Answers

1. 1 = directory, 2 = regular file, 3 = device.
2. Inode 1. It has no parent, so `mkfs` gives `..` the root's own number.
3. 16 bytes per entry; `a` holds `.` (26) and `..` (1).

## 4. Part 2: mystat and hard links

### 4.1 mystat.c, every line

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
```

`types.h` defines the short type names (`uint`, `ushort`, `uint64`); `stat.h` the `T_` constants and `struct stat`; `user.h` the system calls and library functions (`stat`, `printf`, `fprintf`, `exit`). The order matters: `user.h` uses `struct stat`, so `stat.h` must come before it.

```c
char *
typename(int t)
{
  switch(t){
  case T_DIR:
    return "directory";
  case T_FILE:
    return "file";
  case T_DEVICE:
    return "device";
  }
  return "?";
}
```

A function that returns a pointer to a text (`char *`). The `switch` jumps to the `case` that equals `t` and returns the word. The texts are constants stored in the program, so returning their address is safe. For any other number it falls out of the `switch` and returns `"?"`, as the skeleton did.

```c
int
main(int argc, char *argv[])
{
  struct stat st;
```

`argc` is the number of words on the command line, `argv` the words: for `mystat README . console`, `argc` is 4 and `argv[1]` to `argv[3]` are the three paths. `st` is the structure that `stat` will fill.

```c
  if(argc < 2){
    fprintf(2, "usage: mystat path...\n");
    exit(1);
  }
```

My addition: with no path, explain how to use it, on file descriptor 2 (errors), and end with status 1 (failure).

```c
  for(int i = 1; i < argc; i++){
    if(stat(argv[i], &st) < 0){
      fprintf(2, "mystat: cannot stat %s\n", argv[i]);
      continue;
    }
```

For each path: `stat(path, &st)` fills `st`, or returns -1 if the path does not exist. Then: print the error on fd 2 and `continue`, that is go to the next `i` without printing the normal line. One bad path does not stop the others (Figure 2.6: `mystat nosuch r2`).

```c
    printf("%s: %s, inode %d, links %d, size %d bytes\n",
           argv[i], typename(st.type), st.ino, st.nlink, (int)st.size);
  }
  exit(0);
}
```

Print the line in the format of the sheet. `%s` takes a text, `%d` a number. `st.size` is a 64-bit `uint64`; `(int)` makes it a normal `int` for `%d`, as `ls.c` does. Then `exit(0)`: success. The current xv6 would also accept a `return` from `main` (a small wrapper, `start` in `user/ulib.c`, calls `exit` with the returned value), but all xv6 programs end with `exit`, so mine does too.

### 4.2 What `stat()` really does

`stat` is not a system call in xv6. It is a small library function in `user/ulib.c`:

```c
int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if (fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}
```

It opens the file, asks the kernel about the open file with the real system call `fstat`, and closes it. In the kernel, `sys_fstat` → `filestat` (`kernel/file.c`) → `stati` (`kernel/fs.c`), which copies five fields from the in-memory inode:

```c
st->dev = ip->dev;
st->ino = ip->inum;
st->type = ip->type;
st->nlink = ip->nlink;
st->size = ip->size;
```

and `copyout` copies the structure into the program's memory, into my `st`.

### 4.3 The build

```bash
nano user/mystat.c
sed -i 's|^\t$U/_args\\$|&\n\t$U/_mystat\\|' Makefile
grep -n -B1 -A1 "_mystat" Makefile | cat -A
git status -s
```

![Figure 2.1 – Writing user/mystat.c in nano](screenshots/fig-2-1.png)

![Figure 2.2 – The line $U/_mystat\ added to UPROGS](screenshots/fig-2-2.png)

The `sed` command, piece by piece:

| Piece | Meaning |
|---|---|
| `sed -i` | edit the file in place |
| `s\|A\|B\|` | substitute A with B. `\|` is used as the separator instead of `/`, because the text contains `/` |
| `^\t$U/_args\\$` | A: a line that is exactly TAB, `$U/_args`, a backslash (`\\` = one real backslash), end of line (`$`) |
| `&` | in B: the whole matched text, so the `args` line stays |
| `\n\t$U/_mystat\\` | then a new line: TAB, `$U/_mystat`, a backslash |

The single quotes around the whole command stop bash from touching `$U`, `\t` and `\\`; `sed` sees them as written.

![Figure 2.3 – make qemu compiles mystat and puts it on a new disk](screenshots/fig-2-3.png)

`make` compares file times and rebuilds only what is older than what it depends on: `user/mystat.c` is new, so `user/mystat.o` and `user/_mystat` are built. `fs.img` depends on all of `UPROGS`, so it is rebuilt too: a fresh disk.

### 4.4 Hard links, as the kernel does them

```
$ mystat README . console
$ ln README r2
$ mystat README r2
$ rm README
$ mystat r2
$ cat r2 | wc
```

![Figure 2.4 – Hard links at work: ln, then rm of the original name](screenshots/fig-2-4.png)

```
                    entries in /                 inode 2
start               README → 2                   nlink 1, size 2441, blocks [...]
ln README r2        README → 2,  r2 → 2          nlink 2
rm README           (free slot), r2 → 2          nlink 1, same size, same blocks
```

`ln` (`user/ln.c`) calls the system call `link(old, new)`. `sys_link` in `kernel/sysfile.c` (lines 124 to 174):

1. `namei(old)` finds the inode of `README`;
2. refuses if it is a directory (no hard links to directories, so the tree cannot get loops) or if `nlink` would pass `NLINK_MAX`;
3. `ip->nlink++` and `iupdate(ip)` (write the inode back to the disk);
4. `nameiparent(new, name)` finds the directory where `r2` goes (the root) and the last name, `r2`;
5. `dirlink(dp, name, ip->inum)` writes a new entry "r2 → 2" into the first free slot of that directory.

All of it is inside `begin_op()` ... `end_op()`, one transaction of the log, so a crash in the middle cannot leave a link count that does not match the entries.

`rm` (`user/rm.c`) calls `unlink(path)`. `sys_unlink` (lines 202 to 256):

1. `nameiparent` finds the root and the name `README`; `.` and `..` cannot be removed;
2. `dirlookup` finds the entry and its position (`off`);
3. a non-empty directory cannot be removed;
4. `memset(&de, 0, ...)` and `writei(dp, ..., off, ...)` overwrite the entry with zeros: the slot becomes free (`inum` 0);
5. if it was a directory, the parent loses one link (`dp->nlink--`, line 239: the child's `..` is gone);
6. **line 244: `ip->nlink--;`**, then `iupdate(ip)` and `iunlockput(ip)`.

![Figure 2.7 – The end of sys_unlink in kernel/sysfile.c](screenshots/fig-2-7.png)

`cat r2 | wc` prints `48 336 2441`: 48 lines, 336 words, 2441 bytes. The content is exactly what `README` had.

![Figure 2.5 – The root after rm README: only r2 is left, with inode 2](screenshots/fig-2-5.png)

### 4.5 When the blocks are really freed

`sys_unlink` never frees blocks. It ends with `iunlockput(ip)`, which calls `iput(ip)`:

![Figure 2.8 – iput in kernel/fs.c: where the data blocks are freed](screenshots/fig-2-8.png)

```c
int last = (ip->ref == 1 && ip->valid && ip->nlink == 0);
...
if (last) {
  ...
  itrunc(ip); // free the data blocks (type stays nonzero on disk)
  ...
}
ip->ref--;
...
if (last)
  ifree(dev, inum); // now clear type on disk: inum becomes allocatable
```

An inode in memory has **two** counters:

- `nlink`, stored on the disk: how many directory entries point to it;
- `ref`, only in memory: how many pointers to this in-memory inode exist (open files, current directories of processes, kernel code that is using it right now).

`iput` drops one `ref`. Only if this was the last reference **and** there are no links left does it call `itrunc`, which goes through `addrs[]` and gives every data block back to the free bitmap (`bfree`), and then `ifree`, which sets the inode's type to 0 so the number can be used again.

So in Part 2 nothing was freed: `nlink` went from 2 to 1. If `r2` were removed too while no one had it open, `nlink` would be 0 and `ref` 1, and the blocks would be freed at once. If some process still had `r2` open, its `ref` would keep the blocks until it closed the file: the same rule as Linux in Lab 5, stretch (c).

What if the computer crashes after `unlink` but before the last `close`? On the disk there is an inode with a type but no links. At boot, `ireclaim()` (fs.c, line 389) scans all inodes, finds such "orphans", and frees them. (The programs `forphan` and `dorphan` on the disk test exactly that.)

### 4.6 struct dinode and struct inode

![Figure 2.9 – The on-disk inode, struct dinode](screenshots/fig-2-9.png)

```c
struct dinode {
  short type;              // File type
  short major;             // Major device number (T_DEVICE only)
  short minor;             // Minor device number (T_DEVICE only)
  short nlink;             // Number of links to inode in file system
  uint size;               // Size of file (bytes)
  uint addrs[NDIRECT + 1]; // Data block addresses
};
```

Size: 4 × 2 bytes + 4 bytes + 13 × 4 bytes = 8 + 4 + 52 = **64 bytes**, so `IPB` = 1024 / 64 = 16 inodes per block.

What it does **not** have, compared with ext4 (Lab 5): no owner or group (xv6 has no users), no permission bits (anyone may do anything), no times, and no type for symbolic links. A free inode is one with `type == 0`.

The in-memory copy, `struct inode` in `kernel/file.h`, adds what is needed while it is used: `dev` and `inum` (where it came from), `ref` (the reference count), a `lock`, and `valid` (has it been read from the disk yet?).

### 4.7 Directory links in xv6

![Figure 2.6 – Errors go to fd 2; links of xv6 directories](screenshots/fig-2-6.png)

| | `.` (root) | `d` | `d/x` |
|---|---|---|---|
| fresh disk | 1 link | | |
| after `mkdir d` | 2 | 1 link, 32 bytes | |
| after `mkdir d/x` | 2 | 2 links, 48 bytes | 1 link, 32 bytes |

`create()` in `kernel/sysfile.c` makes the new directory with `nlink = 1` (its name in the parent), writes `.` and `..` into it, and adds 1 to the parent "for `..`". There is a comment: `// No ip->nlink++ for ".": avoid cyclic ref count.` So in xv6 a directory's count is 1 + its subdirectories, while Linux (Lab 5) counts `.` too and gives 2 + its subdirectories. The root starts at 1 because `mkfs` sets it so.

### 4.8 Answers

1. After `ln`: `README` and `r2` both inode 2, links 2, 2441 bytes. After `rm README`: `r2` inode 2, links 1, 2441 bytes, and `wc` sees all the content. Changed: the link count and the directory entries. Unchanged: the inode, the size, the data.
2. `ip->nlink--;` at line 244 of `kernel/sysfile.c`. The data blocks are freed by `itrunc` inside `iput()` (`kernel/fs.c`, line 365) when the last reference is dropped and `nlink` is 0.
3. `type`, `major`, `minor`, `nlink`, `size`, `addrs[NDIRECT + 1]`.

## 5. Part 3: the limits

### 5.1 The constants

```bash
grep -n "define BSIZE\|define NDIRECT\|define NINDIRECT\|define DIRSIZ" kernel/fs.h
make clean ; make fs.img 2>&1 | grep nmeta
```

![Figure 3.1 – The constants of kernel/fs.h and the nmeta line printed by mkfs](screenshots/fig-3-1.png)

`\|` inside the `grep` pattern means "or". `2>&1` sends stream 2 (errors) to the same place as stream 1, so that the pipe gets both and `grep nmeta` can find the line whichever stream it is printed on. The `rm -f ...` lines above it are just `make clean` showing what it deletes.

| Constant | Value | Meaning |
|---|---|---|
| `BSIZE` | 1024 | bytes per block |
| `NDIRECT` | 12 | block numbers kept directly in the inode |
| `NINDIRECT` | `BSIZE / sizeof(uint)` = 256 | block numbers that fit in one indirect block |
| `DIRSIZ` | 14 | bytes for a name in a directory entry |
| `MAXFILE` | `NDIRECT + NINDIRECT` = 268 | the most blocks a file can have |

### 5.2 How a file finds its blocks

```
inode.addrs[0]  ─► data block 0          (bytes      0 ..   1023)
inode.addrs[1]  ─► data block 1          (bytes   1024 ..   2047)
...
inode.addrs[11] ─► data block 11         (bytes  11264 ..  12287)
inode.addrs[12] ─► indirect block ┬ [0]   ─► data block 12   (bytes 12288 .. 13311)
                                  ├ [1]   ─► data block 13
                                  ├ ...
                                  └ [255] ─► data block 267  (bytes 273408 .. 274431)
```

`bmap(ip, bn)` in `kernel/fs.c` turns "block number `bn` of this file" into "block number on the disk": if `bn < 12`, it is `addrs[bn]`; otherwise it reads the indirect block `addrs[12]` and takes entry `bn - 12`. In both cases, if the block does not exist yet, it allocates one (`balloc`). A file with 2441 bytes, like `README`, uses `addrs[0]` to `addrs[2]` (3 blocks) and no indirect block.

### 5.3 The largest file

```bash
echo $(( (12 + 1024/4) * 1024 ))
```

![Figure 3.2 – The largest file, computed in bash, and where the disk layout comes from](screenshots/fig-3-2.png)

`$(( ... ))` is arithmetic in bash. The calculation:

```
direct blocks                12
indirect: 1024 bytes / 4 =  256 block numbers
total                       268 blocks
268 × 1024 bytes          = 274,432 bytes = 268 KiB
```

What happens if a program tries to write past it: `writei` (`kernel/fs.c`, line 551) checks `if (off + n > MAXFILE * BSIZE) return -1;`, so the `write` call fails. Real xv6 courses later add a "doubly-indirect" block (a block of indirect blocks) to raise the limit to 65,803 blocks; ext4 uses extents instead.

### 5.4 The disk layout

```
nmeta 47 (boot, super, log blocks 31, inode blocks 13, bitmap blocks 1) blocks 1953 total 2000
```

Where every number comes from (`kernel/param.h` and `mkfs/mkfs.c`):

| Part | Blocks | Computed as |
|---|---|---|
| boot block | 1 | block 0, not used by xv6 |
| superblock | 1 | block 1: the description of the disk (`struct superblock`: size, number of blocks, inodes, where the log, inodes and bitmap start) |
| log | 31 | `nlog = LOGBLOCKS + 1`, with `LOGBLOCKS = MAXOPBLOCKS * 3 = 30`, plus 1 header block |
| inodes | 13 | `NINODES / IPB + 1` = 200 / 16 + 1 = 12 + 1 (integer division) |
| bitmap | 1 | `FSSIZE / BPB + 1` = 2000 / 8192 + 1 = 0 + 1; one bit per block, 8192 bits per block |
| **metadata** (`nmeta`) | **47** | 2 + 31 + 13 + 1 |
| data | 1953 | `FSSIZE - nmeta` = 2000 − 47 |
| **total** | **2000** | `FSSIZE` |

```
block:  0      1       2 ......... 32   33 ....... 45   46       47 .............. 1999
        boot   super   log (31)         inodes (13)     bitmap   data blocks (1953)
```

The other `mkfs` lines agree: `balloc: write bitmap block at sector 46`, and `balloc: first 1055 blocks have been allocated` (47 metadata blocks + about 1008 blocks for `README`, the programs and the root directory). The whole disk is 2000 × 1024 = 2,048,000 bytes, about 2 MB.

The **log** is xv6's journal. A system call like `unlink` changes several blocks (the directory, the inode, maybe the bitmap). Between `begin_op()` and `end_op()` the changed blocks are first written to the log area, then a header saying "committed", and only then to their real places. After a crash, boot either replays a committed log or ignores an uncommitted one, so the disk never shows half of an operation. `MAXOPBLOCKS` = 10 is the most blocks one operation may write, and the log can hold three such operations at once.

### 5.5 Names longer than 14 characters

```
$ mkdir averyveryverylongname
$ ls
$ cd averyveryverylongname
$ /ls
$ cd ..
$ mkdir averyveryverylongXYZ
$ mystat averyveryveryl averyveryverylongname
```

![Figure 3.3 – mkdir averyveryverylongname and ls](screenshots/fig-3-3.png)

![Figure 3.4 – The long name still works, because every lookup is cut to 14 characters too](screenshots/fig-3-4.png)

Every path goes through `skipelem()` in `kernel/fs.c`, which takes the next name out of the path:

```c
len = path - s;
if (len >= DIRSIZ)
  memmove(name, s, DIRSIZ);   // copy only the first 14 characters
else {
  memmove(name, s, len);
  name[len] = 0;
}
```

and names are compared with `namecmp`, which is `strncmp(s, t, DIRSIZ)`: only the first 14 characters count. So:

- `mkdir averyveryverylongname` creates an entry with the 14 characters `averyveryveryl` (no ending zero, the field is full), and `ls` shows that.
- `cd averyveryverylongname` and `mystat averyveryverylongname` work: the name is cut the same way before it is looked up, and finds inode 27.
- `mkdir averyveryverylongXYZ` fails: cut to 14 characters it is `averyveryveryl` again, which exists (`dirlink` refuses a name that is already present).
- Inside the directory, `ls` alone would fail (`exec ls failed`): the xv6 shell has no `PATH`, as Lab 4 found. `/ls` gives the full path.

Linux (ext4) allows 255 bytes per name, and reports an error for longer names instead of cutting them.

### 5.6 Answers

1. `averyveryveryl`: 14 characters, because of `DIRSIZ`.
2. (12 + 1024 / 4) × 1024 = 268 × 1024 = 274,432 bytes.
3. 2000 blocks: 31 log, 13 inode, 1953 data (and boot, super, 1 bitmap: 47 metadata blocks).

## 6. Part 4: find

### 6.1 find.c, every line

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
```

The same as `ls.c`, plus `param.h` for `MAXPATH`. `fs.h` gives `struct dirent` and `DIRSIZ`; `fcntl.h` gives `O_RDONLY`.

```c
int showinum; // set by the optional third argument -n
```

A global variable for the stretch option. Global variables in C start at 0, so by default the inode numbers are not printed.

```c
void
find(char *dir, char *name)
{
  char buf[MAXPATH], *p; // the kernel refuses longer paths anyway
  int fd;
  struct dirent de;
  struct stat st;
```

`buf` will hold the full path of each entry, `p` points inside it, `fd` is the open directory, `de` one directory entry, `st` the information about one entry. These are local variables: every call of `find` (every level of the recursion) gets its own copy on the stack.

```c
  if ((fd = open(dir, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", dir);
    return;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", dir);
    close(fd);
    return;
  }
  if (st.type != T_DIR) {
    fprintf(2, "find: %s is not a directory\n", dir);
    close(fd);
    return;
  }
```

Open the directory. In xv6 a directory can be opened and read like any file (only for reading). Then check what it is; if it is not a directory, say so (Figure 4.7, `find README r2`). Every early exit after `open` closes `fd` first.

```c
  if (strlen(dir) + 1 + DIRSIZ + 1 > sizeof buf) {
    fprintf(2, "find: path too long\n");
    close(fd);
    return;
  }
  strcpy(buf, dir);
  p = buf + strlen(buf);
  *p++ = '/';
```

The longest path built below is the directory, a `/`, a 14-character name and the ending zero. If that does not fit in `buf`, stop. Otherwise copy the directory into `buf`, point `p` at its end, write a `/` there and move `p` past it:

```
buf:  . / ? ? ? ...        for dir = "."
        ↑
        p   (each entry's name will be written here)
```

```c
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0) // free slot
      continue;
```

Read the directory 16 bytes at a time, one entry per `read`, until `read` returns less (the end). Entries with inode 0 are free slots (section 3.5): skip them.

```c
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
```

Copy the 14-byte name field after the `/` and put a zero after it, because a 14-character name has no zero of its own. Now `buf` is a full path, like `./a`, and `p` alone is the name, `a`.

```c
    if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
      continue;
```

Never go into `.` or `..`. Going into `.` would search the same directory again, forever; going into `..` would climb back up and search everything again, forever. `strcmp` returns 0 when two texts are equal.

```c
    if (stat(buf, &st) < 0) {
      fprintf(2, "find: cannot stat %s\n", buf);
      continue;
    }
```

Get the type and inode number of this entry (by its full path).

```c
    if (strcmp(p, name) == 0) {
      if (showinum)
        printf("%s %d\n", buf, st.ino);
      else
        printf("%s\n", buf);
    }
```

If the last part of the path is the name we look for, print the path, and with `-n` its inode number too. This is done for files **and** directories, which is why `find a b` prints `a/b`.

```c
    if (st.type == T_DIR)
      find(buf, name);
  }
  close(fd);
}
```

If the entry is a directory, search it the same way: `find` calls itself with the longer path. When that call returns, the loop continues with the next entry of this directory. After the last entry, close the directory.

```c
int
main(int argc, char *argv[])
{
  if (argc < 3 || argc > 4 || (argc == 4 && strcmp(argv[3], "-n") != 0)) {
    fprintf(2, "usage: find dir name [-n]\n");
    exit(1);
  }
  showinum = (argc == 4);
  find(argv[1], argv[2]);
  exit(0);
}
```

Accept exactly `find dir name` or `find dir name -n`; anything else prints the usage line. `showinum` becomes 1 when there is a fourth word (and it was checked to be `-n`).

### 6.2 What `ls.c` already gave, and what is new

| From `ls.c` | New in `find.c` |
|---|---|
| open the directory, `fstat` it | refuse anything that is not a directory |
| the "path too long" check | a buffer of `MAXPATH` (128) instead of 512 |
| copy `dir`, add `/`, read `struct dirent` in a loop, skip `inum == 0` | skip `.` and `..` |
| copy the name, add the zero, `stat` the full path | compare the name; print the path |
| | recurse into subdirectories |
| `close(fd)` | `close(fd)` on every path out of the function |

### 6.3 Following `find . r2` by hand

The test tree, after `mkdir a`, `mkdir a/b`, `echo x > a/b/r2`, `echo y > a/r2`, `ln cat r2`:

```
/ (1)
├── README, cat (3), echo, ..., console
├── a (28)
│   ├── b (29)
│   │   └── r2 (30)
│   └── r2 (31)
└── r2 (3)          ← same inode as cat
```

```
find(".", "r2")                 reads the root: . .. README cat ... console a r2
│  "."  ".."      skipped
│  "README" ...   not r2, not directories
│  "a"            not r2, but a directory →
│  ├─ find("./a", "r2")                  reads a: . .. b r2
│  │   "b"        directory →
│  │   ├─ find("./a/b", "r2")            reads b: . .. r2
│  │   │    "r2"  match → print ./a/b/r2
│  │   │    close
│  │   "r2"       match → print ./a/r2
│  │   close
│  "r2"           match → print ./r2
│  close
```

The output order, `./a/b/r2`, `./a/r2`, `./r2`, is exactly this walk. It depends on the order of the entries in each directory, which is the order they were created in (new entries go into the first free slot), so the sheet warns that it may differ.

### 6.4 Two limits to respect

- **Open files.** A process may have `NOFILE` = 16 files open. While `find` is inside `./a/b`, the directories `.`, `./a` and `./a/b` are all open at the same time, plus 0, 1, 2 (keyboard, screen, errors). So the deepest tree it can walk is about 13 levels. Without the `close(fd)`, every directory ever visited would stay open, and the 14th directory in the whole search, deep or not, would fail.
- **Stack.** An xv6 program has one page (4096 bytes) of stack (`USERSTACK 1`). Each level of the recursion uses its own `buf`, `de` and `st`: with a 512-byte `buf` like `ls.c` that is about 600 bytes per level, so a few levels deep would already be near the limit. With `MAXPATH` (128) it is about 200 bytes, so the open-files limit is reached first. The kernel refuses paths longer than `MAXPATH` anyway (`argstr(..., MAXPATH)`), so a bigger buffer would not help.

### 6.5 The build and the tests

![Figure 4.1 – user/find.c in nano, the first part](screenshots/fig-4-1.png)

![Figure 4.2 – user/find.c, the second part: the loop over the entries and main](screenshots/fig-4-2.png)

```bash
sed -i 's|^\t$U/_mystat\\$|&\n\t$U/_find\\|' Makefile
git diff Makefile | cat -A
git status -s
```

![Figure 4.3 – My two lines in the Makefile, and the new files](screenshots/fig-4-3.png)

The same `sed` pattern as in 4.3, now after the `mystat` line. `git diff` shows the changes against the last commit: two lines with `+`.

![Figure 4.4 – make clean ; make qemu: a fresh disk with mystat and find](screenshots/fig-4-4.png)

![Figure 4.5 – The test of the sheet](screenshots/fig-4-5.png)

### 6.6 Stretch: which r2 is cat?

![Figure 4.6 – The inode number next to each path](screenshots/fig-4-6.png)

Two paths are the same file exactly when they lead to the same inode (on the same disk; xv6 has one). `find . r2 -n` prints `./r2 3`, and `ls cat` prints `cat 2 3 36728`: both are inode 3. `mystat cat r2 a/r2 a/b/r2` confirms it: `cat` and `r2` have inode 3, 2 links and 36728 bytes, while `a/r2` (31) and `a/b/r2` (30) are separate 2-byte files. Running `r2` would run the `cat` program.

### 6.7 More tests

![Figure 4.7 – More tests: a deeper tree, an absolute path, no match, wrong arguments](screenshots/fig-4-7.png)

| Command | Result | What it shows |
|---|---|---|
| `find . r2` (after `a/b/c/d/r2`) | four paths | it goes four levels deep |
| `find a/b c` | `a/b/c` | any starting directory |
| `find /a d` | `/a/b/c/d` | absolute paths too |
| `find . nosuch` | nothing | no match is not an error |
| `find` | usage line | wrong number of arguments |
| `find README r2` | not a directory | the start must be a directory |

## 7. The xv6 tree at the end

![Figure 5.1 – The xv6 tree at the end: the Lab 6 commit on branch lab](screenshots/fig-5-1.png)

```bash
wget -q http://10.0.2.2:8765/notes.md
git add user/mystat.c user/find.c Makefile notes.md
git commit -q -m "Lab 6: mystat and find"
git log --oneline -3 ; git show --stat HEAD | tail -7
git status -s
```

`notes.md` was written on the Windows side and copied into the virtual machine with `wget` (10.0.2.2 is how the virtual machine reaches the Windows host). `git add` picks the four files, `git commit` saves them as one commit, `git log` shows it on top of the Lab 4 commit and the course revision `06aad25`, and `git show --stat` counts the lines: `mystat.c` 37, `find.c` 74, two Makefile lines, `notes.md`. `git status -s` printing nothing means nothing is left uncommitted.

The `Makefile` in this folder lists `sleep` and `args` too. Their sources are in [`../lab4/`](../lab4/); copy them along (see "How to run it" in the README), or `make` will stop because it cannot find `user/sleep.c`.

## 8. xv6 and ext4 side by side

| | xv6 | ext4 on the Ubuntu VM (Lab 5) |
|---|---|---|
| block size | 1024 bytes | 4096 bytes |
| disk size | 2000 blocks, about 2 MB | 49 GB |
| inodes | 200, fixed by `mkfs` | millions, fixed by `mkfs` (16384 in the 64 MB image) |
| inode size | 64 bytes | 256 bytes |
| inode fields | type, major, minor, nlink, size, 13 block numbers | also owner, group, mode, 4 times, flags, extents... |
| largest file | 268 KiB (12 direct + 256 indirect blocks) | 16 TiB |
| name length | 14 bytes, longer names are cut | 255 bytes, longer names are refused |
| directory entry | fixed 16 bytes | variable length |
| root inode | 1 | 2 |
| new directory's link count | 1 (`.` not counted) | 2 |
| hard links | yes | yes |
| symbolic links | no | yes |
| permissions, users | no | yes |
| journal | the log, 30 blocks + header | the journal (1024 blocks in the 64 MB image) |
| data freed when | `nlink == 0` and last `ref` dropped (`iput`) | links = 0 and last open descriptor closed |
| crash cleanup of orphans | `ireclaim()` at boot | the orphan list / orphan file |

## 9. Test yourself

Try to answer before opening each one.

<details>
<summary>1. What are the four columns of the xv6 <code>ls</code>?</summary>

Name, type (1 directory, 2 file, 3 device), inode number, size in bytes (3.2).
</details>

<details>
<summary>2. Why is <code>console</code> inode 25 on a fresh disk but 26 once <code>mystat</code> is added?</summary>

`console` is created by `init` at the first boot, after `mkfs` has given numbers to `README` and all the programs. One more program on the disk takes one more number before it (3.2).
</details>

<details>
<summary>3. How many entries does a directory of 48 bytes have, and which ones at least?</summary>

48 / 16 = 3 entries: always `.` and `..`, plus one more (in Figure 2.6, `d` with `x`) (3.5).
</details>

<details>
<summary>4. Why does the root show 1024 bytes when it has only 26 entries?</summary>

`mkfs` rounds the root's size up to a whole block. The unused part is entries with inode 0, free slots that `ls` and `find` skip (3.5).
</details>

<details>
<summary>5. Is <code>stat()</code> a system call in xv6?</summary>

No. It is a library function in `user/ulib.c` that calls `open`, the real system call `fstat`, and `close` (4.2).
</details>

<details>
<summary>6. After <code>ln README r2</code> and <code>rm README</code>, why is the content of <code>r2</code> still there?</summary>

`rm` only removed the entry `README` and lowered `nlink` from 2 to 1. The inode and its blocks are freed only when `nlink` is 0 and no reference is left (4.4, 4.5).
</details>

<details>
<summary>7. Which function frees the data blocks, and when?</summary>

`itrunc`, called from `iput()` in `kernel/fs.c`, when the last in-memory reference is dropped (`ref == 1`) and `nlink == 0` (4.5).
</details>

<details>
<summary>8. Why can xv6 not have permissions, even if a program wanted them?</summary>

`struct dinode` has no owner and no mode field: there is nowhere on the disk to store them (4.6).
</details>

<details>
<summary>9. How big is an xv6 inode on the disk, and how many fit in a block?</summary>

64 bytes (4 shorts, 1 uint, 13 uints), so 16 per 1024-byte block (4.6).
</details>

<details>
<summary>10. Compute the largest xv6 file again. What would change if blocks were 2048 bytes?</summary>

(12 + 1024 / 4) × 1024 = 274,432 bytes. With 2048-byte blocks: (12 + 2048 / 4) × 2048 = 524 × 2048 = 1,073,152 bytes (5.3).
</details>

<details>
<summary>11. What are the 47 metadata blocks?</summary>

1 boot, 1 superblock, 31 log (30 + header), 13 inode blocks, 1 bitmap block (5.4).
</details>

<details>
<summary>12. Why does <code>mkdir averyveryverylongXYZ</code> fail after <code>mkdir averyveryverylongname</code>?</summary>

Both are cut to the same 14 characters, `averyveryveryl`, and that name already exists (5.5).
</details>

<details>
<summary>13. Why must <code>find</code> skip <code>.</code> and <code>..</code>?</summary>

`.` is the same directory, so it would search it again and again forever; `..` would climb back up and loop too (6.1).
</details>

<details>
<summary>14. What goes wrong if <code>find</code> forgets <code>close(fd)</code>?</summary>

Every directory it visited stays open. A process can have only 16 open files, so after about 13 directories `open` fails (6.4).
</details>

<details>
<summary>15. How do you know that <code>./r2</code> and <code>cat</code> are the same file?</summary>

They have the same inode number, 3 (`find . r2 -n`, `ls cat`, `mystat`), and both show 2 links (6.6).
</details>

<details>
<summary>16. Why is <code>find a b</code> allowed to print a directory?</summary>

`find` compares the name of every entry, files and directories alike, and prints any match; then, separately, it goes into directories (6.1).
</details>
