extern int input(void);
extern void output(int);

int main() {
  int x = input();
  int y = input();

  int* p = 0;
  int* q = 0;

  if (input()) {
    p = &x;
    q = &y;
  } else {
    p = &y;
    q = &x;
  }

  if (p < q) {
    *q = 42;
  }

  int z = input();

  if (z < 0) {
    *p = (z - 10) ? *q : *p;
  }

  output(*p);
}
