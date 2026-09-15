# CS:APP ch. 3 — resources

*tags: csapp, assembly, x86-64, c, learning*

Where to go beyond the [chapter 3 cheatsheet](../cheatsheets/csapp-ch3-x86-64.md)
and the [hands-on walkthrough](../guides/csapp-ch3-c-to-asm.md): the course
material the book comes from, places to practise, and the references worth
keeping a tab open on.

## The book's own material

- **[CS:APP site](https://csapp.cs.cmu.edu/)** — the authors' site for the
  book. The student area has the source code for every figure, the errata
  list (worth checking before you conclude you've misunderstood something),
  and the lab assignments.
- **[CS:APP labs](https://csapp.cs.cmu.edu/3e/labs.html)** — the assignments
  that go with the chapters. For ch. 3 the relevant ones are **Bomb Lab**
  (read disassembly to work out what input a binary expects) and **Attack
  Lab** (buffer overflows, code injection, return-oriented programming).
  Both are self-contained downloads that run on Linux.
- **[CMU 15-213](https://www.cs.cmu.edu/~213/)** — the course the book *is*.
  Lecture slides and the schedule are public; ch. 3 is the "Machine-Level
  Programming" block, usually four or five lectures.

## Practice

- **[Compiler Explorer](https://godbolt.org/)** — paste C, see the assembly
  instantly, side by side and colour-matched. Switch compilers and
  optimization levels from a dropdown. The single most useful thing for this
  chapter: every "what does this compile to?" question becomes a five-second
  experiment.
- **[pwn.college](https://pwn.college/)** — free, structured, hands-on
  courseware on assembly, reversing and exploitation, built on the same
  material at a deeper level.
- **[ROP Emporium](https://ropemporium.com/)** — return-oriented programming
  as a series of exercises, one technique at a time. Picks up exactly where
  the Attack Lab leaves off.
- The [runnable examples](../examples/csapp-ch3/index.md) in this wiki — 8 annotated
  C files and a `Makefile`, one per chapter topic.

## References

- **[x86 and amd64 instruction reference](https://www.felixcloutier.com/x86/)**
  — Intel's manual, scraped into fast, linkable HTML pages. Look up any
  mnemonic here rather than in the PDF.
- **[System V x86-64 psABI](https://gitlab.com/x86-psABIs/x86-64-ABI)** — the
  actual specification for the calling convention: argument registers,
  callee-saved registers, stack alignment, struct passing. The authoritative
  answer whenever the book's summary isn't specific enough.
- **[Intel SDM](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)**
  — the primary source, several thousand pages. Volume 2 is the instruction
  set reference.
- **[GNU assembler manual](https://sourceware.org/binutils/docs/as/)** — for
  the directives (`.quad`, `.align`, `.section`) that surround the
  instructions in `gcc -S` output.
- **[GDB documentation](https://sourceware.org/gdb/)** — and the
  [gdb & objdump sheet](../cheatsheets/gdb-objdump.md) here for the commands
  that matter in practice.
- **[Agner Fog's optimization manuals](https://www.agner.org/optimize/)** —
  instruction latencies and microarchitecture detail. Far past ch. 3, but the
  place to go when you start asking *why* one sequence is faster.

## Tools

| Tool | For |
|---|---|
| `gcc -Og -S` | your own code → assembly |
| `objdump -d` | any binary → assembly |
| `gdb` | watching it execute |
| [Compiler Explorer](https://godbolt.org/) | the same, in a browser, across compilers |
| [Ghidra](https://ghidra-sre.org/) | decompiling to pseudo-C; free, heavyweight |
| [radare2](https://github.com/radareorg/radare2) / [rizin](https://rizin.re/) | scriptable reverse-engineering from the terminal |

## Where the book shows its age

The book's listings were generated around 2014. Nothing in the model has
changed, but a current toolchain adds things the text never mentions — all of
these were checked against gcc 13.3 on x86-64 Linux:

| You'll see | The book shows | Why |
|---|---|---|
| `endbr64` opening most functions | nothing | Intel CET branch tracking (`-fcf-protection=none` to remove) |
| `name(%rip)` addressing | absolute addresses | PIE is the default (`-fno-pie -no-pie`) |
| Jump tables of `.long` offsets | `.quad` absolute addresses | PIE again |
| `subq %fs:40, %rax` in the canary check | `xorq %fs:40, %rax` | Both test the same thing |
| `call __strcpy_chk` | `call strcpy` | `_FORTIFY_SOURCE` is on by default |
| A canary in almost every function | canaries only when asked for | `-fstack-protector-strong` is on by default |

## Related

- [Chapter 3 cheatsheet](../cheatsheets/csapp-ch3-x86-64.md)
- [From C to assembly](../guides/csapp-ch3-c-to-asm.md)
- [gdb & objdump](../cheatsheets/gdb-objdump.md)
- [C Language bookmarks](index.md#c-language) — the wider pile
- [Reading list](../reading-list.md) — where the book itself is queued
