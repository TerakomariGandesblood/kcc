// Copyright 2012 Rui Ueyama. Released under the MIT license.

#include "test.h"

static void test_or() {
  expect(3, 1 | 2);
  expect(7, 2 | 5);
  expect(7, 2 | 7);

  int a = 3, b = 4;
  expect(7, a | b);
  expect(11, a | (b << 1));
  expect(3, a | a);
  expect(4, b | b);
}

static void test_and() {
  expect(0, 1 & 2);
  expect(2, 2 & 7);

  int a = 3, b = 4;
  expect(0, a & b);
  expect(3, a & a);
  expect(0, a & 0);
  expect(4, b & 7);
}

static void test_not() {
  expect(-1, ~0);
  expect(-3, ~2);
  expect(0, ~-1);

  int a = 3, b = 4;
  expect(-3, (unsigned)~a + 1);
  expect(-1, ~0);
}

static void test_xor() {
  expect(10, 15 ^ 5);

  int a = 3, b = 4;
  expect(0, a ^ a);
  expect(7, a ^ b);
  expect(3, a ^ 0);
  expect(-3, (a ^ -1) + 1);
}

static void test_shift() {
  expect(16, 1 << 4);
  expect(48, 3 << 4);
  expect(1, 15 >> 3);
  expect(2, 8 >> 2);
  expect(1, ((unsigned)-1) >> 31);

  int a = 3, b = 4;
  expect(48, 3 << 4);
  expect(0, 3 >> 4);
  expect(3, a << 0);
  expect(4, b >> 0);
}

void testmain() {
  print("bitwise operators");
  test_or();
  test_and();
  test_not();
  test_xor();
  test_shift();
}
