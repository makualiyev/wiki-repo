# gdb & objdump — reading compiled code

*tags: gdb, objdump, binutils, assembly, debugging*

The tools side of [CS:APP ch. 3](csapp-ch3-x86-64.md): how to get from a binary
to instructions you can read, and how to watch those instructions run. Every
command here was run against gcc 13 / binutils 2.42 / gdb 15 output.

## Which tool

| Question | Tool |
|---|---|
| What assembly does *my source* produce? | `gcc -S` |
| What instructions are in this binary? | `objdump -d` |
| What sections / symbols does it have? | `readelf`, `nm` |
| What is it doing *right now*? | `gdb` |
| Is it even an executable, and which kind? | `file`, `size` |

## Your own code — `gcc`

```bash
gcc -Og -S prog.c -o -          # assembly to stdout, readable optimization
gcc -Og -S -fverbose-asm prog.c # annotate each instruction with the C variable
gcc -Og -c prog.c               # object file, to disassemble instead
gcc -Og -g -o prog prog.c       # debug info, for gdb and objdump -S
```

`-fno-asynchronous-unwind-tables` strips the `.cfi_*` directives that otherwise
outnumber the instructions in a short function.

## Someone else's binary — `objdump`

```bash
objdump -d prog                      # disassemble executable sections
objdump -d --no-show-raw-insn prog   # drop the hex bytes, keep the mnemonics
objdump -d --disassemble=main prog   # just one function
objdump -M intel -d prog             # Intel syntax instead of AT&T
objdump -S prog                      # interleave the C source (needs -g)
objdump -t prog                      # symbol table
objdump -h prog                      # section headers and sizes
objdump -r prog.o                    # relocations (unlinked objects)
```

Narrow it to an address range when a function is huge:

```bash
objdump -d --start-address=0x1140 --stop-address=0x1200 prog
```

### Reading data, not code

Jump tables, string literals and constant arrays live in `.rodata`, and `-d`
won't show them:

```bash
objdump -s -j .rodata prog     # hex + ASCII dump of one section
readelf -x .rodata prog        # same idea, readelf's formatting
strings -t x prog              # printable strings with file offsets
```

!!! warning
    Dump `.rodata` from the **linked executable**, not from a `.o`. In an
    unlinked object the jump-table entries are still relocations, so the dump
    is all zeros. `objdump -r` shows the relocations that will fill them in.

## Shape of the file — `readelf`, `nm`, `size`

```bash
file prog                  # ELF? 64-bit? PIE? stripped? dynamically linked?
size prog                  # text / data / bss totals
readelf -h prog            # ELF header: type, machine, entry point
readelf -S prog            # section headers
readelf -s prog            # symbol table (-D for the dynamic one)
readelf -d prog            # dynamic section: shared library dependencies
nm prog                    # symbols; T = text, D = data, U = undefined
nm -C prog                 # demangle C++ names
```

!!! tip
    `file` telling you "pie executable" explains why every address in
    `objdump` output starts small (`0x1149`) while gdb shows
    `0x555555555149` — the loader picks a base at run time. Same code.

## gdb at the instruction level

```bash
gdb ./prog                 # then `run`, or `run arg1 arg2`
gdb --args ./prog a b      # arguments up front
gdb -q ./prog              # skip the banner
```

| Command | What it does |
|---|---|
| `disassemble` | current function |
| `disassemble main` | a named function |
| `disassemble /s main` | interleaved with source lines |
| `disassemble /r main` | with raw instruction bytes |
| `set disassembly-flavor intel` | Intel syntax |
| `x/5i $pc` | the next 5 instructions |
| `display/i $pc` | show the next instruction after *every* stop |

### Stopping and stepping

| Command | What it does |
|---|---|
| `break main`, `b func` | breakpoint on a function |
| `break *0x401136` | breakpoint on an exact instruction |
| `tbreak` | same, but one-shot |
| `starti` | stop at the very first instruction, before `_start` runs |
| `stepi` / `si` | one instruction |
| `nexti` / `ni` | one instruction, stepping *over* calls |
| `finish` | run to the end of this function, print the return value |
| `continue` / `c` | carry on |
| `watch *(long*)0x7fff…` | stop when a memory location changes |

### Registers and memory

```
info registers              # all of them
info registers rax rsp      # a few
p/x $rax                    # one, in hex
p $eflags                   # → [ ZF PF IF ] — the condition codes, decoded
p/d $rdi                    # decimal
p/t $rax                    # binary
```

The examine command is `x/NFU addr` — **N**umber of items, **F**ormat, **U**nit:

| | |
|---|---|
| Format | `x` hex, `d` signed, `u` unsigned, `t` binary, `c` char, `s` string, `i` instruction, `a` address, `f` float |
| Unit | `b` 1 byte, `h` 2, `w` 4, `g` 8 |

```
x/8xg $rsp        # 8 giant (8-byte) words of the stack, in hex
x/16xb $rdi       # 16 raw bytes at the first argument
x/s $rsi          # the second argument as a C string
x/4dw $rax        # 4 ints, decimal
x/3i $pc          # 3 instructions at the program counter
```

### Where am I

```
bt                 # backtrace
info frame         # frame boundaries, and where the saved rip lives
up / down          # move between frames
info args          # arguments (needs debug info)
info locals        # locals (needs debug info)
```

`info frame` is the one worth knowing by heart — it prints
`Saved registers: rip at 0x7fffffffcb88`, i.e. the exact address the return
address occupies. That's the address a stack overflow has to reach.

### TUI mode

```
layout asm         # split window: source/assembly pane plus the command line
layout regs        # add a live register pane
Ctrl-x a           # toggle TUI off and on again
Ctrl-l             # redraw when it gets corrupted
```

## Recipes

**What arguments is this call getting?** Break on the call, read the argument
registers before it executes:

```
(gdb) break *0x401136
(gdb) run
(gdb) info registers rdi rsi rdx
(gdb) x/s $rdi            # if the first argument is a string
```

**What is this comparison testing?** Break on the `cmp`, step once, read the
flags:

```
(gdb) si
(gdb) p $eflags           # [ ZF PF IF ] → ZF set, so the operands were equal
```

**Which way did the branch go?** `display/i $pc`, then `si` repeatedly — the
program counter tells you which target was taken without any guessing.

**Where does this jump table lead?** Find the table address in the disassembly,
then read the entries as addresses (or, on a PIE binary, as 32-bit offsets from
the table's own address):

```
(gdb) x/8a 0x402008       # absolute entries: -fno-pie builds
(gdb) x/8dw 0x402008      # 32-bit offsets: default PIE builds
```

**Watch the stack change across a call.** `x/8xg $rsp` before `call`, `si` into
it, then `x/8xg $rsp` again — the difference is the pushed return address.

## A small `.gdbinit`

Drop this in `~/.gdbinit` to land in a sensible setup every time:

```
set disassembly-flavor att
set print pretty on
set history save on
set confirm off
display/i $pc
```

## See also

- [CS:APP ch. 3 — x86-64 assembly](csapp-ch3-x86-64.md) — what the instructions
  mean once you can see them
- [From C to assembly](../guides/csapp-ch3-c-to-asm.md) — worked examples
- [CS:APP ch. 3 resources](../bookmarks/csapp-ch3.md)
