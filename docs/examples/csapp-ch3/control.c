/* Control flow: branches, conditional moves, the three loop shapes.
   Try:  gcc -Og -S control.c -o - | less                         */

long max(long x, long y) { return x > y ? x : y; }   /* → cmovg, branchless */
long absval(long x)      { return x < 0 ? -x : x; }

/* Unsafe to make branchless: the compiler must keep the branch, because
   dereferencing p is not allowed when p is NULL. */
long deref_or_zero(long *p) { return p ? *p : 0; }

long sum_while(long *a, long n) {
    long s = 0, i = 0;
    while (i < n) { s += a[i]; i++; }
    return s;
}

long sum_for(long *a, long n) {
    long s = 0;
    for (long i = 0; i < n; i++) s += a[i];
    return s;
}

/* do-while: the body is known to run at least once, so no guard test. */
long countdown(long n) {
    long r = 0;
    do { r += n; n--; } while (n > 0);
    return r;
}
