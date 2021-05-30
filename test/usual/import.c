// Copyright 2012 Rui Ueyama. Released under the MIT license.

#include "test.h"

// import.h would raise an error if read twice.
#import "../usual/import.h"
#import "import.h"
#include "import.h"

// once.h would raise an error if read twice
#include "../usual/once.h"
#include "once.h"
#import "once.h"

void testmain() { print("import"); }
