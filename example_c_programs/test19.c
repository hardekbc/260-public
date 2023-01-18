typedef struct bar {
  int** x;
  int y;
  int* z;
} bar;

extern bar* bar_malloc(void);

int main() {
  bar* a = bar_malloc();
  bar* b = bar_malloc();
  int*** c = &a->x;
  int** d = &b->z;
  int* e = &a->y;
  *c = d;
}
