extern int input(void);
extern void output(int);

typedef struct foo {
  int x;
  int y;
} foo;

typedef struct bar {
  foo a;
  foo b;
} bar;

int main() {
  bar s;

  s.a.x = 0;
  s.a.y = 1;
  s.b.x = 2;
  s.b.y = 3;
}
