#include <stdio.h>

int factorial(int x) {
  return x == 1 ? 1  : x * factorial(x - 1);
}

int main() {
  printf("%d\n", factorial(5));  
}
