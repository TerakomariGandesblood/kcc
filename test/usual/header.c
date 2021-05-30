//
// Created by kaiser on 2019/11/14.
//

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fenv.h>
#include <float.h>
#include <inttypes.h>
#include <iso646.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <uchar.h>
#include <wchar.h>
#include <wctype.h>

#if !__STDC_NO_COMPLEX__
#include <complex.h>
#endif

#if !__STDC_NO_ATOMICS__
#include <stdatomic.h>
#endif

#if !__STDC_NO_COMPLEX__
#include <tgmath.h>
#endif

#if !__STDC_NO_THREADS__
#include <threads.h>
#endif

#include "test.h"

void testmain() { print("header"); }
