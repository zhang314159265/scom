#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef FAIL
// suppress the definition from gtest: /usr/include/gtest/gtest.h
#undef FAIL
#endif

#define FAIL(fmt...) { \
  fprintf(stderr, "%s:%d: Encounter the following error and abort:\n", __FILE__, __LINE__); \
  fprintf(stderr, "\033[31m"); \
  fprintf(stderr, fmt); \
  fprintf(stderr, "\033[0m"); \
  fprintf(stderr, "\n"); \
  assert(0); \
}

#define CHECK(cond, fmt...) { \
  if (!(cond)) { \
    FAIL(fmt); \
  } \
}
