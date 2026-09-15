/* switch: dense cases become a jump table, sparse ones a chain of compares.
   Try:  gcc -Og -S switch.c -o - | less                                   */

long dense(long x, long y) {
    long r = y;
    switch (x) {                 /* 0..6 → jump table in .rodata */
        case 0:  r = y * 13; break;
        case 2:
        case 5:  r = y + 10; break;   /* shared target: fall-through cases */
        case 3:  r = y << 2; break;
        case 4:  r = y - 3;  break;
        case 6:  r += 7;     break;   /* case 2 and 5 share one target */
        default: r = 0x1234;
    }
    return r;
}

long sparse(long x) {            /* 1, 500, 9000 → compare chain, no table */
    switch (x) {
        case 1:    return 11;
        case 500:  return 22;
        case 9000: return 33;
        default:   return 0;
    }
}
