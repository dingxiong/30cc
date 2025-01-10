void _printf(char *, ...);

int foo(int n) {
  return n;
}

int main() {
  foo(5);
  _printf("xxx %d\n", 5);
  return 0;
}
