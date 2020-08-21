// Copyright 2012 Rui Ueyama. Released under the MIT license.

#include "test.h"

extern int externvar1;
int extern externvar2;

int a = 3;

void test() {
  extern int a;
  expect(3, a);
  {
    int a = 4;
    expect(4, a);
  }
  {
    extern int a;
    expect(3, a);
    a = 5;
    {
      extern int a;
      expect(5, a);
    }
  }
}

void testmain() {
  print("extern");
  expect(98, externvar1);
  expect(99, externvar2);
  test();
}
