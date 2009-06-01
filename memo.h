// Crit-bit tree.
// No parent pointers. No linked list.
//
// Uses pointer casting and different structs instead of unions.
// In a trie, internal nodes never become external nodes, and vice versa.

struct memo_node_s {
  char type;
  short critbit;
  struct memo_node_s *left, *right;
};
typedef struct memo_node_s memo_node_t[1];
typedef struct memo_node_s *memo_node_ptr;

struct memo_leaf_s {
  char type;
  void *data;
  char *key;
};
typedef struct memo_leaf_s memo_leaf_t[1];
typedef struct memo_leaf_s *memo_leaf_ptr;

// Iterator.
typedef memo_leaf_ptr memo_it;

struct memo_s {
  memo_node_ptr root;
};
typedef struct memo_s memo_t[1];
typedef struct memo_s *memo_ptr;

void memo_clear(memo_ptr memo);

static inline void memo_init(memo_ptr memo) {
  memo->root = NULL;
}

int memo_has(memo_ptr memo, const char *key);
void *memo_at(memo_ptr memo, const char *key);
memo_it memo_put(memo_ptr memo, void *data, const char *key);

void *memo_alloc();

static inline int memo_it_is_off(memo_it it) {
  return !it;
}

static inline void* memo_it_put(memo_it it, void *data) {
  return it->data = data;
}

static inline void *memo_it_data(memo_it it) {
  return it->data;
}

static inline char *memo_it_key(memo_it it) {
  return it->key;
}

memo_it memo_it_at(memo_ptr memo, const char *key);

void memo_forall(memo_t memo, void (*fn)(void *data, const char *key));

void *memo_remove(memo_ptr memo, const char *key);
void memo_remove_all(memo_ptr memo);
void memo_remove_all_with(memo_ptr memo, void (*fn)(void *data, const char *key));

static inline void memo_clear_with(memo_ptr memo,
    void (*fn)(void *data, const char *key)) {
  memo_remove_all_with(memo, fn);
}

memo_it memo_put_with(memo_ptr memo, void *(*fn)(void *), const char *key);

// Alternative mode: all keys are the same length but can contain any data.
// e.g. SHA1 hashes. Do not mix with ASCIIZ keys!
int memo_has_u(memo_ptr memo, const unsigned char *key, int len);
void *memo_at_u(memo_ptr memo, const unsigned char *key, int len);
memo_leaf_ptr memo_put_u(memo_ptr memo, void *data,
    const unsigned char *key, int len);

// Finds or creates an entry with the given key and writes it to *it.
// Returns 1 if a new memo_it was created.
int memo_it_insert_u(memo_it *it,
    memo_ptr memo, const unsigned char *key, int len);

memo_it memo_it_at_u(memo_ptr memo, const unsigned char *key, int len);

// Faster than memo_at_u if you know that the key is definitely in the tree.
void *memo_sure_at_u(memo_ptr memo, const unsigned char *key, int len);

// Finds or creates an entry with the given key and writes it to *it.
// Returns 1 if a new memo_it was created.
int memo_it_insert(memo_it *it, memo_ptr memo, const char *key);
