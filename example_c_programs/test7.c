extern int input(void);
extern void output(int);

typedef int (*int2int_t)(int);

int foo(int param) {
  return param;
}

int bar(int param) {
  return param;
}

int main() {
  int2int_t func_ptr = input() ? foo : bar;
  output(func_ptr(input()));
}
