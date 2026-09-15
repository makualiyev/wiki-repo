/* Buffer overflow and the stack protector. DO NOT feed this real input —
   it exists to be compiled and read, two ways:

     gcc -Og -S overflow.c -o with-canary.s
     gcc -Og -fno-stack-protector -S overflow.c -o without-canary.s
     diff with-canary.s without-canary.s

   Ubuntu's gcc enables -fstack-protector-strong by default, so the canary
   is in the first listing and gone from the second.                       */

#include <stdio.h>
#include <string.h>

/* The classic: no bounds check, the caller cannot say how big buf is. */
void unsafe_copy(const char *src) {
    char buf[8];
    strcpy(buf, src);          /* writes past buf[7] if src is longer */
    puts(buf);
}

/* Bounded: the size travels with the buffer. */
void safe_copy(const char *src) {
    char buf[8];
    snprintf(buf, sizeof buf, "%s", src);
    puts(buf);
}

/* gets() was removed from C11 entirely; fgets() carries a size. */
void read_line(void) {
    char buf[64];
    if (fgets(buf, sizeof buf, stdin)) fputs(buf, stdout);
}
