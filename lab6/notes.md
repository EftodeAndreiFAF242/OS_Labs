# Lab 6 – Inside the xv6 File System: notes.md

Eftode Andrei, FAF-242. Done on 13 October 2026 in `~/xv6-riscv`, branch `lab` (xv6 revision `06aad25` plus my Lab 4 commit), Ubuntu 26.04.1 LTS, QEMU 10.2.1. My tree also has `sleep`, `args` (Lab 4), `mystat` and `find`, so some inode numbers and sizes are a little different from the sheet (for example, once `mystat` is on the disk, `console` is inode 26, not 25).

## Part 1. Read what xv6 shows you

```
$ ls
.              1 1 1024
..             1 1 1024
README         2 2 2441
cat            2 3 36728
...
console        3 25 0
$ mkdir a
$ ls a
.              1 26 32
..             1 1 1024
```

**(1)** From `kernel/stat.h`: `T_DIR 1` is a directory, `T_FILE 2` is a regular file, `T_DEVICE 3` is a device (`console`).

**(2)** The root directory is inode **1** (`#define ROOTINO 1` in `kernel/fs.h`). The root has no parent, so `mkfs` writes both entries with the root's own number (`mkfs/mkfs.c`, lines 125 and 130: `de.inum = xshort(rootino)` for `.` and again for `..`). Going "up" from `/` stays in `/`.

**(3)** One entry is **16 bytes**: `struct dirent` has `ushort inum` (2 bytes) and `char name[DIRSIZ]` with `DIRSIZ 14` (14 bytes). 32 / 16 = 2 entries: `.` (inode 26, the directory `a` itself) and `..` (inode 1, the root).

## Part 2. mystat

`user/mystat.c` is in this folder. Output:

```
$ mystat README . console
README: file, inode 2, links 1, size 2441 bytes
.: directory, inode 1, links 1, size 1024 bytes
console: device, inode 26, links 1, size 0 bytes
$ ln README r2
$ mystat README r2
README: file, inode 2, links 2, size 2441 bytes
r2: file, inode 2, links 2, size 2441 bytes
$ rm README
$ mystat r2
r2: file, inode 2, links 1, size 2441 bytes
$ cat r2 | wc
48 336 2441
```

**(1)** After `ln`, `README` and `r2` are the same inode 2 and the link count went from 1 to 2. After `rm README`, `r2` is still inode 2 with all 2441 bytes (`wc` counts them), and the link count is back to 1. What changed: the link count and the names in the directory. What did not change: the inode number, the size and the data.

**(2)** `kernel/sysfile.c`, line 244, in `sys_unlink`: `ip->nlink--;` (line 239, `dp->nlink--`, is only for directories: the parent loses the link of the removed `..`). The data blocks are not freed there. They are freed in `iput()` in `kernel/fs.c`: when the last in-memory reference is dropped (`ip->ref == 1`) and `ip->nlink == 0`, it calls `itrunc(ip)` (line 365), which frees the data blocks, and then `ifree()` frees the inode itself. So a file that some process still has open keeps its blocks until that process closes it, as on Linux.

**(3)** `struct dinode` in `kernel/fs.h` has only: `type`, `major`, `minor` (device numbers, for devices only), `nlink`, `size` and `addrs[NDIRECT + 1]` (12 direct block numbers and 1 indirect block). There is no owner, no permission bits, no times, so 64 bytes per inode and 16 inodes per 1024-byte block.

Extra, about directories: in xv6 a new directory has 1 link, not 2 as on Linux. `create()` in `sysfile.c` does not count `.` ("avoid cyclic ref count"), but it does add one to the parent for the child's `..`: after `mkdir d`, the root went from 1 to 2 links, and after `mkdir d/x`, `d` went from 1 to 2 and its size from 32 to 48 bytes (3 entries).

## Part 3. Measure the limits

```
5:#define BSIZE   1024 // block size
26:#define NDIRECT   12
27:#define NINDIRECT (BSIZE / sizeof(uint))
54:#define DIRSIZ 14
nmeta 47 (boot, super, log blocks 31, inode blocks 13, bitmap blocks 1) blocks 1953 total 2000
```

**(1)** `ls` shows `averyveryveryl`: only the first 14 characters, because `DIRSIZ` is 14 and a directory entry has room for 14 bytes of name. The kernel cuts every name to 14 characters when it looks it up too, so `cd averyveryverylongname` still works, and `mkdir averyveryverylongXYZ` fails because it gives the same 14 characters.

**(2)**

```
NINDIRECT = BSIZE / sizeof(uint) = 1024 / 4 = 256 block numbers
MAXFILE   = NDIRECT + NINDIRECT  = 12 + 256 = 268 blocks
largest   = 268 * 1024 = 274,432 bytes = 268 KiB
```

**(3)** The disk has **2000** blocks of 1 KiB. **47** are metadata: 1 boot block, 1 superblock, **31** log blocks (30 for data plus 1 header), **13** inode blocks (200 inodes, 16 per block), 1 bitmap block. **1953** blocks are for data.

## Part 4. find

`user/find.c` is in this folder. It opens the directory, reads `struct dirent` entries, skips free slots (`inum == 0`), `.` and `..`, builds the full path, prints it when the last component equals the name, and calls itself for every subdirectory. Every directory it opens is closed.

```
$ find . r2
./a/b/r2
./a/r2
./r2
$ find a b
a/b
```

**Stretch.** `find . r2 -n` prints the inode number next to each path:

```
$ find . r2 -n
./a/b/r2 30
./a/r2 31
./r2 3
$ ls cat
cat            2 3 36728
```

`./r2` is inode 3, the same as `cat`: it was made with `ln cat r2`, so it is the same file (both have 2 links). `./a/r2` and `./a/b/r2` are different files created with `echo`.
