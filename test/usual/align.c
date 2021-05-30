// Copyright 2012 Rui Ueyama. Released under the MIT license.

#include <stdalign.h>
#include <stddef.h>
#include "test.h"

static void test_alignas() {
  expect(1, offsetof(
                struct {
                  char x;
                  char y;
                },
                y));
}

static void test_alignof() {
  expect(1, __alignof_is_defined);
  expect(1, _Alignof(char));
  expect(1, __alignof__(char));
  expect(1, alignof(char));
  expect(2, alignof(short));
  expect(4, alignof(int));
  expect(8, alignof(double));
  expect(1, alignof(char[10]));
  expect(8, alignof(double[10]));
  expect(1, _Alignof(struct {}));
  expect(4, alignof(struct {
           char a;
           int b;
         }));
  expect(16, alignof(struct {
           int a;
           long double b;
         }));
  expect(16, alignof(long double));

  // The type of the result is size_t.
  expect(1, alignof(char) - 2 > 0);
}

static void test_constexpr() {
  char a[alignof(int)];
  expect(4, sizeof(a));
}

void testmain() {
  print("alignment");
  test_alignas();
  test_alignof();
  test_constexpr();
}
