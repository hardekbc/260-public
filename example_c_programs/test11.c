extern int input(void);
extern void output(int);

int main() {
  int x = 0;
  while (x > 0 && x < 10) x++;
  output(x);
}
