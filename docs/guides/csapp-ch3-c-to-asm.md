# From C to assembly — CS:APP ch. 3, hands on

*tags: assembly, x86-64, c, gcc, csapp*

The [ch. 3 cheatsheet](../cheatsheets/csapp-ch3-x86-64.md) lists the patterns.
This walks through them the other way round: take a C function, compile it, and
read what actually comes out. Every listing below is real `gcc` output, not a
tidied-up version — the point is that you can reproduce and poke at all of it.

The source files live in
[`docs/examples/csapp-ch3/`](../examples/csapp-ch3/index.md) with a `Makefile`:

```bash
cd docs/examples/csapp-ch3
make            # emit a .s listing for every example
make dis        # objdump disassembly instead
make run        # build and run the ones with a main()
make canary     # stack protector on vs off, diffed
```

## Setting the flags

Optimization level decides whether you're reading your program or the
compiler's cleverness.

| Flag | What it gives you |
|---|---|
| `-O0` | Every variable spilled to the stack. Faithful, extremely verbose, unlike anything that ships. |
| `-Og` | **Start here.** Optimized enough to be realistic, structured enough to still recognize your C. |
| `-O2` | What actually ships. Loops unrolled and vectorized, functions inlined away. |

Two flags that just remove noise:

```bash
gcc -Og -fno-asynchronous-unwind-tables -S prog.c   # drop the .cfi_* directives
gcc -Og -S prog.c -o -                              # listing straight to stdout
```

And the set that makes output look like the book's — no PIE, no CET, no canary:

```bash
gcc -Og -fno-pie -no-pie -fcf-protection=none \
    -fno-stack-protector -fno-asynchronous-unwind-tables -S prog.c
```

!!! note "endbr64"
    The first instruction of nearly every function below is `endbr64`. It's a
    landing pad for indirect branches (Intel CET), it does nothing to your
    logic, and the book predates it. Mentally skip it.

## Arithmetic: the compiler hates multiplying

`arith.c`. Three ways to multiply, none of them `imul`:

```c
long times3(long x)      { return x * 3; }
long times5plus7(long x) { return x * 5 + 7; }
long times12(long x)     { return x * 12; }
```

```asm
times3:                          times5plus7:
        leaq    (%rdi,%rdi,2), %rax      leaq    7(%rdi,%rdi,4), %rax
        ret                              ret

times12:
        leaq    (%rdi,%rdi,2), %rax      # x*3
        salq    $2, %rax                 # *4  → x*12
        ret
```

`leaq` does `base + index*scale + displacement` in one instruction and doesn't
touch the flags, so the compiler reaches for it constantly. Scale is limited to
1, 2, 4 or 8, which is why `x*12` needs the extra shift.

Division is the interesting one, because it's the one thing that can't be
faked:

```c
long quotient(long x, long y) { return x / y; }
```

```asm
quotient:
        movq    %rdi, %rax     # dividend must be in %rax
        cqto                   # sign-extend across %rdx:%rax
        idivq   %rsi           # quotient → %rax, remainder → %rdx
        ret
```

But with a *constant* divisor the compiler works around it — and the workaround
is not the obvious one:

```c
long divide_by_2(long x) { return x / 2; }
```

```asm
divide_by_2:
        movq    %rdi, %rax
        shrq    $63, %rax      # 1 if x is negative, else 0
        addq    %rdi, %rax     # bias the dividend
        sarq    %rax           # then shift
        ret
```

A plain `sar` rounds toward negative infinity; C requires rounding toward zero.
Adding 1 first — but only when `x` is negative — fixes the difference. That
`shr $63` is the giveaway.

!!! tip "Widening, and the missing `movzlq`"
    `long widen_signed(int x)` compiles to `movslq %edi, %rax`. The unsigned
    version compiles to plain `movl %edi, %eax` — writing a 32-bit register
    already zeroes the top half, so no zero-extending instruction is needed or
    exists.

## Control flow: when `cmov` is allowed

`control.c`. The textbook branchless ternary:

```c
long max(long x, long y) { return x > y ? x : y; }
```

```asm
max:
        cmpq    %rdi, %rsi
        movq    %rdi, %rax
        cmovge  %rsi, %rax     # no branch at all
        ret
```

Now the same shape, with one difference:

```c
long deref_or_zero(long *p) { return p ? *p : 0; }
```

```asm
deref_or_zero:
        testq   %rdi, %rdi
        je      .L5
        movq    (%rdi), %rax   # only reached when p != NULL
        ret
.L5:
        movl    $0, %eax
        ret
```

The branch stays. `cmov` computes **both** arms and then picks one, which is
fine for arithmetic and fatal for a dereference — the null load would happen
before the choice. This is the cheatsheet's caveat, visible in the output.

### The three loop shapes

```c
long sum_while(long *a, long n) {
    long s = 0, i = 0;
    while (i < n) { s += a[i]; i++; }
    return s;
}
```

```asm
sum_while:
        movl    $0, %eax       # i
        movl    $0, %edx       # s
        jmp     .L7            # ── jump straight to the test
.L8:
        addq    (%rdi,%rax,8), %rdx
        addq    $1, %rax
.L7:
        cmpq    %rsi, %rax
        jl      .L8            # ── and loop back if it holds
        movq    %rdx, %rax
        ret
```

That's "jump to middle": the guard runs once before the first iteration, by
jumping over the body to the test. `sum_for` compiles to the *identical*
listing — a `for` loop is a `while` loop with the update moved.

A `do`-`while` needs no guard, since the body is known to run at least once:

```asm
countdown:
        movl    $0, %eax
.L13:
        addq    %rdi, %rax
        subq    $1, %rdi
        testq   %rdi, %rdi
        jg      .L13           # test at the bottom, no jump-to-middle
        ret
```

## `switch`: two completely different shapes

`switch.c`. Seven dense cases become a jump table:

```asm
dense:
        cmpq    $6, %rdi
        ja      .L9                  # unsigned compare: catches negatives too
        leaq    .L4(%rip), %rdx
        movslq  (%rdx,%rdi,4), %rax  # table entry is a 32-bit offset
        addq    %rdx, %rax           # table address + offset
        notrack jmp *%rax

.L4:    .long   .L8-.L4              # case 0
        .long   .L9-.L4              # case 1 → default
        .long   .L5-.L4              # case 2 ─┐ both cases point
        .long   .L7-.L4              # case 3  │ at the same label
        .long   .L6-.L4              # case 4  │
        .long   .L5-.L4              # case 5 ─┘
        .long   .L3-.L4              # case 6
```

Seven cases, one bounds check, one indirect jump — the cost doesn't grow with
the number of cases. Note the table holds *offsets from its own address*
(`.long .L8-.L4`), not addresses; that's PIE. Compile with `-fno-pie -no-pie`
and you get the book's version exactly:

```asm
        cmpq    $6, %rdi
        ja      .L9
        jmp     *.L4(,%rdi,8)
.L4:    .quad   .L8
        .quad   .L9
```

Spread the same three cases far apart (`1`, `500`, `9000`) and the table
disappears — a table would need 9000 entries — leaving a compare chain:

```asm
sparse:
        cmpq    $500, %rdi
        je      .L12
        cmpq    $9000, %rdi
        je      .L13
        cmpq    $1, %rdi
        jne     .L14
```

## Procedures: arguments, saving, recursion

`proc.c`. Eight arguments, six of which fit in registers:

```asm
caller8:
        subq    $8, %rsp       # realign before pushing an odd number of args
        pushq   $8             # 8th argument
        pushq   $7             # 7th argument
        movl    $6, %r9d       # 6th ─┐
        movl    $5, %r8d       #      │ registers, in reverse order
        movl    $4, %ecx       #      │
        movl    $3, %edx       #      │
        movl    $2, %esi       #      │
        movl    $1, %edi       # 1st ─┘
        call    callee@PLT
        addq    $24, %rsp      # 8 + 8 + 8: both args and the realignment
        ret
```

Stack arguments are pushed in reverse, so argument 7 ends up at the lower
address — nearest `%rsp` when the callee starts.

A value that has to survive a call gets a callee-saved register, which means
the caller now has to save it:

```asm
keep_across_call:
        pushq   %rbx                   # preserve the caller's %rbx
        leaq    (%rdi,%rdi,2), %rbx    # saved = x*3, parked in %rbx
        ...
        call    callee@PLT
        addq    %rbx, %rax             # still intact after the call
        popq    %rbx                   # restore before returning
        ret
```

Recursion is the same rule applied to itself — `factorial` needs `n` after the
recursive call returns, so `n` goes in `%rbx`:

```asm
factorial:
        cmpq    $1, %rdi
        jg      .L12
        movl    $1, %eax       # base case, no frame needed at all
        ret
.L12:
        pushq   %rbx
        movq    %rdi, %rbx     # keep n
        leaq    -1(%rdi), %rdi # n-1
        call    factorial
        imulq   %rbx, %rax     # n * factorial(n-1)
        popq    %rbx
        ret
```

### Seeing the frame in gdb

The alignment rule is easy to check rather than take on faith:

```
(gdb) break main
(gdb) run
(gdb) info registers rsp
rsp   0x7fffffffcb88
(gdb) info frame
 rip = 0x555555555060 in main; saved rip = 0x7ffff7c2a1ca
 Previous frame's sp is 0x7fffffffcb90
 Saved registers:
  rip at 0x7fffffffcb88
```

`0x...88` ends in 8: on entry `%rsp ≡ 8 (mod 16)`, because the caller aligned
to 16 and then `call` pushed 8 bytes of return address. And gdb puts the saved
`rip` at exactly `%rsp` — the return address is the first thing above the
frame.

## Arrays: the scale factor is the element size

`arrays.c`. One instruction each, differing only in the scale:

```asm
elem_int:    movl    (%rdi,%rsi,4), %eax    # int:  scale 4
elem_long:   movq    (%rdi,%rsi,8), %rax    # long: scale 8
elem_char:   movzbl  (%rdi,%rsi), %eax      # char: scale 1, zero-extended
```

Two dimensions, fixed row length — `M[i][j]` in `int M[4][3]`:

```asm
elem2d:
        leaq    (%rsi,%rsi,2), %rax    # 3*i        (row length known: 3)
        leaq    (%rdi,%rax,4), %rax    # M + 12*i   (start of row i)
        movl    (%rax,%rdx,4), %eax    # + 4*j
        ret
```

Make the row length a runtime value and the multiply becomes real:

```asm
elem_vla:
        imulq   %rdi, %rdx             # n * i — an actual multiply
        leaq    (%rsi,%rdx,4), %rax
        movl    (%rax,%rcx,4), %eax
        ret
```

`lea` chains mean a compile-time-known row length; an `imul` in an indexing
sequence means a VLA or a pointer-to-pointer.

## Structs: measure, don't guess

`structs.c` prints its own layout, so there's nothing to take on trust:

```
struct bad   size=24 align=8   a=0 b=4 c=8 d=16
struct good  size=16 align=8   d=0 b=8 a=12 c=13
union U      size= 8 align=8
```

Same four fields both times:

```c
struct bad  { char a; int b; char c; long d; };   /* 24 bytes */
struct good { long d; int b; char a; char c; };   /* 16 bytes */
```

Declaration order is layout order, and every field must land on a multiple of
its own size — so `bad` spends 8 of its 24 bytes on padding. Sorting fields
largest-first packs the small ones into what would have been the gaps. A third
of the struct, recovered by retyping one line.

Field access itself is just a constant displacement:

```asm
node_val:     movq   (%rdi), %rax     # p->val,  offset 0
node_next:    movq   8(%rdi), %rax    # p->next, offset 8
```

## Floating point: a separate machine

`fp.c`. None of this touches the general-purpose registers:

```asm
fadd:    addss   %xmm1, %xmm0     # float
dadd:    addsd   %xmm1, %xmm0     # double
```

Conversions are explicit instructions, and the `t` in the int-ward direction
means *truncate*, which is what a C cast promises:

```asm
int2double:   pxor       %xmm0, %xmm0     # clear first, dependency break
              cvtsi2sdl  %edi, %xmm0
double2int:   cvttsd2sil %xmm0, %eax
```

Negation isn't arithmetic — it flips one bit:

```asm
negate:  xorpd   .LC0(%rip), %xmm0    # constant with only the sign bit set
```

And comparison uses the *unsigned* condition codes, which trips people up:

```asm
fcompare:                       # return x < y
        comisd  %xmm0, %xmm1
        seta    %al             # "above", not "greater"
        movzbl  %al, %eax
        ret
```

## The stack protector, on and off

`overflow.c`, compiled both ways — `make canary` diffs them for you. The added
lines:

```diff
  unsafe_copy:
        pushq   %rbx
        subq    $16, %rsp
+       movq    %fs:40, %rax      # canary from thread-local storage
+       movq    %rax, 8(%rsp)     # parked just below the return address
+       xorl    %eax, %eax        # don't leave a copy in a register
        ...
        call    puts@PLT
+       movq    8(%rsp), %rax     # read it back
+       subq    %fs:40, %rax      # unchanged?
+       jne     .L4
        addq    $16, %rsp
        popq    %rbx
        ret
+.L4:
+       call    __stack_chk_fail@PLT
```

An overflow big enough to reach the return address has to cross the canary
first, so the check fires before `ret` ever uses the corrupted value. It's a
detector, not a fix — the write already happened.

Worth noticing in the same listing: the `strcpy` in the source became a call to
`__strcpy_chk` with the buffer size 8 passed as an argument. That's
`_FORTIFY_SOURCE`, on by default on Ubuntu, adding a runtime length check
wherever the compiler can see how big the destination is.

!!! warning
    `-fno-stack-protector` and `-fno-pie` exist here to make listings readable
    and to reproduce the book's output. Don't carry them into anything you
    actually ship.

## Things to try

- Compile `sum_while` at `-O0`, `-Og`, `-O2` and `-O3` and diff them. At `-O3`
  the loop vectorizes into `%xmm` registers and stops looking like your code.
- Delete `volatile`-free dead stores and watch them vanish; add `volatile` and
  watch them come back.
- Write a `switch` and keep spreading the case values apart until the jump
  table turns into a compare chain. Where's the threshold?
- Reorder the fields of a struct you actually use at work and check `sizeof`
  before and after.
- Take any binary you have and run `objdump -d` on it — see the
  [gdb & objdump sheet](../cheatsheets/gdb-objdump.md) for where to start.

## See also

- [CS:APP ch. 3 — x86-64 assembly](../cheatsheets/csapp-ch3-x86-64.md) — the
  reference sheet for everything above
- [gdb & objdump](../cheatsheets/gdb-objdump.md) — inspecting compiled code
- [CS:APP ch. 3 resources](../bookmarks/csapp-ch3.md) — course material and labs

------------------------------------------------
[<- Table of Contents](../index.md)
