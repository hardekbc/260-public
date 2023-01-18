typedef struct {
  int one;
  int two;
} foo;

extern foo* input(void);
extern void output(int);

int main() {
  foo* x = input();
  output(x->one + x->two);
}
