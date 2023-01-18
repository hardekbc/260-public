extern int input(void);
extern void output(int);

int factorial(int n) {
  if (n <= 1) return 1;
  return n * factorial(n-1);
}

int main() {
  output(factorial(input()));
}
