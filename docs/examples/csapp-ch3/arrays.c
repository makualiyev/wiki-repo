/* Arrays: base + index * element size, and row-major 2D layout.
   Try:  gcc -Og -S arrays.c -o - | less                          */

int  elem_int (int  *a, long i) { return a[i]; }   /* scale 4 */
long elem_long(long *a, long i) { return a[i]; }   /* scale 8 */
char elem_char(char *a, long i) { return a[i]; }   /* scale 1 */

/* Row-major: M[i][j] sits at M + (3*i + j) * sizeof(int) */
int elem2d(int M[4][3], long i, long j) { return M[i][j]; }

/* Variable-length rows: the stride is a runtime multiply (imul). */
int elem_vla(long n, int M[][n], long i, long j) { return M[i][j]; }

/* Pointer walking compiles to the same thing as indexing. */
long sum_ptr(long *a, long *end) {
    long s = 0;
    while (a < end) s += *a++;
    return s;
}
