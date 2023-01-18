extern int input(void);
extern void output(int);

int main() {
  int x;
  int y = input();

  if (y < 0) x = 0;
  if (y > 0) x = 1;

  output(x);
}
