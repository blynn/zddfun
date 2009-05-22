// Crit-bit trees and linked list.
// No parent pointers.
//
// Uses pointer casting and different structs instead of unions.
// In a trie, internal nodes never become external nodes, and vice versa.
//
// Removing linked list code and data saves a little.
//
// TODO: Suppose you want to insert something only if it's not already there.
// If you use has() and put(), you have to traverse the tree twice. It could
// be done with a single function returning void **, or by passing in a
// callback.

#define __CBT_H__

struct cbt_node_s {
  int type;
  struct cbt_node_s *left, *right;
  int critbit;
};
typedef struct cbt_node_s cbt_node_t[1];
typedef struct cbt_node_s *cbt_node_ptr;

struct cbt_leaf_s {
  int type;
  struct cbt_leaf_s *prev, *next;
  void *data;
  char *key;
};
typedef struct cbt_leaf_s cbt_leaf_t[1];
typedef struct cbt_leaf_s *cbt_leaf_ptr;

// Iterator.
typedef cbt_leaf_ptr cbt_it;

struct cbt_s {
  int count;
  cbt_node_ptr root;
  struct cbt_leaf_s *first, *last;
};
typedef struct cbt_s cbt_t[1];
typedef struct cbt_s *cbt_ptr;

void cbt_init(cbt_ptr cbt);
void cbt_clear(cbt_ptr cbt);

int cbt_has(cbt_ptr cbt, const char *key);
void *cbt_at(cbt_ptr cbt, const char *key);
cbt_it cbt_put(cbt_ptr cbt, void *data, const char *key);

static inline int cbt_count(cbt_t cbt) {
  return cbt->count;
}
void *cbt_alloc();

static inline int cbt_it_is_off(cbt_it it) {
  return !it;
}

static inline cbt_it cbt_it_first(cbt_t cbt) {
  return cbt->first;
}

static inline cbt_it cbt_it_next(cbt_it it) {
  return it->next;
}

static inline void cbt_it_put(cbt_it it, void *data) {
  it->data = data;
}

static inline void *cbt_it_data(cbt_it it) {
  return it->data;
}

static inline char *cbt_it_key(cbt_it it) {
  return it->key;
}

cbt_it cbt_it_at(cbt_ptr cbt, const char *key);

void cbt_it_forall(cbt_t cbt, void (*fn)(cbt_it));
void cbt_forall(cbt_t cbt, void (*fn)(void *data, const char *key));

void *cbt_remove(cbt_ptr cbt, const char *key);
void cbt_remove_all(cbt_ptr cbt);
void cbt_remove_all_with(cbt_ptr cbt, void (*fn)(void *data, const char *key));

static inline void cbt_clear_with(cbt_ptr cbt,
    void (*fn)(void *data, const char *key)) {
  cbt_remove_all_with(cbt, fn);
}

cbt_it cbt_put_with(cbt_ptr cbt, void *(*fn)(void *), const char *key);

// Alternative mode: all keys are the same length but can contain any data.
// e.g. SHA1 hashes. Do not mix with ASCIIZ keys!
int cbt_has_u(cbt_ptr cbt, const unsigned char *key, int len);
void *cbt_at_u(cbt_ptr cbt, const unsigned char *key, int len);
cbt_leaf_ptr cbt_put_u(cbt_ptr cbt, void *data,
    const unsigned char *key, int len);

// Finds or creates an entry with the given key and writes it to *it.
// Returns 1 if a new cbt_it was created.
int cbt_it_insert_u(cbt_it *it,
    cbt_ptr cbt, const unsigned char *key, int len);

cbt_it cbt_it_at_u(cbt_ptr cbt, const unsigned char *key, int len);

// Faster than cbt_at_u if you know that the key is definitely in the tree.
void *cbt_sure_at_u(cbt_ptr cbt, const unsigned char *key, int len);
