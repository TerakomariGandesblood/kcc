#include "test.h"

void test1() {
  typedef struct {
    unsigned int a : 1;
    unsigned int b : 8;
    unsigned int c : 3;
    unsigned int d : 24;
  } foo_t;
  expect(8, sizeof(foo_t));
}

void test2() {
  typedef struct {
    unsigned int a : 1;
    unsigned int b : 3;
    unsigned int : 8;
    unsigned int c : 3;
    unsigned int d : 24;
  } foo_t;
  expect(8, sizeof(foo_t));
}

void test3() {
  typedef struct {
    unsigned int a : 6;
    unsigned int : 0;
    unsigned int b : 1;
  } foo_t;
  expect(8, sizeof(foo_t));
}

void test4() {
  typedef struct {
    unsigned int a : 8;
    unsigned int b : 9;
  } foo_t;
  expect(4, sizeof(foo_t));
}

void test5() {
  typedef union {
    struct {
      unsigned int a : 15;
      unsigned int b : 20;
    };
    unsigned long c;
  } foo_t;
  expect(8, sizeof(foo_t));
}

void test6() {
  typedef struct {
    unsigned int a : 8;
    unsigned int b : 9;
  } foo_t;
  expect(4, sizeof(foo_t));
}

void test7() {
  typedef struct {
    unsigned int a : 8;
    unsigned int b : 9;
  } foo_t;
  expect(4, sizeof(foo_t));
}

void testmain() {
  print("bitfield");
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
  test7();
}
