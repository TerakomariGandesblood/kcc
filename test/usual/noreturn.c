// Copyright 2014 Rui Ueyama. Released under the MIT license.

#include <stdnoreturn.h>
#include "test.h"

// _Noreturn is ignored
_Noreturn void f1();
noreturn void f2();
inline void f3() {}

void testmain() { print("noreturn"); }
