extern int input(void);
extern void output(int);

int main() {
  int a = input();

  if (input()) {
    int b = input();
    a += b;
  } else {
    int c = input();
    a -= c;
  }

  output(a);
}
