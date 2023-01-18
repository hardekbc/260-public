extern int input(void);
extern void output(int);

int main() {
  int n = input();
  int factor = 2;
  int primality = 1;

  while (factor < n) {
    if (((n / factor) * factor) == n) {
      primality = 0;
    }
    factor++;
  }

  output(primality);
}
