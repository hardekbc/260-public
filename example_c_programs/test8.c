extern int input(void);
extern void output(int);

typedef struct foo {
  int bar;
  struct foo* next;
} foo;

extern foo* foo_malloc(int);

int main() {
  foo x;
  x.bar = input();
  x.next = foo_malloc(1);
  x.next->bar = input();
  x.next->next = 0;
  output(x.next->bar);
}
