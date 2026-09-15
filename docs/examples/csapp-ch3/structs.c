/* Struct layout, padding and alignment — run it, don't guess.
   Try:  gcc -O2 -o structs structs.c && ./structs                */

#include <stdio.h>
#include <stddef.h>

struct bad  { char a; int b; char c; long d; };   /* padding-heavy order */
struct good { long d; int b; char a; char c; };   /* largest → smallest */
union  U    { int i; double d; char c; };

struct node { long val; struct node *next; };

long node_val (struct node *p) { return p->val; }        /* → 0(%rdi) */
struct node *node_next(struct node *p) { return p->next; } /* → 8(%rdi) */

int main(void) {
    printf("struct bad   size=%2zu align=%zu   a=%zu b=%zu c=%zu d=%zu\n",
           sizeof(struct bad), _Alignof(struct bad),
           offsetof(struct bad, a), offsetof(struct bad, b),
           offsetof(struct bad, c), offsetof(struct bad, d));
    printf("struct good  size=%2zu align=%zu   d=%zu b=%zu a=%zu c=%zu\n",
           sizeof(struct good), _Alignof(struct good),
           offsetof(struct good, d), offsetof(struct good, b),
           offsetof(struct good, a), offsetof(struct good, c));
    printf("union U      size=%2zu align=%zu\n",
           sizeof(union U), _Alignof(union U));
    return 0;
}
