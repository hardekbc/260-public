extern int input(void);
extern void output(int);

int foo(int **p) {
  return **p;
}

int main() {
  int x = input();
  int *y = &x;
  int **z = &y;
  output(foo(z));
}
