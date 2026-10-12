# Lab 3 – Full explanations

Course: Operating Systems, FCIM / FAF, UTM, 2026-2027. Lab 3: The Command Line (shell, pipes and text filters).

[`README.md`](README.md) tells what was done in the lab. This file goes slower. It takes every command and every line of the two scripts apart, piece by piece, shows what each piece receives and what it gives back, and checks each answer in a second way. It is written for someone who has never used Linux. The figure numbers are the same as in the README and in the Word report.

## Contents

1. [Words you need first](#1-words-you-need-first)
2. [Before the lab](#2-before-the-lab)
3. [Part 1: move fast, find help](#3-part-1-move-fast-find-help)
4. [Part 2: what the shell does to your line](#4-part-2-what-the-shell-does-to-your-line)
5. [Part 3: redirection, chaining and jobs](#5-part-3-redirection-chaining-and-jobs)
6. [Part 4: one-liners](#6-part-4-one-liners)
7. [Part 5: the script report.sh](#7-part-5-the-script-reportsh)
8. [Every symbol on one page](#8-every-symbol-on-one-page)
9. [Test yourself](#9-test-yourself)

## 1. Words you need first

| Word | Meaning | In Windows it is like |
|---|---|---|
| terminal | the window where you type | the black Command Prompt window |
| shell | the program inside that window. It reads the line you type and runs it. Here it is `bash` | `cmd.exe` or PowerShell |
| command | the first word of a line: the program to run | the same |
| argument | every word after the command | the same |
| option | an argument that starts with `-` and changes how the command works | switches like `/s` |
| path | where a file is. Folders are separated by `/`, and `/` alone is the top folder | `C:\Users\...`, with `\` |
| `~` | my home folder, `/home/vboxuser` | `C:\Users\MyName` |
| `.` and `..` | the folder I am in, and the folder above it | the same |
| process | a program that is running right now | a row in Task Manager |
| exit code | a number every command leaves when it ends: 0 = it worked, anything else = it failed | `%ERRORLEVEL%` |

How to read one command line:

```
ls   -l   /etc
│    │    └── argument: what to work on (the folder /etc)
│    └─────── option: how to do it (-l = long format)
└──────────── command: which program to run (ls = list a folder)
```

How to read the prompt `vboxuser@os:~/os-lab3$`:

```
vboxuser @ os : ~/os-lab3 $
   │        │       │     └── $ = a normal user (root, the administrator, gets #)
   │        │       └──────── the folder I am in
   │        └──────────────── the name of the computer
   └───────────────────────── my user name
```

The one idea that explains the whole lab: **bash rewrites the line before it starts any program.** The program never sees what I typed. It sees what is left after bash has replaced the patterns, the variables and the quotes.

## 2. Before the lab

### 2.1 The setup script

```bash
cd ~/Downloads
bash os2026-setup.sh
```

| Piece | What it does |
|---|---|
| `cd ~/Downloads` | `cd` = change directory. Go to the folder `Downloads` inside my home folder |
| `bash os2026-setup.sh` | start a second bash and let it read its commands from the file, not from the keyboard. A file full of commands is called a script |

The script prints `[ OK ]` after each thing that worked:

![Figure 0.1 – The course script ends with All set.](screenshots/fig-0-1.png)

1. **Packages.** It installs programs with `apt`, the Ubuntu installer (the idea of the Microsoft Store, but typed): `git`, `make`, `gcc`, the RISC-V compiler, QEMU, `man-db`, `cron`, `python3`, `bc`. Mine were already installed, so every line says "is already the newest version".
2. **Tools.** It checks that each tool answers.
3. **xv6.** It looks for the small teaching system in `~/xv6-riscv`. Mine was there from Lab 2, so it was left alone.
4. **Build and boot.** It builds xv6 and starts it once.

One repair was needed first. `apt` keeps a list of places to download from, and this virtual machine still had its installation CD on that list (`/etc/apt/sources.list.d/cdrom.sources`). The CD is gone, so `apt-get update` failed and the script stopped. I added the line `Enabled: no` at the top of that file, which switches off that one source.

### 2.2 The data set, piece by piece

Part 4 needs a file to ask questions about. The sheet gives this block:

```bash
mkdir -p ~/os-lab3 && cd ~/os-lab3
for i in $(seq 1 500); do
  echo "2026-10-05 10:$(printf %02d $((i % 60))) user$((i % 7)) $([ $((i % 5)) -eq 0 ] && echo FAIL || echo OK) /page$((i % 4))"
done > access.log
head -3 access.log ; wc -l access.log
```

![Figure 0.2 – Generating access.log](screenshots/fig-0-2.png)

| Piece | What it does |
|---|---|
| `mkdir -p ~/os-lab3` | make the folder. `-p` means "do not complain if it already exists" |
| `&&` | run the next command only if this one worked |
| `cd ~/os-lab3` | go into the new folder |
| `for i in LIST; do ... done` | repeat the lines between `do` and `done`, once for every word in LIST. Each time, `i` holds the current word |
| `seq 1 500` | a program that prints the numbers 1, 2, ... 500 |
| `$(seq 1 500)` | run that program and paste what it printed right here. So LIST becomes `1 2 3 ... 500` |
| `echo "..."` | print one line of text |
| `$((i % 60))` | arithmetic. `%` is the remainder of a division, so the result is always between 0 and 59. This is the minute |
| `printf %02d N` | print the number N with two digits: `5` becomes `05` |
| `user$((i % 7))` | the word `user` glued to a number from 0 to 6 |
| `[ $((i % 5)) -eq 0 ]` | a test. `[` is a command that succeeds when the test is true. `-eq` means "equal, as numbers" |
| `TEST && echo FAIL \|\| echo OK` | if the test is true print `FAIL`, if not print `OK` |
| `/page$((i % 4))` | `/page` glued to a number from 0 to 3 |
| `done > access.log` | everything the loop prints goes into the file `access.log`, not onto the screen |
| `head -3 access.log` | show the first 3 lines of the file |
| `;` | then run the next command, whatever happened |
| `wc -l access.log` | count the lines of the file. It printed `500 access.log` |

The `>` at the start of the second and third lines of the loop is not typed by me. Bash prints it to say "the command is not finished, keep typing".

One turn of the loop by hand, for `i = 5`:

```
5 % 60 = 5   →  printf %02d 5  →  05
5 % 7  = 5   →  user5
5 % 5  = 0   →  the test is true  →  FAIL
5 % 4  = 1   →  /page1

line 5 of the file:   2026-10-05 10:05 user5 FAIL /page1
```

And for `i = 60`: `60 % 60 = 0` gives minute `00`, `60 % 7 = 4` gives `user4`, `60 % 5 = 0` gives `FAIL`, `60 % 4 = 0` gives `/page0`. Line 60 of the file is `2026-10-05 10:00 user4 FAIL /page0`.

So every line has five fields, separated by one space:

```
2026-10-05 10:05 user5 FAIL /page1
    1        2     3    4     5
  date     time  user result page
```

Because the file is made by plain arithmetic, every answer of Part 4 can be worked out on paper first:

| Question | Reasoning | Answer |
|---|---|---|
| How many lines fail? | a line fails when `i` divides by 5: 500 / 5 | 100 |
| How many pages? | `i % 4` has 4 possible results, each equally often: 500 / 4 | 4 pages, 125 lines each |
| How many users? | `i % 7` has 7 possible results. 500 = 7 × 71 + 3, so three users get one extra line | 7 users: `user1`, `user2`, `user3` have 72 lines, the others 71 |
| Who fails most? | the failing lines are `i` = 5, 10, 15, ... 500: one hundred of them. 100 = 7 × 14 + 2, so two users get one extra failure. They are the users of the first two failing lines: line 5 (`user5`) and line 10 (`user3`) | `user3` and `user5` with 15, the others 14 |

These numbers come back in Part 4, where the commands find the same answers.

## 3. Part 1: move fast, find help

### 3.1 Tab completion

The Tab key asks bash to finish the word I am typing. It looks at the real names on the disk.

![Figure 1.1 – Tab completion](screenshots/fig-1-1.png)

| What I typed | What happened | Why |
|---|---|---|
| `ls /usr/sh` + Tab | the line became `ls /usr/share/` | only one name in `/usr` starts with `sh` |
| Tab again | nothing | there is nothing left to finish |
| `ls /usr/s` + Tab + Tab | bash printed `sbin/  share/  src/` and showed my line again | three names start with `s`. Bash cannot choose, so the second Tab lists them |
| `cd /et` + Tab | the line became `cd /etc/` | only one name in `/` starts with `et` |

`^C` in the picture is Ctrl+C. It throws away the line I was typing without running it. The last line, `cd ~/os-lab3`, brings me back to the lab folder.

### 3.2 History search with Ctrl+R

Bash remembers every line I run. Ctrl+R searches that list backwards.

![Figure 1.5 – Ctrl+R](screenshots/fig-1-5.png)

I ran three commands, then pressed Ctrl+R and typed `da`:

| Command | What it printed | What it is |
|---|---|---|
| `date` | `Mon Oct  5 02:34:17 PM UTC 2026` | the date and time |
| `whoami` | `vboxuser` | my user name |
| `uname -r` | `7.0.0-34-generic` | the version of the Linux kernel |

The last line of the picture, ``(reverse-i-search)`da': date``, is bash saying: "the newest command that contains `da` is `date`". Enter runs it again. This is faster than pressing the up arrow many times.

### 3.3 Is it built in, or a file on the disk?

```bash
type cd ; type ls ; type echo ; which ls
```

![Figure 1.2 – type and which](screenshots/fig-1-2.png)

A name I type as a command can be one of three things:

| Kind | What it is | Example from the picture |
|---|---|---|
| built-in | a part of bash itself. There is no separate file | `cd is a shell builtin`, `echo is a shell builtin` |
| alias | a nickname that bash replaces with other text | ``ls is aliased to `ls --color=auto'``: Ubuntu makes `ls` always show colours |
| external program | a file on the disk that bash starts as a new process | `which ls` prints `/usr/bin/ls` |

`type NAME` tells which kind a name is. `which NAME` prints the file that would be run. The `;` between them only separates four commands written on one line.

**Why `cd` must be a built-in.** Every process has its own current folder. If `cd` were a separate program, bash would start a new process, that process would change *its own* folder, then exit, and bash would still be where it was. This can be seen with two lines:

```bash
bash -c 'cd /tmp ; pwd'    # a second bash moves to /tmp and prints /tmp, then ends
pwd                        # my shell is still in ~/os-lab3
```

Only the shell can change the shell's folder, so `cd` has to live inside the shell.

### 3.4 PATH: where bash looks for programs

```bash
echo $PATH
echo $PATH | tr ':' '\n'
echo $PATH | tr ':' '\n' | wc -l
```

![Figure 1.3 – The directories in PATH](screenshots/fig-1-3.png)

| Piece | What it does |
|---|---|
| `$PATH` | a variable: a name that holds a value. The value of `PATH` is a list of folders with `:` between them. Bash replaces `$PATH` with that list before `echo` starts |
| `echo` | prints its arguments, so here it prints the list |
| `\|` | a pipe: what the left command prints becomes the input of the right command |
| `tr ':' '\n'` | `tr` = translate. It replaces every `:` with `\n`, which means "new line". Now there is one folder per line |
| `wc -l` | counts lines: **10** |

When I type `ls`, bash walks through these folders from the first to the last and runs the first file named `ls` that it finds:

```
/usr/local/sbin   no ls here
/usr/local/bin    no ls here
/usr/sbin         no ls here
/usr/bin          found: /usr/bin/ls   →  run it, stop looking
```

Windows has the same idea, also called `PATH`, with `;` between the folders.

### 3.5 Exit codes

```bash
nosuchprog ; echo "exit code: $?"
```

![Figure 1.4 – Exit code of a missing command](screenshots/fig-1-4.png)

- `nosuchprog` is not a built-in, not an alias, and not in any of the 10 folders. Bash prints `command not found`.
- `$?` is a special variable: the exit code of the last command. Bash replaces it with **127**, the number it uses for "I could not find that command".

Exit codes met in this lab:

| Code | Meaning | Where |
|---|---|---|
| 0 | it worked | every normal command |
| 1 | the script's own "something is wrong" | `report.sh` with a missing file (Part 5) |
| 2 | `ls` could not open what I asked for | `ls /nope` (Part 3) |
| 127 | command not found | `nosuchprog` here, and `report.sh` without `./` in Part 5 |

`&&` and `||` (Part 3) and `if` (Part 5) all work by looking at this number.

### 3.6 Help without a browser

![Figure 1.6 – man ls, searching for sort](screenshots/fig-1-6.png)

`man ls` opens the manual of `ls`. Keys inside it:

| Key | Does |
|---|---|
| `/sort` then Enter | search for the word "sort". Every match is highlighted |
| `n` | jump to the next match |
| Space | next page |
| `q` | quit |

After a few `n` the screen shows `-t  Sort by modification time (the 'mtime' in the inode), newest first.` That is the answer to question 4: the option is **`-t`**.

![Figure 1.7 – ls --help and man -k](screenshots/fig-1-7.png)

- `ls --help | head`: almost every program prints a short help with `--help`. `head` keeps only the first 10 lines, so the screen is not flooded.
- `man -k "list directory"`: `-k` searches the one-line descriptions of all manuals. It answers "which command does this?" when I do not know the name. The number in brackets is the chapter of the manual: `(1)` is ordinary commands, `(8)` is commands for administrators.

![Figure 1.8 – help cd](screenshots/fig-1-8.png)

- `help cd`: built-ins are not programs, so they have no manual page of their own. `help` is bash's own help for them.

## 4. Part 2: what the shell does to your line

### 4.1 The order of the work

After I press Enter, bash does these steps to the line, in this order. Only at the end does a program start.

| Step | What bash does | Example |
|---|---|---|
| 1 | cuts the line into words at the spaces. Quotes keep spaces inside one word | `ls "alfa beta.txt"` is 2 words, not 3 |
| 2 | writes out braces | `log{1..3}.txt` → `log1.txt log2.txt log3.txt` |
| 3 | replaces `~` | `~/os-lab3` → `/home/vboxuser/os-lab3` |
| 4 | replaces variables, `$( )` and `$(( ))` | `$HOME` → `/home/vboxuser`, `$(whoami)` → `vboxuser`, `$((7 * 6))` → `42` |
| 5 | replaces patterns with the file names that match | `*.c` → `a.c b.c` |
| 6 | removes the quotes and backslashes that I typed | `"alfa beta.txt"` → `alfa beta.txt`, still one word |
| 7 | connects the redirections (`>`, `<`, `2>`), then starts the program with the words that are left | Part 3 |

### 4.2 The test folder

```bash
mkdir -p ~/os-lab3/play && cd ~/os-lab3/play
touch a.c b.c file1.txt file2.txt notes.txt "alfa beta.txt"
ls
```

![Figure 2.1 – Files in the play directory](screenshots/fig-2-1.png)

- `touch NAME` creates an empty file with that name. Six names, six files.
- `"alfa beta.txt"` is in quotes so that it stays one name. Without the quotes `touch` would create two files, `alfa` and `beta.txt`.

The picture also shows `ls -l`, the long format. Reading one line of it:

```
-rw-rw-r--  1  vboxuser  vboxuser  0  Oct  5 14:35  a.c
    │       │     │         │      │       │         └── name
    │       │     │         │      │       └──────────── last change
    │       │     │         │      └──────────────────── size in bytes (0: touch makes empty files)
    │       │     │         └─────────────────────────── group that owns it
    │       │     └───────────────────────────────────── user that owns it
    │       └─────────────────────────────────────────── number of names this file has
    └─────────────────────────────────────────────────── kind and permissions (explained in Part 5)
```

### 4.3 Patterns

```bash
echo *.c
echo file?.txt
echo [a-b]*
echo *.xyz
```

![Figure 2.2 – Globbing results](screenshots/fig-2-2.png)

Replacing a pattern with file names is called globbing. Three signs are used:

| Sign | Matches | Example | Result here |
|---|---|---|---|
| `*` | any text, also nothing | `*.c` | `a.c b.c` |
| `?` | exactly one character | `file?.txt` | `file1.txt file2.txt` |
| `[a-b]` | exactly one character from the list | `[a-b]*` | `a.c alfa beta.txt b.c` |

`echo` does nothing but print its arguments. It never opens a folder. The file names came from bash, which replaced the pattern and only then started `echo`.

Two details that are easy to miss:

- `echo [a-b]*` printed `a.c alfa beta.txt b.c`. That looks like four names, but `echo` received **three** arguments: `a.c`, `alfa beta.txt` and `b.c`. The pattern only asks for a first letter `a` or `b`, so the file with the space matches too.
- `echo *.xyz` printed `*.xyz`. No file ends in `.xyz`, and when nothing matches, bash leaves the pattern as it was. `echo` received the five characters `*.xyz` and printed them.

### 4.4 A name with a space

```bash
ls alfa beta.txt
ls "alfa beta.txt"
ls alfa\ beta.txt
```

![Figure 2.3 – The ls error and the fix](screenshots/fig-2-3.png)

| Line | What `ls` received | Result |
|---|---|---|
| `ls alfa beta.txt` | two names: `alfa` and `beta.txt` | two errors, because neither file exists |
| `ls "alfa beta.txt"` | one name: `alfa beta.txt` | the file is found |
| `ls alfa\ beta.txt` | one name: `alfa beta.txt` | the file is found |

A space cuts the line into words (step 1). Quotes stop that for everything between them. A backslash stops it for the one character after it.

`ls` printed the name as `'alfa beta.txt'`. Those quotes are not part of the name. `ls` adds them on the screen when a name has a space, so that I can see where the name begins and ends and can copy it back into a command.

### 4.5 Quotes

```bash
echo '$HOME' "$HOME" \$HOME
```

![Figure 2.4 – Single quotes, double quotes, backslash](screenshots/fig-2-4.png)

It printed `$HOME /home/vboxuser $HOME`.

| Written as | Bash does | `echo` receives |
|---|---|---|
| `'$HOME'` | nothing. Inside single quotes every character is just itself | `$HOME` |
| `"$HOME"` | replaces the variable. Inside double quotes `$name`, `$( )` and `$(( ))` still work, but spaces and `*` are plain characters | `/home/vboxuser` |
| `\$HOME` | the backslash makes the next character plain, so `$` is only a dollar sign | `$HOME` |

A rule of thumb: single quotes when the text must stay exactly as typed, double quotes when the text has a variable in it.

### 4.6 Braces, arithmetic, command substitution

```bash
echo {1..5} ; echo $((7 * 6))
echo "today is $(date +%A), I am $(whoami)"
touch log{1..3}.txt && ls log*
```

![Figure 2.5 – Braces, arithmetic, command substitution](screenshots/fig-2-5.png)

| Written as | Name | Bash turns it into |
|---|---|---|
| `{1..5}` | brace expansion | `1 2 3 4 5`. It only writes text. It does not look at the disk |
| `$((7 * 6))` | arithmetic | `42`. Inside `$(( ))` the `*` means "multiply", not "any file" |
| `$(date +%A)` | command substitution | bash runs `date +%A` and pastes the result. `+%A` asks `date` for the name of the day only: `Monday` |
| `$(whoami)` | command substitution | `vboxuser` |

The last line uses two of them together. Bash first rewrites it as:

```bash
touch log1.txt log2.txt log3.txt && ls log1.txt log2.txt log3.txt
```

The braces made three names out of nothing. `touch` created the files, and because `touch` worked (`&&`), `ls` ran. By then the files existed, so `log*` matched them.

### 4.7 The ten lines: typed, received, printed

| What I typed | What the program really received | What was printed |
|---|---|---|
| `echo *.c` | `echo` + `a.c` `b.c` | `a.c b.c` |
| `echo file?.txt` | `echo` + `file1.txt` `file2.txt` | `file1.txt file2.txt` |
| `echo [a-b]*` | `echo` + `a.c` `alfa beta.txt` `b.c` | `a.c alfa beta.txt b.c` |
| `echo *.xyz` | `echo` + `*.xyz` | `*.xyz` |
| `ls alfa beta.txt` | `ls` + `alfa` `beta.txt` | two errors |
| `ls "alfa beta.txt"` | `ls` + `alfa beta.txt` | `'alfa beta.txt'` |
| `echo '$HOME' "$HOME" \$HOME` | `echo` + `$HOME` `/home/vboxuser` `$HOME` | `$HOME /home/vboxuser $HOME` |
| `echo {1..5} ; echo $((7 * 6))` | `echo` + `1` `2` `3` `4` `5`, then `echo` + `42` | `1 2 3 4 5` and `42` |
| `echo "today is $(date +%A), I am $(whoami)"` | `echo` + one argument: `today is Monday, I am vboxuser` | `today is Monday, I am vboxuser` |
| `touch log{1..3}.txt && ls log*` | `touch` + three names, then `ls` + the same three names | `log1.txt log2.txt log3.txt` |

The middle column is the lesson of this part. In the report, four of the ten predictions were wrong (`[a-b]*`, `*.xyz`, the name with a space, and the quotes that `ls` adds). The table of predictions is in the [README](README.md#the-predictions-table).

## 5. Part 3: redirection, chaining and jobs

### 5.1 Three channels

Every program starts with three open channels. Each has a number.

| Number | Name | Used for | Normally goes to |
|---|---|---|---|
| 0 | stdin (standard input) | what the program reads | the keyboard |
| 1 | stdout (standard output) | the normal result | the screen |
| 2 | stderr (standard error) | error messages | the screen |

Redirection means: connect one of these channels to a file. Bash does it before the program starts (step 7 of the table in 4.1). The program does not know. It keeps writing to "channel 1" as always.

| Written as | Means |
|---|---|
| `> file` | channel 1 goes into the file. The file is emptied first |
| `>> file` | channel 1 is added to the end of the file |
| `2> file` | channel 2 goes into the file |
| `< file` | channel 0 comes from the file |
| `2>&1` | channel 2 goes to the place where channel 1 goes **right now** |
| `>&2` | channel 1 goes to the place where channel 2 goes right now |

### 5.2 Result and errors in two files

```bash
cd ~/os-lab3
ls /etc /nope > out.txt 2> err.txt
wc -l out.txt ; cat err.txt
```

![Figure 3.1 – stdout and stderr in separate files](screenshots/fig-3-1.png)

`ls /etc /nope` was chosen because it produces both kinds of output: a real listing (the folder `/etc` exists) and an error (`/nope` does not).

```
without redirection                 with  > out.txt 2> err.txt

channel 1 ──> screen  (listing)     channel 1 ──> out.txt
channel 2 ──> screen  (error)       channel 2 ──> err.txt
```

Nothing appeared on the screen, because both channels went into files.

- `wc -l out.txt` printed `238 out.txt`. `/etc` holds 237 entries (Part 4, Q8). The one extra line is the title `/etc:` that `ls` prints when it is given more than one name.
- `cat err.txt` prints the content of the file: the one error line, `ls: cannot access '/nope': No such file or directory`.

### 5.3 The order of redirections

```bash
ls /etc /nope > all.txt 2>&1 ; tail -2 all.txt
ls /etc /nope 2>&1 > all.txt
```

![Figure 3.2 – Comparing the order of redirections](screenshots/fig-3-2.png)

Bash reads redirections from left to right, and `2>&1` copies the place where channel 1 points at that moment. It does not tie the two channels together for ever.

**First version: `> all.txt 2>&1`**

| Step | Channel 1 goes to | Channel 2 goes to |
|---|---|---|
| start | screen | screen |
| `> all.txt` | **all.txt** | screen |
| `2>&1` (copy where 1 goes now) | all.txt | **all.txt** |

Both end in the file. `wc -l all.txt` gives 239 lines (238 + the error), and `grep -c "No such file" all.txt` gives 1: the error is inside.

**Second version: `2>&1 > all.txt`**

| Step | Channel 1 goes to | Channel 2 goes to |
|---|---|---|
| start | screen | screen |
| `2>&1` (copy where 1 goes now) | screen | **screen** |
| `> all.txt` | **all.txt** | screen |

Channel 2 copied "screen" and was never moved again. The error appeared on the screen, and the file has 238 lines and 0 error lines.

Why did `tail -2 all.txt` not show the error in the first version? `tail -2` prints the last two lines, and they are `xml` and `zsh_command_not_found`, the last two entries of `/etc`. `ls` complains about `/nope` first and lists `/etc` after, so the error is the first line of the file, not the last. This is why the picture uses `grep -c` as the proof.

### 5.4 Adding to a file, and reading from a file

```bash
echo first > notes.md ; echo second >> notes.md ; cat notes.md
wc -l < access.log
wc -l access.log
```

![Figure 3.5 – Appending and input redirection](screenshots/fig-3-5.png)

- `echo first > notes.md` creates the file with one line.
- `echo second >> notes.md` adds a second line. With `>` here, the first line would have been erased.
- `cat notes.md` shows both lines.

The two `wc` lines count the same file and print different things:

| Line | Who opens the file | What `wc` knows | Prints |
|---|---|---|---|
| `wc -l access.log` | `wc`. The name is its argument | the name | `500 access.log` |
| `wc -l < access.log` | bash. It connects the file to channel 0 | nothing: it only sees text arriving on its input | `500` |

This difference is used on line 8 of `report.sh`, where only the number is wanted.

### 5.5 Chaining commands

```bash
ls /nope 2> /dev/null || echo "failed with code $?"
mkdir -p tmp && cd tmp && echo "I am in $(pwd)" ; cd ~/os-lab3
```

![Figure 3.3 – The operators](screenshots/fig-3-3.png)

| Written as | The second command runs |
|---|---|
| `a ; b` | always |
| `a && b` | only if `a` worked (exit code 0) |
| `a \|\| b` | only if `a` failed (exit code not 0) |

**First line, step by step:**

1. `ls /nope` fails, because the folder does not exist. Its exit code is 2.
2. `2> /dev/null` sends its error message to `/dev/null`, a special file that throws away everything written to it. (Windows calls it `NUL`.) So no error is seen.
3. `||` sees a failure and runs the `echo`.
4. Inside the double quotes, `$?` is replaced with the exit code of `ls`: `failed with code 2`.

**Second line, step by step:**

1. `mkdir -p tmp` makes the folder. It worked, so:
2. `cd tmp` moves into it. It worked, so:
3. `echo "I am in $(pwd)"` runs. `pwd` prints the current folder, and bash pastes it: `I am in /home/vboxuser/os-lab3/tmp`.
4. `; cd ~/os-lab3` runs in any case and brings me back. The extra `pwd` in the picture confirms it.

The `&&` chain is a safety belt: if `mkdir` had failed, `cd` would not have run, and nothing after it either.

### 5.6 Background jobs

```bash
sleep 100 &
jobs ; kill %1 ; jobs
```

![Figure 3.4 – A background job](screenshots/fig-3-4.png)

`sleep 100` does nothing for 100 seconds. Normally the terminal would be blocked for that time. The `&` at the end starts it in the background and gives the prompt back at once.

| Line on the screen | Meaning |
|---|---|
| `[1] 6898` | bash started job number 1. Its process number is 6898 |
| `[1]+  Running   sleep 100 &` | the first `jobs`: job 1 is running |
| `[1]+  Running   sleep 100 &` | the second `jobs`, a moment after `kill %1`. Bash has sent the stop signal but has not yet noticed that the process ended |
| `[1]+  Terminated   sleep 100` | bash reports the end of the job, just before it shows the next prompt |
| (nothing) | the last `jobs`: the list is empty |

`kill %1` asks job 1 to stop. `%1` is the job number of this terminal. Without it I would have to write the process number: `kill 6898`.

## 6. Part 4: one-liners

### 6.1 The tools

Each tool reads lines of text, does one small job, and prints lines of text. That is why they can be joined with pipes.

| Tool | Job | Small example |
|---|---|---|
| `grep WORD file` | keep only the lines that contain WORD | `grep FAIL access.log` keeps the 100 failed lines |
| `grep -c WORD file` | print how many lines contain WORD | `grep -c FAIL access.log` prints `100` |
| `cut -d' ' -f3` | keep only field 3 of each line. `-d' '` says "the fields are separated by a space" | `2026-10-05 10:05 user5 FAIL /page1` becomes `user5` |
| `sort` | put the lines in order | |
| `sort -u` | put in order and keep one copy of each line | |
| `sort -n` | sort as numbers | |
| `sort -r` | biggest first | |
| `uniq -c` | join equal lines that are next to each other, and write how many there were | three lines `user0` become `3 user0` |
| `wc -l` | count lines | |
| `head -3` | the first 3 lines | |
| `tail -3` | the last 3 lines | |
| `nl` | put a number in front of every line | |

**Why `sort` always comes before `uniq -c`.** `uniq` only joins equal lines that touch each other. In the log the users are mixed, so without `sort` nothing is joined:

```
grep FAIL access.log | cut -d' ' -f3 | uniq -c     grep FAIL access.log | cut -d' ' -f3 | sort | uniq -c

      1 user5                                           14 user0
      1 user3                                           14 user1
      1 user1                                           14 user2
      1 user6                                           15 user3
      ... 100 lines like this                           ... 7 lines
```

**Why `sort -rn` and not `sort -r`.** Without `-n` the lines are compared as text, character by character, and `9` would come before `15` because the character `9` is bigger than the character `1`. With `-n` they are compared as numbers.

### 6.2 The pattern from the sheet

```bash
grep OK access.log | cut -d' ' -f3 | sort | uniq -c | sort -rn
```

![Figure 4.12 – The starting pattern](screenshots/fig-4-12.png)

Read from left to right: keep the successful lines, keep only the user, put equal users together, count each one, biggest count first. Every question below is a variation of this line.

Check: 58 + 58 + 57 + 57 + 57 + 57 + 56 = 400, and 400 successful lines + 100 failed lines = 500.

### 6.3 Q1. How many requests failed?

```bash
grep -c FAIL access.log
```

![Figure 4.1](screenshots/fig-4-1.png)

| Piece | What it does |
|---|---|
| `grep` | look for lines that contain a text |
| `-c` | do not print the lines, print how many there are |
| `FAIL` | the text to look for |
| `access.log` | the file to look in |

**Answer: 100.** The same as the paper answer in 2.2. `grep FAIL access.log | wc -l` gives the same number in two steps.

### 6.4 Q2. How many different pages, and which?

```bash
cut -d' ' -f5 access.log | sort -u | nl
```

![Figure 4.2](screenshots/fig-4-2.png)

What flows through the pipe after each step:

```
cut -d' ' -f5 access.log        | sort -u            | nl

/page1                            /page0                  1  /page0
/page2                            /page1                  2  /page1
/page3                            /page2                  3  /page2
/page0                            /page3                  4  /page3
/page1
... 500 lines                     4 lines                 4 numbered lines
```

- `cut` keeps the fifth field, the page: 500 lines, mixed.
- `sort -u` leaves one line for each different page.
- `nl` numbers the lines, so the last number is the count and the lines themselves are the names. One command answers both halves of the question.

**Answer: 4 pages, `/page0` to `/page3`.**

### 6.5 Q3. How many requests did each page get?

```bash
cut -d' ' -f5 access.log | sort | uniq -c
```

![Figure 4.3](screenshots/fig-4-3.png)

The same start as Q2, but `sort | uniq -c` counts the copies before joining them, where `sort -u` only threw them away.

**Answer: 125 each.** Check asked by the sheet: 4 × 125 = 500, the number of lines in the log.

### 6.6 Q4. Which users have the most failed requests?

```bash
grep FAIL access.log | cut -d' ' -f3 | sort | uniq -c | sort -rn | head -3
```

![Figure 4.4](screenshots/fig-4-4.png)

This is the longest pipeline of the lab. What each step gives to the next:

| Step | Command | What comes out | Lines |
|---|---|---|---|
| 1 | `grep FAIL access.log` | only the failed lines: `2026-10-05 10:05 user5 FAIL /page1`, `2026-10-05 10:10 user3 FAIL /page2`, ... | 100 |
| 2 | `cut -d' ' -f3` | only the user of each: `user5`, `user3`, `user1`, `user6`, ... | 100 |
| 3 | `sort` | equal users are now together: `user0`, `user0`, ... `user6` | 100 |
| 4 | `uniq -c` | one line per user, with its count: `14 user0`, `14 user1`, `14 user2`, `15 user3`, `14 user4`, `15 user5`, `14 user6` | 7 |
| 5 | `sort -rn` | the biggest count first: `15 user5`, `15 user3`, `14 user6`, ... | 7 |
| 6 | `head -3` | the first three | 3 |

I print three lines, not one, because there is a tie at the top. One line would have hidden `user3`.

**Answer: `user3` and `user5`, 15 failed requests each.**

The second command in the picture stops after step 4 and shows all seven counts. They add up to 14 × 5 + 15 × 2 = 100, the answer of Q1. This is the second check asked by the sheet, and it is also the paper answer from 2.2.

### 6.7 Q5. How many requests did user3 make, and how many failed?

```bash
echo "$(grep -c ' user3 ' access.log) requests, $(grep ' user3 ' access.log | grep -c FAIL) failed"
```

![Figure 4.5](screenshots/fig-4-5.png)

Two numbers are needed, and a pipe can carry only one stream. So each number is computed inside its own `$( )`, and `echo` puts them into one sentence. Bash works from the inside out:

```
$(grep -c ' user3 ' access.log)                  →  72
$(grep ' user3 ' access.log | grep -c FAIL)      →  15

echo "72 requests, 15 failed"
```

- The first `$( )` counts the lines of `user3`.
- The second keeps the lines of `user3` and counts those among them that contain `FAIL`. Two `grep` in a row mean "this **and** that".
- The spaces inside `' user3 '` ask for the whole field. In this log it changes nothing, but in a log that also had a `user30`, the text `user3` alone would match both.

**Answer: 72 requests, 15 failed.** Both agree with the paper answers in 2.2 (72 lines for `user3`) and with Q4 (15 failures).

### 6.8 Q6. The last 3 failed requests, only time and user

```bash
grep FAIL access.log | tail -3 | cut -d' ' -f2,3
```

![Figure 4.6](screenshots/fig-4-6.png)

```
grep FAIL access.log | tail -3              | cut -d' ' -f2,3

2026-10-05 10:10 user0 FAIL /page2            10:10 user0
2026-10-05 10:15 user5 FAIL /page3            10:15 user5
2026-10-05 10:20 user3 FAIL /page0            10:20 user3
```

- `tail -3` keeps the last three of the 100 failed lines. They are lines 490, 495 and 500 of the log.
- `-f2,3` keeps two fields, the second and the third. `cut` puts the same separator, a space, between them.

**Answer: `10:10 user0`, `10:15 user5`, `10:20 user3`.**

### 6.9 Q7. The login shells in /etc/passwd

```bash
cut -d: -f7 /etc/passwd | sort | uniq -c | sort -rn
```

![Figure 4.7](screenshots/fig-4-7.png)

`/etc/passwd` is the list of accounts of the computer: one line per account, seven fields, with `:` between them. For example the line of the administrator:

```
root : x : 0 : 0 : root : /root : /bin/bash
 1     2   3   4    5      6        7
name  (password is kept elsewhere)  user number  group number  description  home folder  login shell
```

Field 7 is the program that starts when that account logs in. The command is the pattern from 6.2 with two changes: the separator is `:` (`-d:`), and the field is 7. No quotes are needed around `:`, because a colon means nothing special to bash. A space does, which is why the log needed `-d' '`.

**Answer: `/usr/sbin/nologin` 40, `/bin/false` 5, `/bin/bash` 2, `/bin/sync` 1.** That is 48 accounts. Most of them belong to system services, not to people, so their "shell" is a program that refuses the login. The two accounts with `bash` are `root` and `vboxuser`.

### 6.10 Q8. Why do the two counts differ by one?

```bash
ls /etc | wc -l ; ls -l /etc | wc -l ; ls -l /etc | head -1
```

![Figure 4.8](screenshots/fig-4-8.png)

- `ls /etc | wc -l` prints **237**. When the output of `ls` goes into a pipe and not onto the screen, `ls` writes one name per line, so counting lines counts entries.
- `ls -l /etc | wc -l` prints **238**.
- `ls -l /etc | head -1` shows the first line of the long format: `total 1160`.

The long format starts with one line that is not a file: `total`, the disk space used by the entries. One extra line, one extra count. (The picture uses `head -3` to show that line together with the first two real entries.)

### 6.11 Bonus. Failed requests in a minute that ends in 0

```bash
grep -E ':[0-5]0 user[0-6] FAIL' access.log | wc -l
```

![Figure 4.10](screenshots/fig-4-10.png)

The text between the quotes is a pattern, read piece by piece against a line such as `2026-10-05 10:10 user3 FAIL /page2`:

| Piece | Matches | In the example |
|---|---|---|
| `:` | a colon | the `:` of the time |
| `[0-5]` | one character from 0 to 5: the tens of the minute | `1` |
| `0` | the character 0: the minute ends in 0 | `0` |
| (space) | a space | the space after the time |
| `user` | these four letters | `user` |
| `[0-6]` | one character from 0 to 6 | `3` |
| (space) | a space | |
| `FAIL` | these four letters | `FAIL` |

- `[0-5]` is the same idea as `[a-b]` in Part 2, but here it is `grep` that reads it, not bash. The single quotes are what stop bash from touching it.
- `-E` switches on the extended pattern language of `grep`. The sheet asks for it.
- `wc -l` counts the matching lines.

**Answer: 50.** Check on paper: the failing lines are `i` = 5, 10, 15, 20, ... so their minutes end in 5, 0, 5, 0, ... Exactly half of the 100 end in 0.

### 6.12 The file oneliners.sh

The eight commands and the bonus are saved in [`oneliners.sh`](oneliners.sh), each under a comment with its question. A line that starts with `#` is a comment: bash skips it.

The file was written in `nano`, a simple editor that works inside the terminal. The bottom of its window lists the keys: `^O` (Ctrl+O) saves, `^X` (Ctrl+X) leaves.

![Figure 4.11 – oneliners.sh in the editor](screenshots/fig-4-11.png)

```bash
bash oneliners.sh
```

![Figure 4.9 – Full run of oneliners.sh](screenshots/fig-4-9.png)

Bash reads the file from top to bottom and runs each line, so the answers come out one under the other. Which lines of the picture belong to which question:

| Lines on the screen | Question |
|---|---|
| `100` | Q1 |
| four numbered pages | Q2 |
| four counts of 125 | Q3 |
| `15 user5`, `15 user3`, `14 user6` | Q4 |
| `72 requests, 15 failed` | Q5 |
| three lines with time and user | Q6 |
| four shells with their counts | Q7 |
| `237`, `238`, `total 1160` | Q8 |
| `50` | bonus |

Two things about how it runs:

- It must be started inside `~/os-lab3`. The commands say `access.log` with no folder in front, and such a name is looked for in the current folder.
- It needs no `chmod +x` and no first line `#!/bin/bash`, because I do not run the file itself. I run `bash` and hand it the file to read.

## 7. Part 5: the script report.sh

A script is a text file with commands. [`report.sh`](report.sh) turns the one-liners into a small program that takes the name of the log as an argument.

![Figure 5.1 – Contents of report.sh](screenshots/fig-5-1.png)

`cat -n` prints a file with line numbers. The same numbers are used below.

### 7.1 Every line

```bash
#!/bin/bash
```

**Line 1.** The first two characters, `#!`, are a message for Linux itself: "to run this file, start the program whose path follows, and give it this file". So `./report.sh access.log` really becomes `/bin/bash ./report.sh access.log`. It must be the very first line. For bash itself the line starts with `#`, so it is a comment and is skipped.

```bash
# usage: ./report.sh access.log
```

**Line 2.** A comment for people: how the script is meant to be called.

```bash
log=$1
```

**Line 3.** `$1` is the first word written after the name of the script. For `./report.sh access.log`, `$1` is `access.log`. The line stores it in a variable named `log`. There must be no space around `=`: with spaces, bash would look for a command named `log`.

```bash
if [ ! -f "$log" ]; then
```

**Line 4.** `if COMMAND; then ... fi` runs the lines up to `fi` only when COMMAND succeeds. The command here is `[`, the test command.

| Piece | Meaning |
|---|---|
| `[` ... `]` | test what is between the brackets. A space is needed after `[` and before `]` |
| `-f NAME` | true when NAME exists and is an ordinary file |
| `!` | turns the answer around: true when it is **not** a file |
| `"$log"` | the value of the variable, kept as one word by the quotes |

So the line reads: "if there is no file with this name, then".

```bash
  echo "no such file: $log" >&2
  exit 1
fi
```

**Lines 5, 6, 7.** They run only when the file is missing.

- Line 5 prints a message. `>&2` sends it to channel 2, the error channel, so it does not mix with the real output of the script.
- Line 6 stops the script at once with exit code 1. Whoever called the script can see that it failed.
- Line 7, `fi`, closes the `if`. It is `if` written backwards.

```bash
total=$(wc -l < "$log")
```

**Line 8.** Count the lines of the log and keep the number in `total`. The `<` is the trick from 5.4: with it `wc` prints only `500`. Without it the variable would hold `500 access.log`.

```bash
fails=$(grep -c FAIL "$log")
```

**Line 9.** The one-liner of Q1, with the file name coming from the variable. `fails` becomes `100`.

```bash
echo "$total requests, $fails failed"
```

**Line 10.** Print the summary. Inside double quotes the variables are replaced: `500 requests, 100 failed`.

```bash
# failures of each user, from user0 to user6
for n in {0..6}; do
  echo "user$n: $(grep " user$n " "$log" | grep -c FAIL) failures"
done
```

**Lines 11 to 14.** This is the part I wrote, in place of the `TODO` of the sheet.

- Line 11 is a comment.
- Line 12: `{0..6}` is brace expansion (4.6), so the line is the same as `for n in 0 1 2 3 4 5 6; do`. The loop runs seven times, and `n` is one of the numbers each time.
- Line 13 does the work. It is the one-liner of Q5 with the number of the user replaced by `$n`.
- Line 14, `done`, closes the loop.

Line 13 from the inside out, for `n = 3`:

```
" user$n "                              →  " user3 "
grep " user3 " "$log"                   →  the 72 lines of user3
... | grep -c FAIL                      →  15
echo "user3: 15 failures"
```

The quotes inside `$( )` do not end the outer string: inside `$( )` bash starts counting quotes again.

### 7.2 Following one run

`./report.sh access.log`, as bash sees it:

| Line | What happens | Values |
|---|---|---|
| 3 | `log` is set | `log` = `access.log` |
| 4 | is `access.log` **not** a file? No, it exists. Lines 5 and 6 are skipped | |
| 8 | `wc -l < access.log` | `total` = `500` |
| 9 | `grep -c FAIL access.log` | `fails` = `100` |
| 10 | prints `500 requests, 100 failed` | |
| 12-14 | seven turns, each prints one line | `user0: 14 failures` ... `user6: 14 failures` |
| end | no more lines. The script ends with the exit code of its last command | 0 |

`./report.sh nope.log`:

| Line | What happens |
|---|---|
| 3 | `log` = `nope.log` |
| 4 | is `nope.log` **not** a file? Yes |
| 5 | prints `no such file: nope.log` on channel 2 |
| 6 | the script stops with exit code 1. Lines 8 to 14 never run |

Why the quotes in `"$log"` matter: if I run `./report.sh` with no argument at all, `log` is empty. With the quotes, the test is `[ ! -f "" ]`, an empty name is not a file, and the script correctly prints `no such file:` and stops. Without the quotes the empty value would vanish from the line, the test would become `[ ! -f ]`, which means something else, and the script would go on without a file.

### 7.3 Running it

A new file is not allowed to run as a program:

![Figure 5.4 – Before chmod +x](screenshots/fig-5-4.png)

The first column of `ls -l` is the kind of the entry and its permissions:

```
-  rw-  rw-  r--
│   │    │    └── everybody else: read
│   │    └─────── the group vboxuser: read, write
│   └──────────── the owner vboxuser: read, write
└──────────────── kind: - is a file, d is a folder
```

`r` = may read, `w` = may change, `x` = may run as a program. There is no `x` anywhere, so bash answers `Permission denied`. In Windows a file is a program because its name ends in `.exe` or `.bat`. In Linux the name does not matter (`.sh` is only a hint for people): what matters is the `x` permission.

```bash
chmod +x report.sh
./report.sh access.log
```

![Figure 5.2 – Running the script](screenshots/fig-5-2.png)

`chmod` changes permissions, and `+x` adds "may run". `ls -l` now shows `-rwxrwxr-x`, and the terminal prints the name in green, its colour for programs. The output agrees with Part 4: 500 requests, 100 failed, 15 failures for `user3` and `user5`, and 14 × 5 + 15 × 2 = 100.

The three ways to start the script, side by side:

| Typed | Works? | Why |
|---|---|---|
| `bash report.sh access.log` | yes, even without `chmod +x` | I run `bash`, and the script is only a file it reads |
| `./report.sh access.log` | yes, after `chmod +x` | I run the file itself. Linux reads line 1 to learn that bash must read it |
| `report.sh access.log` | no: `command not found` | a name without `/` is searched only in the folders of `PATH` |

![Figure 5.5 – Without ./](screenshots/fig-5-5.png)

- `report.sh access.log` fails, because `~/os-lab3` is not one of the `PATH` folders.
- The second command proves it. It is the line from 3.4 with a `grep -c` at the end: among the folders of `PATH`, 0 contain the text `os-lab3`.
- `./report.sh` has a `/` in it, so bash does not search at all. `.` is the current folder, so `./report.sh` means "the file `report.sh` that is right here".

The current folder is left out of `PATH` on purpose. If it were in, a file named `ls` inside a folder I downloaded could run in place of the real `ls`.

### 7.4 Error handling

```bash
./report.sh nope.log ; echo "exit code: $?"
./report.sh nope.log 2> /dev/null ; echo "exit code: $?"
```

![Figure 5.3 – Error handling](screenshots/fig-5-3.png)

- The first line shows the message and exit code 1.
- In the second line, `2> /dev/null` throws away channel 2, and the message disappears. This proves that `>&2` on line 5 really sent the message to the error channel. The exit code is still 1.

Why a script should behave like this:

- Errors on channel 2 do not get mixed into the result. `./report.sh access.log > result.txt` saves only real output, and an error would still be seen on the screen.
- The exit code lets other commands react: `./report.sh nope.log || echo "report failed"`.

### 7.5 Bonus: cron

`cron` is a service that runs commands at set times. It is the Linux relative of the Windows Task Scheduler. Every user has a table of jobs, the crontab.

| Command | Does |
|---|---|
| `crontab -e` | opens my table in an editor |
| `crontab -l` | prints my table |
| `crontab -r` | deletes my table |

I added one line at the bottom:

```
* * * * * /home/vboxuser/os-lab3/report.sh /home/vboxuser/os-lab3/access.log >> /home/vboxuser/os-lab3/report.log 2>&1
```

![Figure 5.6 – The line added with crontab -e](screenshots/fig-5-6.png)

| Piece | Meaning |
|---|---|
| `* * * * *` | five time fields: minute, hour, day of the month, month, day of the week. `*` means "every", so this is every minute of every day |
| `/home/vboxuser/os-lab3/report.sh` | the program, with its full path. cron does not start in my folder and does not know `~/os-lab3` |
| `/home/vboxuser/os-lab3/access.log` | the argument of the script, `$1`, also with its full path |
| `>> .../report.log` | add the output to the end of the file. cron has no screen to print on |
| `2>&1` | send the errors to the same file. It is written after `>>`, the right order from 5.3 |

For comparison, the example in the comments of the picture starts with `0 5 * * 1`: minute 0, hour 5, any day of the month, any month, day of the week 1. That is 05:00 every Monday.

![Figure 5.7 – report.log after three minutes](screenshots/fig-5-7.png)

Reading the picture from the top:

1. `crontab -l | tail -1` prints the last line of my table: the job is there.
2. The two `date` commands show the time at the start, 14:48:52, and after waiting, 14:51:08.
3. Between them cron fired at 14:49:00, 14:50:00 and 14:51:00, three times. `cat report.log` shows three copies of the output, 8 lines each.
4. `crontab -r` deletes the table, and `crontab -l` confirms it: `no crontab for vboxuser`. Without this the script would keep running every minute.

## 8. Every symbol on one page

| Symbol | Name | What it does | First seen |
|---|---|---|---|
| `\|` | pipe | the output of the left command becomes the input of the right one | 3.4 |
| `>` | redirect output | channel 1 into a file, which is emptied first | 2.2 |
| `>>` | append | channel 1 added to the end of a file | 5.4 |
| `<` | redirect input | channel 0 from a file | 5.4 |
| `2>` | redirect errors | channel 2 into a file | 5.2 |
| `2>&1` | | channel 2 to where channel 1 goes now | 5.3 |
| `>&2` | | channel 1 to where channel 2 goes now | 7.1 |
| `;` | sequence | run the next command in any case | 2.2 |
| `&&` | and | run the next command only on success | 2.2 |
| `\|\|` | or | run the next command only on failure | 5.5 |
| `&` | background | start the command and give the prompt back | 5.6 |
| `*` | | any text in a file name | 4.3 |
| `?` | | one character in a file name | 4.3 |
| `[a-b]` | | one character from a list | 4.3 |
| `{1..5}` | braces | write out `1 2 3 4 5` | 4.6 |
| `$NAME` | variable | the value stored under that name | 3.4 |
| `$( )` | command substitution | run a command and paste its output here | 2.2 |
| `$(( ))` | arithmetic | calculate and paste the result here | 2.2 |
| `' '` | single quotes | everything inside stays as typed | 4.5 |
| `" "` | double quotes | spaces and `*` are protected, `$` still works | 4.5 |
| `\` | backslash | the next character is plain | 4.4 |
| `#` | comment | the rest of the line is ignored | 6.12 |
| `#!` | shebang | first line of a script: which program must read it | 7.1 |
| `~` | | my home folder | 1 |
| `.` and `..` | | this folder, the folder above | 1 |
| `$?` | | exit code of the last command | 3.5 |
| `$1` | | first argument of a script | 7.1 |
| `%1` | | job number 1 of this terminal | 5.6 |
| `/dev/null` | | a file that throws away what is written to it | 5.5 |

## 9. Test yourself

Try to answer before opening each one.

<details>
<summary>1. I type <code>echo *.txt</code> in a folder with <code>a.txt</code> and <code>b.txt</code>. How many arguments does echo receive?</summary>

Two: `a.txt` and `b.txt`. Bash replaced the pattern before it started `echo` (4.3).
</details>

<details>
<summary>2. And in a folder with no <code>.txt</code> file at all?</summary>

One: the text `*.txt` itself. When nothing matches, bash leaves the pattern as it is (4.3).
</details>

<details>
<summary>3. Why can <code>cd</code> not be a normal program?</summary>

Each process has its own current folder. A separate `cd` would change the folder of its own process and then exit. The shell would not have moved (3.3).
</details>

<details>
<summary>4. What is the difference between <code>'$HOME'</code> and <code>"$HOME"</code>?</summary>

Single quotes keep the text as typed, so `echo` gets `$HOME`. Double quotes still replace the variable, so `echo` gets `/home/vboxuser` (4.5).
</details>

<details>
<summary>5. <code>ls /etc /nope 2&gt;&amp;1 &gt; all.txt</code>: where does the error go?</summary>

To the screen. `2>&1` is read first and copies the place where channel 1 goes at that moment, which is the screen. Only after that is channel 1 moved to the file (5.3).
</details>

<details>
<summary>6. Why does <code>wc -l &lt; access.log</code> print only <code>500</code>?</summary>

Bash opens the file and connects it to the input of `wc`. `wc` never learns the name, so it has no name to print (5.4).
</details>

<details>
<summary>7. What do <code>;</code>, <code>&amp;&amp;</code> and <code>||</code> do?</summary>

`a ; b` runs `b` always. `a && b` runs `b` only if `a` worked. `a || b` runs `b` only if `a` failed (5.5).
</details>

<details>
<summary>8. Why is there always a <code>sort</code> before <code>uniq -c</code>?</summary>

`uniq` only joins equal lines that are next to each other. `sort` brings the equal lines together first (6.1).
</details>

<details>
<summary>9. In Q4, why <code>head -3</code> and not <code>head -1</code>?</summary>

Two users share the first place with 15 failures. One line would show only one of them (6.6).
</details>

<details>
<summary>10. How can the answers be checked without trusting the pipeline?</summary>

The Q3 counts must add up to 500, the number of lines. The failures per user must add up to the answer of Q1, 100. And because the log is made by arithmetic, each answer can be worked out on paper (2.2).
</details>

<details>
<summary>11. What does the first line <code>#!/bin/bash</code> do?</summary>

It tells Linux which program must read the file when the file itself is run with `./report.sh` (7.1).
</details>

<details>
<summary>12. Why <code>./report.sh</code> and not <code>report.sh</code>?</summary>

A name without `/` is searched only in the folders of `PATH`, and the current folder is not one of them. `./` gives the place directly (7.3).
</details>

<details>
<summary>13. Why does the script use <code>&gt;&amp;2</code> and <code>exit 1</code>?</summary>

The error goes to channel 2 so that it never mixes with the result, and the exit code 1 tells whoever called the script that it failed (7.4).
</details>

<details>
<summary>14. What does <code>* * * * *</code> mean in a crontab line?</summary>

Every minute of every hour of every day. The five fields are minute, hour, day of the month, month and day of the week (7.5).
</details>
