#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scom/check.h"

#define VEC_FOREACH_I(vec_ptr, item_type, item_ptr, i) \
  item_type* item_ptr = NULL; \
  for (int i = 0; i < (vec_ptr)->len && (item_ptr = vec_get_item(vec_ptr, i)); ++i)

// TODO: can we generate a unique name for the iterator variable
// automatically.
#define VEC_FOREACH(vec_ptr, item_type, item_ptr) VEC_FOREACH_I(vec_ptr, item_type, item_ptr, __i)

struct vec {
  int itemsize;
  int capacity; // in number of item
  int len; // in number of item
  void *data;
};

/*
 * Make sure we don't malloc in this function so there is
 * no need to free if there is no insertion to this vector
 * later.
 */
static inline struct vec vec_create(int itemsize) {
  struct vec vec;
  vec.itemsize = itemsize;
  vec.capacity = 0;
  vec.len = 0;
	vec.data = NULL;
  return vec;
}

static inline void vec_free(struct vec* vec) {
  if (vec->data) {
    free(vec->data);
  }
  vec->data = NULL;
}

static inline void vec_append(struct vec* vec, void *itemptr) {
  if (vec->len == vec->capacity) {
    if (vec->capacity == 0) {
      vec->capacity = 16;
    } else {
      vec->capacity <<= 1;
    }
    vec->data = realloc(vec->data, vec->capacity * vec->itemsize);
  }
  memcpy(vec->data + vec->len * vec->itemsize, itemptr, vec->itemsize);
  ++vec->len;
}

static inline void* vec_get_item(struct vec* vec, int idx) {
  CHECK(idx >= 0 && idx < vec->len, "vec index out of range: index %d, size %d", idx, vec->len);
  return vec->data + idx * vec->itemsize;
}

/*
 * The caller should use the pointer immediately since they may store some
 * other items or be freed or get reallocated.
 */
static inline void* vec_pop_item(struct vec* vec) {
  assert(vec->len > 0);
  return vec->data + (--vec->len) * vec->itemsize;
}

/*
 * return 1 if string is found, 0 otherwise.
 * This does a linear scan thru the vector.
 */
static inline int vec_str_find(struct vec* vec, char* needle) {
  assert(vec->itemsize == sizeof(char*));  // this is the best check we can have RN since we don't store item type
  VEC_FOREACH(vec, char*, item_ptr) {
    if (strcmp(*item_ptr, needle) == 0) {
      return 1;
    }
  }
  return 0;
}
