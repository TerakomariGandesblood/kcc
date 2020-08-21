// Copyright 2012 Rui Ueyama. Released under the MIT license.

#include "test.h"

enum { g1, g2, g3 } global1;

void test() {
  typedef enum {
    AA,
    BB = 3,
    CC = 3,
    DD,
  } idtype_t;

  expect(0, AA);
  expect(3, BB);
  expect(4, DD);
  {
    int AA = -4;
    int BB = -5;
    expect(-4, AA);
    expect(-5, BB);
  }
}

void testmain() {
  print("enum");
  test();

  expect(0, g1);
  expect(2, g3);

  enum { x } v;
  expect(0, x);

  enum { y };
  expect(0, y);

  enum tag { z };
  enum tag a = z;
  expect(0, z);
  expect(0, a);
}
