extern int input(void);
extern void output(int);

int main() {
  int x = input();
  if (!x) { x++; }
  if (x || input()) { x++; }
  output(x);
}
