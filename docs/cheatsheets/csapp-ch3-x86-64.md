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
    indirect `jmp` through a `.rodata` table. Cases that fall through share the
    same target label; `ja` is unsigned, so it catches negative indices too.

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

GCC's stack protector, as it appears in disassembly:

```asm
movq %fs:40, %rax      # load the canary from thread-local storage
movq %rax, -8(%rbp)    # place it just below the return address
...
movq -8(%rbp), %rax    # on the way out, read it back
xorq %fs:40, %rax      # still the same value?
jne  __stack_chk_fail  # no → canary corrupted, abort
```

!!! tip
    `gets()` is the classic culprit — no bounds checking at all. Use `fgets()`.
    Modern GCC enables `-fstack-protector-strong` by default on most distros.

---

Source: Bryant & O'Hallaron, *Computer Systems: A Programmer's Perspective*,
3rd ed., ch. 3. Study companion — not a substitute for the text.
