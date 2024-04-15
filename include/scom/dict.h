#pragma once

/*
 * A generic dictionary implemention in C. Following are some of the requirements:
 * - different key type:
 *   - key type is string and all keys need to be freed in destruction.
 *   - key type is int and there is no need to free keys in destruction.
 *   - customize the hash function and equality function
 * - different value type:
 *   - int
 *   - a struct: store pointers for a struct rather than the struct itself
 * - internal/external key/value storage
 *   - if key/value type is within a pointer size, store them inline; otherwise
 *     allocate memory from heap and store a pointer for key/value instead.
 *     The user of the library should allocate memory themselves and pass
 *     in the pointer. The library will free the pointers in the dtor
 *     function.
 *   - an alternative is that we enable the library storing a struct inline.
 *     That can save some malloc calls but make the API complex
 *     and may also be bad for cache locality.
 * TODO: does not support deletion yet since there is no such usage in my
 * other projects using this library yet. They may be added when needed.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "util.h"

#define DICT_FOREACH(dict_ptr, entry_ptr) \
  for (struct dict_entry *entry_ptr = dict_begin(dict_ptr); \
      entry_ptr != dict_end(dict_ptr); \
      entry_ptr = dict_next(dict_ptr, entry_ptr))

/*
 * Invariants:
 * - free slots don't have memory allocated for key/val
 * - allocated slots have memory allocated for key/val if the dict is configured
 *   to do memory allocation for key/val.
 */
enum {
	FREE = 0, // free slot. FREE should be 0 so use calloc to allocate the initial buffer does not need extra change to setup the entry flags.
	ALLOCATED, // allocated slot
};

struct dict_entry {
	void *key, *val;
	int flags;
};

typedef uint32_t (*hash_fn_t)(void *key);
// return truth value iff lhs 'equals' rhs
typedef bool (*eq_fn_t)(void *lhs, void* rhs);

static uint32_t str_hash_fn(void *_key) {
	const char* key = (const char*) _key;
  uint32_t h = 0;
  for (int i = 0; key[i]; ++i) {
    h = h * 23 + key[i];
  }
  return h;
}

static bool str_eq_fn(void* _lhs, void* _rhs) {
	const char* lhs = (const char*) _lhs;
	const char* rhs = (const char*) _rhs;
	return strcmp(lhs, rhs) == 0;
}

struct dict {
	struct dict_entry* entries;
	int size;
	int capacity;
	hash_fn_t hash_fn;
	eq_fn_t eq_fn;
	bool should_free_key;
	bool should_free_val;
};

static struct dict dict_create(hash_fn_t hash_fn, eq_fn_t eq_fn, bool should_free_key, bool should_free_val) {
	struct dict dict;
	dict.size = 0;
	dict.capacity = 64;
	dict.entries = (struct dict_entry*) calloc(dict.capacity, sizeof(struct dict_entry));
	assert(hash_fn);
	assert(eq_fn);
	dict.hash_fn = hash_fn;
	dict.eq_fn = eq_fn;
	dict.should_free_key = should_free_key;
	dict.should_free_val = should_free_val;
	return dict;
}

/*
 * Create an dict with 'char*' key and int value.
 */
static struct dict dict_create_str_int() {
	return dict_create(str_hash_fn, str_eq_fn, 1, 0);
}

/*
 * Locate the key in the entry list.
 * If the key exists, then return the entry allocated for it;
 * Otherwise, return the entry that should be allocated for the key if
 * we insert the key before further mutating to the dictionary.
 *
 * Return NULL if the dictionary is full and the key does not exist in the
 * dictionary.
 */
static struct dict_entry* _dict_locate(struct dict* dict, void *key) {
	int h = dict->hash_fn(key) % dict->capacity;
	int cnt = 0;
	while (dict->entries[h].flags == ALLOCATED && !dict->eq_fn(key, dict->entries[h].key)) {
		if (++cnt == dict->capacity) {
			// dictionary is full and the key is not found. return NULL
			return NULL;
		}
		++h;
		if (h == dict->capacity) {
			h = 0;
		}
	}
	return &(dict->entries[h]);
}

/*
 * Return the dict_entry if key is found; Return NULL otherwise.
 */
static struct dict_entry* dict_find(struct dict* dict, void *key) {
	struct dict_entry* entry = _dict_locate(dict, key);
	if (entry && entry->flags == ALLOCATED) {
		assert(dict->eq_fn(key, entry->key));
		return entry;
	} else {
		return NULL;
	}
}

/*
 * The key must exist in the dictionary. Return the value.
 */
static void *dict_find_nomiss(struct dict* dict, void *key) {
	struct dict_entry* entry = dict_find(dict, key);
	if (!entry) {
		printf("Expect the key to exist but it's missing\n");
	}
	assert(entry && entry->flags == ALLOCATED && dict->eq_fn(key, entry->key));
	return entry->val;
}

static void _dict_expand(struct dict* dict) {
  struct dict_entry* oldentries = dict->entries;
	int oldcapacity = dict->capacity;
  int newcapacity = (dict->capacity << 1);
  struct dict_entry* newentries = (struct dict_entry*) calloc(newcapacity, sizeof(struct dict_entry));

	// set capacity and entries before calling _dict_locate since _dict_locate
	// need access these fields.
  dict->capacity = newcapacity;
  dict->entries = newentries;

  for (int i = 0; i < oldcapacity; ++i) {
    struct dict_entry* src_entry = oldentries + i;
    if (src_entry->flags == ALLOCATED) {
      struct dict_entry* dst_entry = _dict_locate(dict, src_entry->key);
      dst_entry->key = src_entry->key;
      dst_entry->val = src_entry->val;
			dst_entry->flags = ALLOCATED;
    }
  }

  free(oldentries);
}

/*
 * Insert if the key is not added yet; update if the key already exits.
 * Return the number of entries created.
 */
static int dict_put(struct dict* dict, void *key, void *val) {
	// at most 50% full
	if ((dict->size << 1) >= dict->capacity) {
		_dict_expand(dict);
	}
	struct dict_entry* entry = _dict_locate(dict, key);
	assert(entry);
	if (entry->flags == ALLOCATED) {
		// update
		if (dict->should_free_key) {
			free(entry->key);
		}
		if (dict->should_free_val) {
			free(entry->val);
		}
		entry->key = key;
		entry->val = val;
		return 0;
	} else {
		// insert
		assert(entry->flags == FREE);
		entry->key = key;
		entry->val = val;
		entry->flags = ALLOCATED;
		++dict->size;
		return 1;
	}
}

static void dict_free(struct dict* dict) {
  for (int i = 0; i < dict->capacity; ++i) {
		struct dict_entry* entry = &dict->entries[i];
		if (entry->flags == ALLOCATED) {
			if (dict->should_free_key) {
				free(entry->key);
			}
			if (dict->should_free_val) {
				free(entry->val);
			}
		}
  }
  free(dict->entries);
}

static struct dict_entry *dict_end(struct dict *dict) {
  return dict->entries + dict->capacity;
}

/*
 * Skip unused entries until the first used entry or the end.
 */
struct dict_entry *_dict_skip_unused(struct dict *dict, struct dict_entry *cur) {
  while (cur != dict_end(dict) && cur->flags == FREE) {
    ++cur;
  }
  return cur;
}

static struct dict_entry *dict_next(struct dict *dict, struct dict_entry *cur) {
  assert(cur != dict_end(dict));
  return _dict_skip_unused(dict, cur + 1);
}

/*
 * If the dict is not empty, return the pointer to the first entry, otherwise
 * return end.
 */
static struct dict_entry *dict_begin(struct dict *dict) {
  return _dict_skip_unused(dict, dict->entries);
}
