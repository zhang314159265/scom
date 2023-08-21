#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

// variable length string
struct str {
  int capacity;
	int len;
	char *buf;
};

/*
 * init_capa == 0 will not result in a malloc.
 * A str_free is not necessary in this case.
 */
static inline struct str str_create(int init_capa) {
	struct str str;
	str.capacity = init_capa;
	str.len = 0;
  if (str.capacity > 0) {
  	str.buf = (char*) malloc(str.capacity);
  } else {
    str.buf = NULL;
  }
	return str;
}

/*
 * Ensure there is enough space for at least one more byte.
 */
static inline void _str_ensure_space(struct str* pstr) {
	if (pstr->len == pstr->capacity) {
		if (pstr->capacity == 0) {
			pstr->capacity = 16;
		} else {
			pstr->capacity <<= 1;
		}
		pstr->buf = (char*) realloc(pstr->buf, pstr->capacity);
	}
}

/*
 * Append a character to the str object.
 */
static inline void str_append(struct str* pstr, char ch) {
	assert(pstr->len <= pstr->capacity);
	_str_ensure_space(pstr);
	assert(pstr->len < pstr->capacity);
	pstr->buf[pstr->len++] = ch;
}

static inline void str_free(struct str* pstr) {
  if (pstr->buf) {
	  free(pstr->buf);
    pstr->buf = NULL;
  }
}

static inline int str_lenconcat(struct str* str, const char* extra, int n) {
	int oldlen = str->len;
  for (int i = 0; i < n; ++i) {
    str_append(str, extra[i]);
  }
	return oldlen;
}

/*
 * Concat an cstr (with training '\0') and return the length before
 * concating.
 *
 * This is useful in scenario like maintaining an .strtab in ELF file.
 * When concating a new name, the returned length will be the offset for
 * that name in the .strtab.
 *
 * Note that, concating multiple C-string will resulting in multiple
 * '\0' in the 'struct str' object.
 */
static inline int str_concat(struct str* str, const char* extra) {
  return str_lenconcat(str, extra, strlen(extra) + 1);
}

/*
 * Append a character 'n' times.
 */
static inline void str_nappend(struct str* pstr, int n, char ch) {
  for (int i = 0; i < n; ++i) {
    str_append(pstr, ch);
  }
}

/*
 * Implement a move semantic to release the ownership of the buffer
 * from 'pstr' and assign the ownership to the returned 'struct str'.
 */
static struct str str_move(struct str* pstr) {
  struct str ret = *pstr;
	memset(pstr, 0, sizeof(*pstr));
  return ret;
}
