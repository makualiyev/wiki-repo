/* Procedures: argument registers, callee-saved registers, recursion.
   Try:  gcc -Og -S proc.c -o - | less                                */

long callee(long a, long b, long c, long d, long e, long f, long g, long h);

/* Args 1-6 in registers, 7+ pushed on the stack by the caller. */
long caller8(void) { return callee(1, 2, 3, 4, 5, 6, 7, 8); }

/* Needs a value preserved across a call → gets a callee-saved register,
   which the callee must push on entry and pop before ret. */
long keep_across_call(long x) {
    long saved = x * 3;
    long got   = callee(x, 0, 0, 0, 0, 0, 0, 0);
    return saved + got;
}

long factorial(long n) { return n <= 1 ? 1 : n * factorial(n - 1); }

/* Taking a local's address forces it onto the stack — it cannot live
   in a register, because a register has no address. */
long needs_stack(long x) {
    long buf[4] = { x, x + 1, x + 2, x + 3 };
    return callee((long) buf, 0, 0, 0, 0, 0, 0, 0);
}
