#include "test.h"

static void test1() {
  int a = 3, b = 4;
  expect(1, a && b);
  expect(1, a || b);
  expect(0, !a || !b);
  expect(1, !a || b);
  expect(1, a || !b);
}

static void test2() {
  char a, b;
  a = 3;
  b = 4;
  expect(1, a && b);
  expect(1, a || b);
  expect(0, !a || !b);
  expect(1, !a || b);
  expect(1, a || !b);
}

static void test3() {
  float a, b;
  a = 3.0f;
  b = 4.0f;
  expectf(1.0f, a && b);
  expectf(1.0f, a || b);
  expectf(0.0f, !a || !b);
  expectf(1.0f, !a || b);
  expectf(1.0f, a || !b);
}

void testmain() {
  print("logic");
  test1();
  test2();
  test3();
}