extern int input(void);
extern void output(int);

struct foo {
  int bar;
  struct foo* next;
};

extern int* int_malloc(int);
extern struct foo* foo_malloc(int);

int main() {
  struct foo* foos = foo_malloc(42);
  for (int i = 0; i < input(); i++) {
    foos[i].bar = input();
    foos[i].next = 0;
  }

  output(foos[13].bar);

  int *ints = int_malloc(42);
  for (int j = 0; j < input(); j++) {
    ints[j] = input();
  }

  output(ints[17]);
}
