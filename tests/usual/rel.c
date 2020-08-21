#include "test.h"

static void test1() {
  int a = 3, b = 4;
  expect(1, a < b);
  expect(1, a <= b);
  expect(0, a > b);
  expect(0, a >= b);
  expect(1, a <= 3);
  expect(1, a == 3);
  expect(1, a >= 3);
  expect(1, (a >= 3) >= 1);
  expect(1, a != b);
}

void testmain() {
  print("rel");
  test1();
}
