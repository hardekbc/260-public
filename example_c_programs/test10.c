extern int input(void);
extern void output(int);

int main() {
  int x = 0;
  if (input() && input()) x = 1;
  output(x);
}
