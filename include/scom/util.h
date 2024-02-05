#pragma once

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define bool int
#define true 1
#define false 0

static int make_align(int val, int align) {
  assert(align > 0);
  return (val + align - 1) / align * align;
}

static uint32_t endian_swap(uint32_t val) {
  return ((val >> 24) & 0xff)
         | ((val >> 8) & 0xff00)
         | ((val << 8) & 0xff0000)
         | ((val << 24) & 0xff000000);
}

static int startswith(const char* s, const char* t) {
  int len = strlen(t);
  return strncmp(s, t, len) == 0;
}

static int endswith(const char* s, const char* t) {
  int l1 = strlen(s);
  int l2 = strlen(t);

  return l1 >= l2 && strcmp(s + l1 - l2, t) == 0;
}

static char* lenstrdup(const char* src, int len) {
  char* dst = (char*) malloc(len + 1);
  assert(dst != NULL);
  memcpy(dst, src, len);
  dst[len] = '\0';
  return dst;
}
