extern int input(void);
extern void output(int);

int main() {
  int x = input();
  int y = x;
  int z = x;
  output(x + y + z);
}
