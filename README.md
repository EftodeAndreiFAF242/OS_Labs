# Operating Systems – laboratory works

- **Student:** Eftode Andrei, group FAF-242
- **Course:** Operating Systems, FCIM / FAF, Technical University of Moldova, 2026-2027

Each lab has its own folder. Every folder holds the Word report, the files the lab asks for, and a `README.md` that explains what was done. From Lab 3 on, each lab also has a `full_explanations.md`, which takes every command and every line of code apart, piece by piece.

| Folder | Lab | Topic | What is inside |
|---|---|---|---|
| [`lab1-2/`](lab1-2/) | 1 and 2 | The OS as a resource manager (Linux); meet xv6 | One report for both labs, `report.md`, `notes.md`, `sleep.c`, `pingpong.c`, `Makefile` |
| [`lab3/`](lab3/) | 3 | The command line: shell, pipes and text filters | Report, `oneliners.sh`, `report.sh`, screenshots |
| [`lab4/`](lab4/) | 4 | The xv6 shell from the outside | Report, `sleep.c`, `args.c`, `args_linux.c`, `Makefile`, screenshots |
| [`lab5/`](lab5/) | 5 | The file system with your own hands (Linux) | Report, `report.md`, screenshots |
| [`lab6/`](lab6/) | 6 | Inside the xv6 file system | Report, `mystat.c`, `find.c`, `Makefile`, `notes.md`, screenshots |

The labs follow two tracks. Track A looks at a real operating system, Linux, from the outside (Labs 1, 3 and 5). Track B builds on a small teaching operating system, xv6 (Labs 2, 4 and 6).

## Where the work is done

- A Windows 11 laptop with a Linux virtual machine: **Ubuntu 26.04.1 LTS** in VirtualBox (user `vboxuser`, computer name `os`).
- The machine was prepared with the course script `os2026-setup.sh`, which installs the tools, downloads xv6 into `~/xv6-riscv`, builds it and boots it once. Its last line was **All set.** (see [`lab3/README.md`](lab3/README.md#preparing-the-laptop)).
- Tools: `riscv64-linux-gnu-gcc` 15.2.0, QEMU 10.2.1, GNU Make 4.4.1, git.
- xv6: revision `06aad25` of [mit-pdos/xv6-riscv](https://github.com/mit-pdos/xv6-riscv), the one the course uses, on a branch named `lab`. Labs 1-2 were done earlier on revision `9374395`, the last one before the system call `sleep()` was renamed to `pause()`.

## How a lab is added

```bash
git add lab7 README.md && git commit -m "Lab 7" && git push
```
