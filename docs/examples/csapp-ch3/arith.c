/* Arithmetic: what the compiler does instead of imul/idiv.
   Try:  gcc -Og -S arith.c -o - | less                         */

long times3(long x)      { return x * 3; }        /* → leaq (%rdi,%rdi,2) */
long times5plus7(long x) { return x * 5 + 7; }    /* → leaq 7(%rdi,%rdi,4) */
long times12(long x)     { return x * 12; }       /* → lea + shift */

long divide_by_2(long x) { return x / 2; }        /* signed: needs a bias */
long shift_by_2(long x)  { return x >> 2; }       /* arithmetic shift */
unsigned long ushift(unsigned long x) { return x >> 2; }  /* logical shift */

long quotient(long x, long y) { return x / y; }   /* → cqto + idivq */
long remainder_(long x, long y) { return x % y; } /* same divide, %rdx */

long zero(void)          { return 0; }            /* → xorl %eax, %eax */
int  low32(long x)       { return (int) x; }      /* truncate */
long widen_signed(int x) { return x; }            /* → movslq / cltq */
unsigned long widen_unsigned(unsigned x) { return x; } /* → movl, implicit zero-extend */
