# CS:APP ch. 3 — x86-64 assembly

*tags: assembly, x86-64, c, reverse-engineering, csapp*

Study companion for *Computer Systems: A Programmer's Perspective* (Bryant &
O'Hallaron, 3rd ed.), chapter 3 — **Machine-Level Representation of Programs**.

Everything below is **AT&T syntax** — what `gcc -S` and `objdump -d` emit on
Linux — so operands read `OP src, dst` and the destination is on the right.

!!! note "What this is"
    A lookup sheet for reading disassembly: registers, instructions, control
    flow, procedures, data layout, buffer overflow. It's not a substitute for
    the text.

    Companions: the [C-to-assembly walkthrough](../guides/csapp-ch3-c-to-asm.md)
    compiles every pattern here on a real machine, the
    [gdb & objdump sheet](gdb-objdump.md) covers the tools, and
    [ch. 3 resources](../bookmarks/csapp-ch3.md) collects the course material.

!!! warning "AT&T vs Intel syntax"
    The book, `gcc -S` and `objdump -d` use **AT&T** syntax: `OP src, dst`,
    registers prefixed `%`, constants prefixed `$`. Intel manuals, `objdump
    -M intel`, most Windows tooling and most disassemblers use **Intel**
    syntax, where the operands are the other way round and there are no
    sigils — `mov rax, 5` means `movq $5, %rax`. Check which one you're
    looking at before you read a single instruction.

## Registers

16 general-purpose 64-bit registers. Each is also addressable at 32-, 16- and
8-bit widths.

### Argument passing (caller → callee)

| Register | Role | 32 / 16 / 8-bit names |
|---|---|---|
| `%rdi` | 1st argument | `%edi` `%di` `%dil` |
| `%rsi` | 2nd argument | `%esi` `%si` `%sil` |
| `%rdx` | 3rd argument | `%edx` `%dx` `%dl` |
| `%rcx` | 4th argument | `%ecx` `%cx` `%cl` |
| `%r8`  | 5th argument | `%r8d` `%r8w` `%r8b` |
| `%r9`  | 6th argument | `%r9d` `%r9w` `%r9b` |

### Return and special

| Register | Role | 32 / 16 / 8-bit names |
|---|---|---|
| `%rax` | return value | `%eax` `%ax` `%al` |
| `%rsp` | stack pointer | `%esp` `%sp` `%spl` |
| `%rbp` | frame pointer (callee-saved) | `%ebp` `%bp` `%bpl` |

### Callee-saved — the callee must restore them

| Register | 32 / 16 / 8-bit names |
|---|---|
| `%rbx` | `%ebx` `%bx` `%bl` |
| `%r12` | `%r12d` `%r12w` `%r12b` |
| `%r13` | `%r13d` `%r13w` `%r13b` |
| `%r14` | `%r14d` `%r14w` `%r14b` |
| `%r15` | `%r15d` `%r15w` `%r15b` |

### Caller-saved — scratch across a call

| Register | 32 / 16 / 8-bit names |
|---|---|
| `%r10` | `%r10d` `%r10w` `%r10b` |
| `%r11` | `%r11d` `%r11w` `%r11b` |

`%rax` and the six argument registers are caller-saved too: anything that isn't
`%rbx`, `%rbp` or `%r12`–`%r15` can come back from a `call` clobbered.

### Register widths

```text
┌──────────────────────────────── %rax (64-bit) ─┐
│                  ┌───────────── %eax (32-bit) ─┤
│                  │       ┌───── %ax  (16-bit) ─┤
│                  │       │     ┌─ %al   (8-bit)┤
│ 63            32 │ 31  16│15  8│ 7           0 │
└──────────────────┴───────┴─────┴───────────────┘
```

!!! tip
    Writing to a 32-bit register (e.g. `%eax`) **zeroes** the upper 32 bits of
    the 64-bit register. Writing to an 8- or 16-bit register does **not** touch
    the rest.

## Data sizes and suffixes

Instructions carry a suffix for operand size; it matches the C type being
operated on.

| Suffix | Size | C type | Example |
|---|---|---|---|
| `b` | 1 byte | `char` | `movb $5, %al` |
| `w` | 2 bytes | `short` | `movw $5, %ax` |
| `l` | 4 bytes | `int` | `movl $5, %eax` |
| `q` | 8 bytes | `long`, pointer | `movq $5, %rax` |

!!! tip
    `l` is "long word" = 32 bits — *not* C's `long`, which is 64-bit on
    x86-64. `q` is "quad word" = 64 bits.

## Operand forms

Three kinds of operand show up in instructions.

**Immediate** — a constant, prefixed with `$`:

```asm
$0x1F
```

**Register** — the contents of a register:

```asm
%rax
```

**Memory** — the interesting one. General form `D(Rb, Ri, S)`, meaning
`Mem[Reg[Rb] + S × Reg[Ri] + D]`:

| Part | Meaning |
|---|---|
| `D` | displacement — a constant, often 0 / 4 / 8 |
| `Rb` | base register |
| `Ri` | index register (cannot be `%rsp`) |
| `S` | scale factor: 1, 2, 4 or 8 |

Common patterns:

```asm
(%rax)           # Mem[rax]          base only
8(%rax)          # Mem[rax + 8]      base + displacement
(%rax,%rcx)      # Mem[rax + rcx]    base + index
(%rax,%rcx,4)    # Mem[rax + 4*rcx]  array access
0x10(,%rcx,8)    # Mem[8*rcx + 16]   scaled index, no base
```

!!! tip
    `arr[i]` with `arr` in `%rdi`, `i` in `%rsi` and 4-byte elements is
    `(%rdi,%rsi,4)`.

## Data movement — `mov`

`mov` copies source to destination. It cannot go memory → memory directly; route
through a register.

```asm
movq  %rax, %rbx      # reg → reg
movq  $42,  %rax      # imm → reg
movq  %rax, (%rbx)    # reg → mem
movq  (%rbx), %rax    # mem → reg
```

Zero-extending (`movz`) — fill the high bytes with zeros:

```asm
movzbq %al, %rax      # byte → quad, zero-fill
movzwl %ax, %eax      # word → long, zero-fill
```

Sign-extending (`movs`) — replicate the sign bit:

```asm
movsbq %al, %rax      # byte → quad, sign-fill
movslq %eax, %rax     # long → quad, sign-fill
cltq                  # sign-extend %eax → %rax (no operands)
```

!!! tip
    There's no `movzlq`: writing to `%eax` already zeroes the upper 32 bits of
    `%rax`, so a plain `movl` does the job.

## `lea` — load effective address

`leaq` computes an address but does **not** touch memory. The compiler leans on
it for arithmetic.

```asm
leaq (%rdi,%rsi), %rax     # rax = rdi + rsi
leaq (%rdi,%rdi,2), %rax   # rax = 3 * rdi
leaq 7(%rdi,%rdi,4), %rax  # rax = 5*rdi + 7
```

It's a one-instruction `x = a + b*s + d` that leaves the condition codes alone.

!!! tip
    `leaq` is the compiler's favourite trick for multiplying by 2, 3, 4, 5, 8
    or 9 without `imul`. Seeing `leaq` in disassembly usually means arithmetic,
    not addresses.

## Arithmetic and logical ops

Two-operand form: `OP src, dst` → `dst = dst OP src`.

**Arithmetic**

```asm
addq  %rax, %rbx   # rbx += rax
subq  %rax, %rbx   # rbx -= rax   (dst - src)
imulq %rax, %rbx   # rbx *= rax
negq  %rax         # rax = -rax
incq  %rax         # rax++
decq  %rax         # rax--
```

**Logical / bitwise**

```asm
andq  %rax, %rbx   # rbx &= rax
orq   %rax, %rbx   # rbx |= rax
xorq  %rax, %rax   # rax = 0  (the fast zeroing idiom)
notq  %rax         # rax = ~rax
```

**Shifts**

```asm
salq  $4, %rax     # rax <<= 4   (same as shlq)
sarq  $1, %rax     # rax >>= 1   arithmetic — sign-preserving
shrq  $1, %rax     # rax >>= 1   logical — zero-fill
salq  %cl, %rax    # shift by the amount in %cl
```

!!! tip
    The shift count must be an immediate or sit in `%cl` (the low byte of
    `%rcx`). No other register works.

## Multiplication and division

Two-operand `imulq` covers the everyday case. Full 128-bit products and *all*
division use fixed registers — `%rax` and `%rdx` — and take a single operand.

| Instruction | Effect |
|---|---|
| `imulq S, D` | `D = D * S`, truncated to 64 bits (signed and unsigned agree on the low half) |
| `imulq S` | signed full product: `%rdx:%rax = %rax * S` (128-bit) |
| `mulq S` | unsigned full product: `%rdx:%rax = %rax * S` |
| `cqto` | sign-extend `%rax` across `%rdx:%rax` — required before `idivq` |
| `idivq S` | signed divide: quotient → `%rax`, remainder → `%rdx` |
| `divq S` | unsigned divide, same registers |

`long quotient(long x, long y) { return x / y; }` compiles to:

```asm
movq  %rdi, %rax    # dividend into %rax
cqto                # sign-extend it across %rdx:%rax
idivq %rsi          # %rax = quotient, %rdx = remainder
```

!!! tip
    Division costs tens of cycles, so the compiler dodges it whenever the
    divisor is a constant — but a signed divide by a power of two needs a
    *bias*, because C rounds toward zero while `sar` rounds down:

    ```asm
    movq  %rdi, %rax   # x
    shrq  $63, %rax    # 1 if x < 0, else 0
    addq  %rdi, %rax   # add the bias first
    sarq  %rax         # (x + (x<0)) >> 1
    ```

    `shr $63` followed by an add and a shift is "signed divide by a power of
    two", not a bit-twiddling hack.

## Condition codes and comparisons

ALU ops set four flags as a side effect. `cmp` and `test` set them *without*
storing a result.

| Flag | Name | Set when |
|---|---|---|
| `CF` | Carry | unsigned overflow |
| `ZF` | Zero | result == 0 |
| `SF` | Sign | result < 0 |
| `OF` | Overflow | signed overflow |

Setting flags without saving the result:

```asm
cmpq  %rsi, %rdi   # computes rdi - rsi, sets flags, discards result
testq %rax, %rax   # computes rax & rax — is rax zero / negative / positive?
```

Reading flags back into a byte register with `setX`:

```asm
sete  %al    # al = ZF               equal
setne %al    # al = ~ZF              not equal
setl  %al    # al = SF^OF            less     (signed)
setg  %al    # al = ~(SF^OF) & ~ZF   greater  (signed)
setb  %al    # al = CF               below    (unsigned)
seta  %al    # al = ~CF & ~ZF        above    (unsigned)
```

!!! tip
    `cmpq b, a` sets flags as if computing `a - b` — the *first* operand is
    subtracted *from* the second. So `jl` after `cmpq %rsi, %rdi` jumps when
    `rdi < rsi`.

## Jumps and conditional moves

Conditional jumps branch on the condition codes; `cmov` moves data conditionally
without branching.

```asm
jmp  .L1      # unconditional
je   .L1      # equal          ZF
jne  .L1      # not equal      ~ZF
jl   .L1      # less           signed
jle  .L1      # less or equal  signed
jg   .L1      # greater        signed
jge  .L1      # greater/equal  signed
jb   .L1      # below          unsigned, CF
ja   .L1      # above          unsigned
```

Branchless conditional move — `val = (x > y) ? x : y`:

```asm
cmpq  %rsi, %rdi    # compare x, y
movq  %rsi, %rax    # rax = y  (the default)
cmovg %rdi, %rax    # if x > y: rax = x
```

!!! tip
    The compiler prefers `cmov` over a branch when both sides are simple
    expressions — no branch-prediction penalty. But `cmov` evaluates **both**
    sides, so it's unsafe when one of them could fault (a null deref, a
    division by zero).

## Loops → assembly patterns

C loops are lowered into conditional jumps. GCC typically emits a "do-while" or
a "jump to middle".

**do-while**

```asm
// do { body } while (test);
.L1:                   # loop:
  <body>
  cmpq ...
  jne  .L1             #   if test → loop
```

**while — jump to middle**

```asm
// while (test) { body }
  jmp  .L2             # goto test
.L1:                   # loop:
  <body>
.L2:                   # test:
  cmpq ...
  jne  .L1             #   if test → loop
```

**for — rewritten as a while**

```c
for (init; test; update) { body }

// becomes:
init;
while (test) { body; update; }
```

!!! tip
    Reading disassembly: a label, some work, then a conditional jump *back* to
    that label is a loop. The jump target tells you where the loop body starts.

## `switch` → jump table

A dense `switch` compiles to a jump table — an array of code addresses indexed
by the switch variable.

```asm
        .section .rodata
.L4:                          # jump table
        .quad   .L3           # case 0 → .L3
        .quad   .L8           # case 1 → .L8  (default)
        .quad   .L5           # case 2 → .L5
        .quad   .L6           # case 3 → .L6

        .text
        cmpq    $3, %rdi
        ja      .L8           # x > 3 (or negative) → default
        jmp     *.L4(,%rdi,8) # jump to table[x]
```

The indirect jump `jmp *.L4(,%rdi,8)` reads the address stored at
`.L4 + 8 × rdi` and jumps there.

!!! tip
    The giveaway is a bounds check (`cmpq $N`, `ja .default`) followed by an
    indirect `jmp` through a `.rodata` table. Two cases that do the same thing
    share one target label — the table just holds that label twice. And `ja` is
    unsigned, so the single bounds check catches negative indices too.

That's the book's form, and it's what you get with `-fno-pie -no-pie`. By
default today the table holds **4-byte offsets from its own address** rather
than absolute addresses, so the jump takes three more instructions:

```asm
        cmpq    $6, %rdi
        ja      .L9
        leaq    .L4(%rip), %rdx      # address of the table
        movslq  (%rdx,%rdi,4), %rax  # sign-extend the 32-bit offset
        addq    %rdx, %rax           # table address + offset = target
        notrack jmp *%rax

.L4:    .long  .L8-.L4               # entries are distances, not addresses
        .long  .L9-.L4
```

A sparse `switch` (say `case 1`, `case 500`, `case 9000`) gets no table at
all — the compiler emits a chain of `cmp`/`je` instead.

## Stack manipulation — `push` and `pop`

The stack grows **downward**, toward lower addresses, so pushing *decrements*
`%rsp`.

```asm
pushq %rbx     # rsp -= 8;  Mem[rsp] = rbx
popq  %rbx     # rbx = Mem[rsp];  rsp += 8
```

Both take one operand and always move 8 bytes in 64-bit code. To allocate room
for locals the compiler doesn't push repeatedly — it drops `%rsp` once:

```asm
subq  $24, %rsp    # allocate 24 bytes of frame
...
addq  $24, %rsp    # release it (or `leave` if %rbp is the frame pointer)
```

## Procedures and the stack

`call` pushes the return address and jumps; `ret` pops it and returns. The first
six arguments go in registers, the rest on the stack.

**System V AMD64 ABI**

| What | Where |
|---|---|
| Arguments 1–6 | `%rdi`, `%rsi`, `%rdx`, `%rcx`, `%r8`, `%r9` |
| Arguments 7+ | pushed on the stack by the caller |
| Return value | `%rax` (plus `%rdx` for 128-bit values) |

Stack frame layout, high → low address:

```text
┌──────────────────────┐  higher addresses
│  Argument 7, 8, ...  │  ← pushed by the caller
├──────────────────────┤
│  Return address      │  ← pushed by `call`
├──────────────────────┤
│  Saved %rbp          │  ← optional frame pointer
├──────────────────────┤
│  Saved callee-saved  │
│  regs: rbx, r12-r15  │
├──────────────────────┤
│  Local variables     │
├──────────────────────┤
│  Argument build area │  ← for calls this frame makes
└──────────────────────┘  ← %rsp
```

`call` / `ret` mechanics:

```asm
callq func    # pushq %rip  (the return address), then jmp func
ret           # popq  %rip  — pop the return address and jump back
```

!!! tip
    `%rsp` must be 16-byte aligned **before** a `call`. Since `call` pushes 8
    bytes, the callee starts with `%rsp ≡ 8 (mod 16)` — keep that in mind when
    allocating stack space with `subq`.

## Arrays in assembly

Array access is just `base + index × element_size`.

```asm
# int arr[5];  arr[i]
# %rdi = arr (base), %rsi = i (index)
movl (%rdi,%rsi,4), %eax    # eax = arr[i],  4 = sizeof(int)
```

Nested / 2D arrays are row-major — `M[i][j]` lives at `M + (C*i + j) * size`:

```asm
# int M[4][3];  M[i][j]  →  M + (3*i + j) * 4
# %rdi = M, %rsi = i, %rdx = j
leaq (%rsi,%rsi,2), %rax    # rax = 3*i
addq %rdx, %rax             # rax = 3*i + j
movl (%rdi,%rax,4), %eax    # eax = M[i][j]
```

!!! tip
    Fixed-size arrays get immediate scale factors and `lea` tricks.
    Variable-length arrays need the row stride computed at run time — expect an
    `imulq`.

## Structs, unions and alignment

Struct fields are laid out in declaration order, with padding inserted for
alignment. Union fields all start at offset 0.

```c
struct S {
  char   a;     // offset 0   (1 byte)
                // 3 bytes padding
  int    b;     // offset 4   (4 bytes, needs 4-byte alignment)
  char   c;     // offset 8   (1 byte)
                // 7 bytes padding
  long   d;     // offset 16  (8 bytes, needs 8-byte alignment)
};              // total: 24 bytes, 8-byte aligned
```

```text
offset:   0    1    2    3    4    5    6    7
        ┌────┬────┬────┬────┬────┬────┬────┬────┐
        │ a  │pad │pad │pad │      b (int)      │
        └────┴────┴────┴────┴────┴────┴────┴────┘
offset:   8    9   10   11   12   13   14   15
        ┌────┬────┬────┬────┬────┬────┬────┬────┐
        │ c  │pad │pad │pad │pad │pad │pad │pad │
        └────┴────┴────┴────┴────┴────┴────┴────┘
offset:  16   17   18   19   20   21   22   23
        ┌────┬────┬────┬────┬────┬────┬────┬────┐
        │              d (long)                 │
        └────┴────┴────┴────┴────┴────┴────┴────┘
```

**Alignment rules.** K-byte data must sit at an address divisible by K. The
struct as a whole is aligned to its largest member, and its size is rounded up
to a multiple of that alignment.

```c
union U {
  int    i;    // every field starts at offset 0
  double d;    // size = max(4, 8, 1) = 8
  char   c;
};             // total: 8 bytes
```

!!! tip
    To shrink a struct, order fields largest → smallest. That removes the
    internal padding gaps (trailing padding to the struct's own alignment
    stays).

## Addressing globals — `%rip`-relative

Locals live at an offset from `%rsp`. Globals, statics, string literals and
jump tables are addressed relative to the **instruction pointer**, so the code
works wherever the loader maps it — position-independent executables are the
default on every current distro.

`long counter; long bump(void) { return ++counter; }`:

```asm
movq counter(%rip), %rax
addq $1, %rax
movq %rax, counter(%rip)
```

For an array the base address gets materialized first, then indexed as usual:

```asm
leaq table(%rip), %rax      # rax = &table[0]
movq (%rax,%rdi,8), %rax    # rax = table[i]
```

!!! tip
    `name(%rip)` means a global, a static, a literal or a jump table — never a
    local. Locals are `N(%rsp)` or `N(%rbp)`.

## Floating point — the `%xmm` registers

Floating point never touches the general-purpose registers. It has its own file
of sixteen 128-bit registers, `%xmm0`–`%xmm15`, and its own instructions (SSE2).

| | |
|---|---|
| Arguments | `%xmm0`–`%xmm7`, counted separately from the integer arguments |
| Return value | `%xmm0` |
| Saving | all caller-saved — there are no callee-saved `%xmm` registers |

Read the suffixes as two letters: `s` = **s**calar (one value, not a vector),
then `s` = **s**ingle (`float`) or `d` = **d**ouble (`double`).

| Instruction | Meaning |
|---|---|
| `movss` / `movsd` | move one `float` / `double` |
| `addss` / `addsd` | add — likewise `sub`, `mul`, `div` |
| `cvtsi2sd` | `int` → `double` |
| `cvttsd2si` | `double` → `int`, **t**runcating, which is what a C cast does |
| `cvtss2sd` / `cvtsd2ss` | between `float` and `double` |
| `comisd` / `ucomisd` | compare two values and set the flags |
| `xorpd` / `andpd` | sign-bit tricks — negation and `fabs` |

`double dadd(double x, double y) { return x + y; }` is a single instruction:

```asm
addsd %xmm1, %xmm0    # args arrive in xmm0/xmm1, result leaves in xmm0
```

!!! tip
    Two surprises worth memorizing. `-x` is not arithmetic: it's `xorpd`
    against a constant that has only the sign bit set. And FP comparison sets
    the flags like an **unsigned** compare, so `x < y` becomes `comisd` plus
    `seta`/`setb` — you will never see `setl`/`setg` on floating point.

## Buffer overflow basics

Writing past the end of a stack buffer walks upward into the saved registers
and the return address — overwrite that, and you control where `ret` goes.

```text
┌──────────────────────┐  higher addresses
│  Return address      │  ← overwritten → control flow hijacked
├──────────────────────┤
│  Saved %rbp          │  ← corrupted on the way there
├──────────────────────┤
│  buf[7]              │  ← the overflow keeps writing upward
│  ...                 │
│  buf[0]              │  ← gets() starts writing here
└──────────────────────┘  ← %rsp
```

**Protections**

| Mechanism | What it does |
|---|---|
| Stack canary | A secret value between the locals and the return address, checked before `ret` |
| ASLR | Randomizes stack / heap / library base addresses on every run |
| NX / DEP | Marks the stack non-executable, so injected shellcode can't run |

NX is why *return-oriented programming* exists: if you can't inject new code,
chain fragments of code that's already there. A "gadget" is a handful of
instructions ending in `ret`; a stack full of gadget addresses executes them
one after another, since each `ret` pops the next one. ASLR is the defence —
the addresses aren't predictable.

GCC's stack protector, as it appears in disassembly:

```asm
movq %fs:40, %rax         # load the canary from thread-local storage
movq %rax, 8(%rsp)        # stash it just below the return address
xorl %eax, %eax           # don't leave a copy lying around in a register
...
movq 8(%rsp), %rax        # on the way out, read it back
subq %fs:40, %rax         # still the same value?
jne  .L4                  # no → corrupted
addq $16, %rsp
ret
.L4:
call __stack_chk_fail@PLT
```

That's `gcc -Og` output verbatim. The book writes the check as `xorq %fs:40,
%rax`; current gcc uses `subq`. Same test — both leave zero in `%rax` exactly
when the canary is intact.

!!! tip
    `gets()` is the classic culprit — no bounds checking at all. Use `fgets()`.
    Modern GCC enables `-fstack-protector-strong` by default on most distros.

## Output the book predates

CS:APP's listings were produced around 2014. A current `gcc` on a current
distro adds things the book never shows. None of them change the chapter's
model, but all of them will be in front of you in every listing.

| What you see | Why it's there |
|---|---|
| `endbr64` opening nearly every function | Intel CET / branch tracking — it marks a legal target for an indirect jump or call. `-fcf-protection=none` removes it. |
| `name(%rip)` instead of absolute addresses | PIE is the default. `-fno-pie -no-pie` restores the book's addressing. |
| Jump tables of `.long` offsets, not `.quad` addresses | Also PIE, as above. |
| `call __strcpy_chk` where the source says `strcpy` | `_FORTIFY_SOURCE` is on by default on Ubuntu; when the compiler knows the buffer size it swaps in a checked variant. |
| `%fs:40` loads and a `__stack_chk_fail` branch | `-fstack-protector-strong` is on by default. |

To get listings close to the book's:

```bash
gcc -Og -fno-pie -no-pie -fcf-protection=none \
    -fno-stack-protector -fno-asynchronous-unwind-tables -S prog.c
```

## See also

- [From C to assembly](../guides/csapp-ch3-c-to-asm.md) — every pattern above,
  compiled and read on a real machine
- [gdb & objdump](gdb-objdump.md) — the tools for looking at a binary
- [CS:APP ch. 3 resources](../bookmarks/csapp-ch3.md) — course site, labs,
  references
- [Runnable examples](../examples/csapp-ch3/index.md) — the C files behind the listings

---

Source: Bryant & O'Hallaron, *Computer Systems: A Programmer's Perspective*,
3rd ed., ch. 3. Study companion — not a substitute for the text. Listings
marked as real output were generated with gcc 13.3 on x86-64 Linux.
