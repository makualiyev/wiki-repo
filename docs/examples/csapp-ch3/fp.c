/* Floating point lives in %xmm registers with its own instruction set.
   Try:  gcc -Og -S fp.c -o - | less                                    */

float  fadd(float x, float y)   { return x + y; }     /* → addss, %xmm0 */
double dadd(double x, double y) { return x + y; }     /* → addsd */
double dmul(double x, double y) { return x * y; }     /* → mulsd */

double int2double(int x)    { return x; }             /* → cvtsi2sd */
int    double2int(double x) { return (int) x; }       /* → cvttsd2si, truncates */
double float2double(float x){ return x; }             /* → cvtss2sd */

/* Bit tricks: negation flips the sign bit with xorps, no arithmetic. */
double negate(double x) { return -x; }

int  fcompare(double x, double y) { return x < y; }   /* → comisd/ucomisd */
double pick(double x, double y, int c) { return c ? x : y; }
