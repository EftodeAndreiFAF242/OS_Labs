# Lab 5 – The File System with Your Own Hands: report.md

Eftode Andrei, FAF-242. Done on 13 October 2026 in the Ubuntu 26.04.1 LTS virtual machine (user `vboxuser`, host name `os`), in the folder `~/os-lab5`. The "laptop" of the sheet is this virtual machine, so the CPU, RAM and disk numbers below are the ones the virtual machine reports.

```
$ whoami
vboxuser
```

## Part 1. The tree and the file types

```
$ ls -ld /etc /etc/passwd /bin /dev/null /tmp
lrwxrwxrwx   1 root root     7 Apr 20 08:46 /bin -> usr/bin
crw-rw-rw-   1 root root  1, 3 Oct 12 10:55 /dev/null
drwxr-xr-x 142 root root 12288 Oct 13 12:03 /etc
-rw-r--r--   1 root root  2778 Oct  1 08:55 /etc/passwd
drwxrwxrwt  17 root root   380 Oct 13 12:03 /tmp
```

**(1) Types.** `/etc` is `d`, a directory. `/etc/passwd` is `-`, a regular file. `/bin` is `l`, a symbolic link. `/dev/null` is `c`, a character device. `/tmp` is `d`, a directory (the `t` at the end of its permissions is the sticky bit, see Part 3).

**(2) `/bin`.** It is not a directory. It is a symbolic link to `usr/bin`, a relative path, so it means `/usr/bin`. `readlink /bin` prints `usr/bin`. `/sbin` and `/lib` are links to `usr/sbin` and `usr/lib` in the same way. Even `/usr/bin/ls` is a link here, to `../lib/cargo/bin/coreutils/ls`: Ubuntu 26.04 uses the Rust rewrite of coreutils, one program with 115 hard links (`ls -l` shows a link count of 115).

**(3) Devices.** Character devices: `/dev/null` (`c`, major 1, minor 3), `/dev/zero` (1, 5), `/dev/random` (1, 8), `/dev/tty` (5, 0). Block devices: `/dev/sda` (`b`, major 8, minor 0), the virtual disk, with its partitions `/dev/sda1` and `/dev/sda2`; also `/dev/loop0` to `/dev/loop18` and the CD drive `/dev/sr0`.

**(4) Sizes.**

```
$ du -sh /var/log 2>/dev/null ; du -sh /usr 2>/dev/null
29M     /var/log
5.3G    /usr
```

`/usr` is bigger, by about 5.3 GB, roughly 190 times bigger. (As a normal user `du` cannot read some log files, and `2>/dev/null` hides those errors, so 29M may be a little too low.)

## Part 2. Inodes and links

```
$ stat notes.txt
  File: notes.txt
  size: 6               Blocks: 8          IO Block: 4096   regular file
Device: 8,2     Inode: 525387      Links: 1
$ ln notes.txt hard.txt ; ls -li notes.txt hard.txt
525387 -rw-rw-r-- 2 vboxuser vboxuser 6 Oct 13 12:06 hard.txt
525387 -rw-rw-r-- 2 vboxuser vboxuser 6 Oct 13 12:06 notes.txt
$ rm notes.txt ; cat hard.txt ; ls -li hard.txt
hello
525387 -rw-rw-r-- 1 vboxuser vboxuser 6 Oct 13 12:06 hard.txt
$ ln -s hard.txt soft.txt ; ls -li hard.txt soft.txt
525387 -rw-rw-r-- 1 vboxuser vboxuser 6 Oct 13 12:06 hard.txt
525388 lrwxrwxrwx 1 vboxuser vboxuser 8 Oct 13 12:06 soft.txt -> hard.txt
$ rm hard.txt ; cat soft.txt ; ls -l soft.txt
cat: soft.txt: No such file or directory
lrwxrwxrwx 1 vboxuser vboxuser 8 Oct 13 12:06 soft.txt -> hard.txt
```

**(1)** `Inode: 525387` is the inode number. `Links: 1` is the number of names (hard links) the file has. `ls -li` shows the same two numbers: the first column and the number after the permissions.

**(2)** `ln` did not copy anything. It wrote a second name, `hard.txt`, for the same inode 525387, and the link count became 2. `rm` removes a name, not a file: it deletes the directory entry `notes.txt` and lowers the link count to 1. The inode and its data stay, because `hard.txt` still points to them. The data is freed only when the count reaches 0 and no process has the file open.

**(3)** `soft.txt` is a separate inode (525388) whose content is only the text `hard.txt`. `cat soft.txt` follows that text as a path, and after `rm hard.txt` no file has that name, so it fails although `soft.txt` itself still exists (a dangling link, shown in red). Its size is 8 bytes because `hard.txt` is 8 characters long: the link stores exactly the path, nothing else.

**(4)** Prediction before running: 2, then 4. Result: 2, then 4.

```
$ ls -ldi d d/. d/x/.. d/y/..
525387 drwxrwxr-x 4 vboxuser vboxuser 4096 Oct 13 12:06 d
525387 drwxrwxr-x 4 vboxuser vboxuser 4096 Oct 13 12:06 d/.
525387 drwxrwxr-x 4 vboxuser vboxuser 4096 Oct 13 12:06 d/x/..
525387 drwxrwxr-x 4 vboxuser vboxuser 4096 Oct 13 12:06 d/y/..
```

A new directory has 2 names: `d` in `~/os-lab5`, and the entry `.` inside itself. Every subdirectory has a `..` entry that points back to its parent, so `x/..` and `y/..` add two more: 4. (The number 525387 is the inode that was freed by `rm hard.txt`; the file system gave it to `d`.)

**(5)**

```
$ echo x > one.txt ; ls -l one.txt ; du -h one.txt
-rw-rw-r-- 1 vboxuser vboxuser 2 Oct 13 12:06 one.txt
4.0K    one.txt
$ truncate -s 1G big.img ; ls -lh big.img ; du -h big.img
-rw-rw-r-- 1 vboxuser vboxuser 1.0G Oct 13 12:06 big.img
0       big.img
```

The file system gives space in whole blocks of 4096 bytes (`IO Block: 4096`, `Blocks: 8` of 512 bytes), so the 2-byte file takes one full 4 KiB block. `truncate` only writes the size 1 GiB into the inode and allocates no blocks (`Blocks: 0`): it is a sparse file, and reading it returns zeros. `ls` shows the size, `du` shows the space.

## Part 3. Permissions

```
$ chmod 600 s.txt ; ls -l s.txt
-rw------- 1 vboxuser vboxuser 7 Oct 13 12:07 s.txt
$ chmod 000 s.txt ; cat s.txt
cat: s.txt: Permission denied
$ chmod u+r s.txt ; cat s.txt
secret
$ umask ; touch new.txt ; mkdir newdir ; ls -ld new.txt newdir
0002
-rw-rw-r-- 1 vboxuser vboxuser    0 Oct 13 12:07 new.txt
drwxrwxr-x 2 vboxuser vboxuser 4096 Oct 13 12:07 newdir
```

**(1)**

```
600  rw-------   a private file: an SSH private key, s.txt
644  rw-r--r--   a normal file everyone may read: /etc/passwd
755  rwxr-xr-x   a program or folder everyone may use: /usr/bin/ls, /etc
700  rwx------   a private folder or script: ~/.ssh
```

**(2)** The kernel picks one set of bits: the owner's, if you are the owner. With `000` the owner bits are `---`, so reading is refused; the group and other bits are not tried, and owning a file grants no access by itself. What the owner can do is change the mode, which is how `chmod u+r` fixed it. Only root skips these checks.

**(3)** The umask is `0002` (`umask -S`: `u=rwx,g=rwx,o=rx`). It removes write for others. Files start from 666, so `666 - 002 = 664` (`rw-rw-r--`, new.txt); directories start from 777, so `777 - 002 = 775` (`rwxrwxr-x`, newdir).

**(4)** Prediction before running: read only, `ls` works and `cat` fails; execute only, `ls` fails and `cat` works. Result: exactly that.

```
$ chmod u=r,go= box  ; ls box ; cat box/f     # read only
f
cat: box/f: Permission denied
$ chmod u=x,go= box  ; ls box ; cat box/f     # execute only
ls: cannot open directory 'box': Permission denied
hi
```

`r` on a directory lets you read the list of names, nothing more: `ls box` printed `f`, but `ls -l box` showed `-????????? ? ? ? ? ? f` because it could not open the entries. `x` lets you go through the directory to a name you already know: `cat box/f` worked, but the list cannot be read.

**(5)**

```
$ stat -c '%a  %A  %U:%G  %n' /usr/bin/passwd /tmp /etc/shadow
4755  -rwsr-xr-x  root:root  /usr/bin/passwd
1777  drwxrwxrwt  root:root  /tmp
640  -rw-r-----  root:shadow  /etc/shadow
$ cat /etc/shadow
cat: /etc/shadow: Permission denied
```

The `s` of `passwd` is the setuid bit: the program runs with the identity of its owner, root, whoever starts it. `passwd` needs it because it writes the new password hash into `/etc/shadow`, which only root may write (I cannot even read it). The `t` of `/tmp` is the sticky bit: everyone may create files there, but only the owner of a file (or root) may delete or rename it.

## Part 4. Disks and the kernel as files

```
$ lsblk -e7 -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINTS
NAME    SIZE TYPE FSTYPE MOUNTPOINTS
sda    50.5G disk
├─sda1    1G part vfat   /boot/efi
└─sda2 49.4G part ext4   /
sr0    1024M rom
$ mount | grep " / "
/dev/sda2 on / type ext4 (rw,relatime)
```

**(1)**

```
/dev/sda   50.5G   the virtual disk
 ├── sda1   1G      vfat   mounted at /boot/efi   (EFI boot partition)
 └── sda2   49.4G   ext4   mounted at /           (everything else)
loop0 ... loop17: snap packages, mounted read-only under /snap
sr0: the virtual CD drive, empty
```

**(2)** `df -h /`: size 49G, used 7.8G, available 39G, **17%** full.

**(3)** `grep -c processor /proc/cpuinfo` prints **8** cores. `/proc/meminfo` says `MemTotal: 5776308 kB`, about **5.5 GiB** of RAM. These are the virtual machine's numbers (it was given 8 processors and 6403 MB; the kernel keeps part of the RAM for itself). `/proc/cpuinfo` also shows the real processor: AMD Ryzen 9 9955HX 16-Core.

**(4)** It showed `Name: head`. `/proc/self` always means "the process that is reading this", and the process that opened `/proc/self/status` was `head`. With `grep Name /proc/self/status` it says `grep`, and with `cat` it says `cat`. In the same way `ls -l /proc/self/fd` showed the descriptors of `ls` itself: 0, 1, 2 on the terminal `/dev/pts/0`, and 3, the directory `ls` had open.

## Stretch (all four)

**(a) Magic numbers.**

```
$ gzip -c /etc/passwd > notes.txt ; file notes.txt ; xxd notes.txt | head -1
notes.txt: gzip compressed data, was "passwd", ...
00000000: 1f8b 0808 1a20 be6a 0003 7061 7373 7764  ..... .j..passwd
```

The first two bytes, `1f 8b`, are the gzip magic number (`08` after them is the compression method, deflate). `file` reads these bytes and ignores the name: a copy called `photo.jpg` is still "gzip compressed data". A program starts with `7f 45 4c 46` (`.ELF`). A text file has no magic number; `file` guesses "ASCII text" from the characters.

**(b) setuid programs.** `find / -xdev -perm -4000 -type f` found 16: `pppd`, `su`, `chsh`, `mount`, `ntfs-3g`, `fusermount3`, `pkexec`, `passwd`, `sudo.ws`, `chfn`, `gpasswd`, `umount`, `ssh-keysign`, `/usr/lib/cargo/bin/su`, `/usr/lib/cargo/bin/sudo`, `dbus-daemon-launch-helper`. Besides the classic ones (`passwd`, `su`, `sudo`, `mount`, `umount`, `chsh`, `chfn`, `gpasswd`), this Ubuntu 26.04 has two versions of `sudo`: the classic one, renamed `sudo.ws`, and the Rust rewrite `sudo-rs` in `/usr/lib/cargo/bin`, which `/usr/bin/sudo` points to through `/etc/alternatives`. There is also a Rust `su`, and helpers for mounting (`ntfs-3g`, `fusermount3`), dial-up (`pppd`), polkit and D-Bus.

**(c) A deleted file that is still open.**

```
$ exec 3< bigfile ; rm bigfile
$ ls -l /proc/$$/fd
lr-x------ 1 vboxuser vboxuser 64 Oct 13 12:11 3 -> '/home/vboxuser/os-lab5/bigfile (deleted)'
$ df -m . | tail -1        # still 8121 MB used
$ exec 3<&-
$ df -m . | tail -1        # 7921 MB used
```

The link says `(deleted)`: the name is gone, but the shell still holds the inode open through descriptor 3. The 200 MB came back only after `exec 3<&-` closed it, when the last reference disappeared.

**(d) A file system inside a file.** `mkfs.ext4 disk.img` printed `Creating filesystem with 16384 4k blocks and 16384 inodes`, and `df -i /mnt` confirmed **16384 inodes** (12 used, 16372 free): one inode for every 4 KiB block. The mounted file system showed 56M (the journal and the tables take the rest of the 64M) and only `lost+found` inside.

## Conclusion

A name is not a file. The file is the inode (a number with the size, owner, permissions and block list), and directories are tables of names pointing to inode numbers. Hard links are more names for one inode, symbolic links are small files containing a path, and the data is freed only when no name and no open descriptor is left. Permissions are three sets of `rwx` that mean different things on files and directories, and the kernel, the disks and the processes can all be read as files under `/dev` and `/proc`.
