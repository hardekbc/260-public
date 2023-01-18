extern int input(void);
extern void output(int);

void foo(int p) {
  output(p);
}

int main() {
  int x = input();
  foo(x);
}
