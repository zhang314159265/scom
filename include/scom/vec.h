#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scom/check.h"
#include "scom/util.h"

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

  // whether the destructor should free each item
  bool should_free_each_item;
};

static inline void vec_append(struct vec* vec, void *itemptr);
static inline void* vec_get_item(struct vec* vec, int idx);

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
  vec.should_free_each_item = false;
  return vec;
}

// spaces preceding items with be ignored. But spaces inside or after items will be
// kept.
//
// The vec_free function will free the memory allocated for each item automatically
// since the vec.should_free_each_item flag is set to true here.
static inline struct vec vec_create_from_csv(const char* csv) {
  struct vec list = vec_create(sizeof(char*));
  list.should_free_each_item = true;

  const char *l = csv, *r;
  while (*l) {
    while (isspace(*l)) {
      ++l;
    }
    if (!*l) {
      break;
    }
    r = l;
    while (*r && *r != ',') {
      ++r;
    }
    if (r - l > 0) {
      char* item = lenstrdup(l, r - l);
      vec_append(&list, &item);
    }
    l = r;
    if (*l == ',') {
      ++l;
    }
  }
  return list;
}

static inline void vec_free(struct vec* vec) {
  if (vec->data) {
    if (vec->should_free_each_item) {
      // each item should store a pointer. We iterate thru the vector assuming
      // each item is a void*, but this works for other pointer type 
      // (e.g. char*) up-front.
      assert(vec->itemsize == sizeof(void*));
      VEC_FOREACH(vec, void*, item_ptr) {
        free(*item_ptr);
      }
    }
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
