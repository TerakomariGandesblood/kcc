#include <stdalign.h>
#include "test.h"

typedef struct {
  int a;
  int b;
  long long arr[];
} foo_t;

static void test_incomp() {
  expect(8, sizeof(foo_t));
  expect(8, alignof(foo_t));
}

typedef struct {
  int a;
  int b;
  long long arr[0];
} bar_t;

static void test_zero_size() {
  expect(8, sizeof(bar_t));
  expect(8, alignof(bar_t));
  expect(8, alignof(bar_t));
}

void testmain() {
  print("struct incomp");
  test_incomp();
  test_zero_size();
}
