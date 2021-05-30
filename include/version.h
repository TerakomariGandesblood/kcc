#pragma once

#define KCC_VER_MAJOR 0

#define KCC_VER_MINOR 1

#define KCC_VER_PATCH 0

#define KCC_VERSION \
  (KCC_VER_MAJOR * 10000 + KCC_VER_MINOR * 100 + KCC_VER_PATCH)

#define STRINGIFY(x) #x

#define TO_STRING(x) STRINGIFY(x)

#define KCC_VERSION_STR TO_STRING(KCC_VER_MAJOR.KCC_VER_MINOR.KCC_VER_PATCH)