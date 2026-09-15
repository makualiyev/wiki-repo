# CS:APP ch. 3 — runnable examples

*tags: assembly, x86-64, c, csapp*

Eight annotated C files, one per chapter-3 topic, plus a `Makefile`. They exist
to be compiled and read rather than run — the output *is* the lesson. The
[C-to-assembly guide](../../guides/csapp-ch3-c-to-asm.md) walks through what
each one produces; the [cheatsheet](../../cheatsheets/csapp-ch3-x86-64.md) is
the reference for the instructions that come out.

## Getting the listings

```bash
cd docs/examples/csapp-ch3

make             # a .s listing for every example
make arith.s     # just one
make dis         # objdump disassembly of the compiled objects
make run         # build and run the examples that have a main()
make canary      # stack protector on vs off, diffed
make clean
```

The default flags are `-Og -std=c17 -Wall -Wextra
-fno-asynchronous-unwind-tables`: optimized enough that the output is
realistic, structured enough that you can still see your C in it, and without
the `.cfi_*` directives that bury a short function. Override with
`make CFLAGS=-O2` to see what actually ships.

## The files

| File | Shows |
|---|---|
| [`arith.c`](arith.c) | `lea` instead of multiply, the biased shift instead of divide, `cqto`/`idivq`, widening |
| [`control.c`](control.c) | `cmov` and the case where the compiler *must* keep a branch; the three loop shapes |
| [`switch.c`](switch.c) | dense cases → jump table, sparse cases → compare chain |
| [`proc.c`](proc.c) | arguments 7+ on the stack, callee-saved registers, recursion, locals that need an address |
| [`arrays.c`](arrays.c) | scale factors, row-major 2D indexing, VLA strides, pointer walking |
| [`structs.c`](structs.c) | padding and alignment — it prints its own layout, and the same fields reordered |
| [`fp.c`](fp.c) | `%xmm` registers, `addsd`, conversions, sign-bit negation, unsigned-style compares |
| [`overflow.c`](overflow.c) | the stack protector, with and without |
| [`Makefile`](Makefile) | the targets above |

## The one that runs

`structs.c` has a `main()` and prints what it measures:

```
$ make run
struct bad   size=24 align=8   a=0 b=4 c=8 d=16
struct good  size=16 align=8   d=0 b=8 a=12 c=13
union U      size= 8 align=8
```

Same four fields in both structs — declaring them largest-first saves a third
of the space, because the small fields fall into what would otherwise be
padding.

!!! warning
    `overflow.c` is compile-and-read material. The unsafe function is there to
    show what the stack protector is protecting, and the flags used to disable
    protections (`-fno-stack-protector`, `-fno-pie`) belong in a study
    directory, not in anything you ship.
