extern int input(void);
extern void output(int);

typedef struct foo {
  int* a;
  int* b;
} foo;

typedef struct bar {
  foo x;
  foo* y;
} bar;

extern int* int_malloc(int);
extern foo* foo_malloc(int);
extern bar* bar_malloc(int);

int main() {
  bar* p = bar_malloc(1);
  bar* q = bar_malloc(1);
  bar *r = bar_malloc(1);

  p->x.a = int_malloc(42);
  p->x.b = int_malloc(42);
  p->y = foo_malloc(1);

  r->x.a = p->x.a;
  r->x.b = p->x.b;
  r->y = p->y;

  q->x.a = int_malloc(42);
  q->x.b = int_malloc(42);
  q->y = foo_malloc(1);

  p->x.a = q->x.a;
  p->x.b = q->x.b;
  p->y = q->y;
}
