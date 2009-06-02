#include <stdlib.h>
#include <string.h>
#include "memo.h"

#define NDEBUG
#include <assert.h>

enum {
  put_stack_size = 48,
};

enum memo_type {
  MEMO_NODE,
  MEMO_LEAF,
};

void memo_node_free(memo_node_ptr t) {
  if (!t) return;
  if (MEMO_LEAF == t->type) {
    free(((memo_leaf_ptr) t)->key);
  } else {
    memo_node_free(t->left);
    memo_node_free(t->right);
  }
  free(t);
}

void *memo_alloc() {
  return malloc(sizeof(struct memo_s));
}

void memo_clear(memo_ptr memo) {
  memo_node_free(memo->root);
}

static inline int chartestbit(char c, int bit) {
  // Note we label the most significant bit 0, and the least 7.
  return (1 << (7 - bit)) & c;
}

static int firstcritbit(const char *key0, const char *key1) {
  const char *cp0 = key0;
  const char *cp1 = key1;
  int bit;

  while(*cp0 == *cp1) {
    if (!*cp0) {
      return 0;
    }
    cp0++;
    cp1++;
  }

  char c = *cp0 ^ *cp1;
  for (bit = 7; !(c >> bit); bit--);
  // Subtract bit from 7 because we number them the other way.
  // Add 1 because we want to use the sign as an extra bit of information.
  // We'll subtract 1 from it later.
  int critbit = ((cp0 - key0) << 3) + 7 - bit + 1;
  if ((*cp0 >> bit) & 1) return critbit;
  return -critbit;
}

static inline int testbit(const char *key, int bit) {
  return chartestbit(key[bit >> 3], (bit & 7));
}

static memo_leaf_ptr memo_leaf_at(memo_node_ptr n, const char *key) {
  memo_node_ptr p = n;
  int len = (strlen(key) << 3) - 1;
  for (;;) {
    if (MEMO_LEAF == p->type) {
      return (memo_leaf_ptr) p;
    }
    if (len < p->critbit) {
      do {
	p = p->left;
      } while (MEMO_NODE == p->type);
      return (memo_leaf_ptr) p;
    }
    if (testbit(key, p->critbit)) {
      p = p->right;
    } else {
      p = p->left;
    }
  }
}

int memo_has(memo_ptr memo, const char *key) {
  if (!memo->root) return 0;
  memo_leaf_ptr p = memo_leaf_at(memo->root, key);
  return (!strcmp(p->key, key));
}

memo_it memo_it_at(memo_ptr memo, const char *key) {
  if (!memo->root) return NULL;
  memo_leaf_ptr p = memo_leaf_at(memo->root, key);
  if (!strcmp(p->key, key)) {
      return p;
  }
  return NULL;
}

void *memo_at(memo_ptr memo, const char *key) {
  memo_leaf_ptr p = memo_it_at(memo, key);
  if (!p) return NULL;
  return p->data;
}

static void memo_leaf_put(memo_leaf_ptr n, void *data, const char *key) {
  n->type = MEMO_LEAF;
  n->data = data;
  n->key = strdup(key);
}

memo_it memo_put_with(memo_ptr memo, void *(*fn)(void *), const char *key) {
  if (!memo->root) {
    memo_leaf_ptr leaf = malloc(sizeof(memo_leaf_t));
    memo_leaf_put(leaf, fn(NULL), key);
    memo->root = (memo_node_ptr) leaf;
    return leaf;
  }

  memo_node_ptr stack_space[put_stack_size];
  memo_node_ptr *stack = stack_space;
  int stack_max = put_stack_size;
  int i = 0;
  memo_node_ptr t = memo->root;
  int keylen = (strlen(key) << 3) - 1;
  stack[i] = t;
  while (MEMO_NODE == t->type) {
    // If the key is shorter than the remaining keys on this subtree,
    // we can compare it against any of them (and are guaranteed the new
    // node must be inserted above this node). However, this is uncommon thus
    // not worth optimizing. We let it loop through each time down the
    // left path.
    if (keylen < t->critbit || !testbit(key, t->critbit)) {
      t = t->left;
    } else {
      t = t->right;
    }
    i++;
    if (i == stack_max) {
      // Badly balanced tree? This is going to cost us.
      stack_max *= 2;
      if (stack == stack_space) {
	stack = malloc(stack_max * sizeof(memo_node_ptr));
	memcpy(stack, stack_space, put_stack_size * sizeof(memo_node_ptr));
      } else stack = realloc(stack, stack_max * sizeof(memo_node_ptr));
    }
    stack[i] = t;
  }

  memo_leaf_ptr leaf = (memo_leaf_ptr) t;
  int res = firstcritbit(key, leaf->key);
  if (!res) {
    leaf->data = fn(leaf->data);
    return leaf;
  }

  memo_leaf_ptr pleaf = malloc(sizeof(memo_leaf_t));
  memo_node_ptr pnode = malloc(sizeof(memo_node_t));
  memo_leaf_put(pleaf, fn(NULL), key);
  pnode->type = MEMO_NODE;
  pnode->critbit = abs(res) - 1;

  // Last pointer on stack is a leaf, hence decrement before test.
  do {
    i--;
  } while(i >= 0 && pnode->critbit < stack[i]->critbit);

  if (res > 0) {
    // Key is bigger, therefore it goes on the right.
    pnode->left = stack[i + 1];
    pnode->right = (memo_node_ptr) pleaf;
  } else {
    // Key is smaller, therefore it goes on the left.
    pnode->left = (memo_node_ptr) pleaf;
    pnode->right = stack[i + 1];
  }
  if (i < 0) {
    memo->root = pnode;
  } else {
    if (stack[i]->left == stack[i + 1]) {
      stack[i]->left = pnode;
    } else {
      stack[i]->right = pnode;
    }
  }
  if (stack != stack_space) free(stack);
  return pleaf;
}

memo_it memo_put(memo_ptr memo, void *data, const char *key) {
  void *returndata(void *p) { return data; }
  return memo_put_with(memo, returndata, key);
}

void *memo_remove(memo_ptr memo, const char *key) {
  assert(memo->root);
  // assert(memo_has(memo, key));  // TODO: write memo_has
  memo_node_ptr stack_space[put_stack_size];
  memo_node_ptr *stack = stack_space;
  int stack_max = put_stack_size;
  int i = 0;
  memo_node_ptr t = memo->root;
  stack[i] = t;
  while (MEMO_NODE == t->type) {
    assert((strlen(key) << 3) - 1 >= t->critbit);
    if (!testbit(key, t->critbit)) {
      t = t->left;
    } else {
      t = t->right;
    }
    i++;
    if (i == stack_max) {
      // Badly balanced tree? This is going to cost us.
      stack_max *= 2;
      if (stack == stack_space) {
	stack = malloc(stack_max * sizeof(memo_node_ptr));
	memcpy(stack, stack_space, put_stack_size * sizeof(memo_node_ptr));
      } else stack = realloc(stack, stack_max * sizeof(memo_node_ptr));
    }
    stack[i] = t;
  }
  memo_leaf_ptr p = (memo_leaf_ptr) t;
  if (0 == i) {  // We found it at the root.
    memo->root = NULL;
  } else {
    memo_node_ptr sibling;
    if (stack[i - 1]->left == stack[i]) {
      sibling = stack[i - 1]->right;
    } else {
      sibling = stack[i - 1]->left;
    }
    if (1 == i) {  // One-level down: reassign root.
      memo->root = sibling;
    } else {  // Reassign grandparent.
      if (stack[i - 2]->left == stack[i - 1]) {
	stack[i - 2]->left = sibling;
      } else {
	stack[i - 2]->right = sibling;
      }
    }
    free(stack[i - 1]);
  }
  free(p->key);
  void *data = p->data;
  free(p);
  return data;
}

static int firstcritbit_u(const unsigned char *key0,
                          const unsigned char *key1, int len) {
  const unsigned char *cp0 = key0, *limit = key0 + len;
  const unsigned char *cp1 = key1;

  for(;;) {
    if (cp0 == limit) return 0;
    if (*cp0 != *cp1) break;
    cp0++;
    cp1++;
  }

  int bit;

  char c = *cp0 ^ *cp1;
  for (bit = 7; !(c >> bit); bit--);
  // Subtract bit from 7 because we number them the other way.
  // Add 1 because we want to use the sign as an extra bit of information.
  // We'll subtract 1 from it later.
  int critbit = ((cp0 - key0) << 3) + 7 - bit + 1;
  if ((*cp0 >> bit) & 1) return critbit;
  return -critbit;
}

static memo_leaf_ptr memo_leaf_at_u(memo_node_ptr n,
    const unsigned char *key, int len) {
  memo_node_ptr p = n;
  int bitlen = (len << 3) - 1;
  for (;;) {
    if (MEMO_LEAF == p->type) {
      return (memo_leaf_ptr) p;
    }
    // TODO: See comment in put_u about fixed length keys.
    if (bitlen < p->critbit) {
      do {
	  p = p->left;
      } while (MEMO_NODE == p->type);
      return (memo_leaf_ptr) p;
    }
    if (testbit((char *) key, p->critbit)) {
      p = p->right;
    } else {
      p = p->left;
    }
  }
}

void *memo_at_u(memo_ptr memo, const unsigned char *key, int len) {
  if (!memo->root) return NULL;
  memo_leaf_ptr p = memo_leaf_at_u(memo->root, key, len);
  if (!memcmp(p->key, key, len)) {
      return p->data;
  }
  return NULL;
}

void *memo_sure_at_u(memo_ptr memo, const unsigned char *key, int len) {
  assert(memo_has_u(memo, key, len));
  return memo_leaf_at_u(memo->root, key, len)->data;
}

static void memo_leaf_put_u(memo_leaf_ptr n, void *data,
    const unsigned char *key, int len) {
  n->type = MEMO_LEAF;
  n->data = data;
  n->key = malloc(len);
  memcpy(n->key, key, len);
}

memo_leaf_ptr memo_put_u(memo_ptr memo,
    void *data, const unsigned char *key, int len) {
  if (!memo->root) {
    memo_leaf_ptr leaf = malloc(sizeof(memo_leaf_t));
    memo_leaf_put_u(leaf, data, key, len);
    memo->root = (memo_node_ptr) leaf;
    return leaf;
  }

  memo_node_ptr stack_space[put_stack_size];
  memo_node_ptr *stack = stack_space;
  int stack_max = put_stack_size;
  int i = 0;
  memo_node_ptr t = memo->root;
  stack[i] = t;
  i++;
  while (MEMO_NODE == t->type) {
    // No length check: all keys should be the same length.
    if (!testbit((char *) key, t->critbit)) {
      t = t->left;
    } else {
      t = t->right;
    }
    if (i == stack_max) {
      // Badly balanced tree? This is going to cost us.
      stack_max *= 2;
      if (stack == stack_space) {
	stack = malloc(stack_max * sizeof(memo_node_ptr));
	memcpy(stack, stack_space, put_stack_size * sizeof(memo_node_ptr));
      } else stack = realloc(stack, stack_max * sizeof(memo_node_ptr));
    }
    stack[i] = t;
    i++;
  }

  memo_leaf_ptr leaf = (memo_leaf_ptr) t;
  int res = firstcritbit_u(key, (unsigned char *) leaf->key, len);
  if (!res) {
    leaf->data = data;
    return leaf;
  }

  memo_leaf_ptr pleaf = malloc(sizeof(memo_leaf_t));
  memo_node_ptr pnode = malloc(sizeof(memo_node_t));
  memo_leaf_put_u(pleaf, data, (unsigned char *) key, len);
  pnode->type = MEMO_NODE;
  pnode->critbit = abs(res) - 1;

  // Decrement i so it points to top of stack.
  i--;
  // Last pointer on stack is a leaf, so decrement before test.
  do {
    i--;
  } while(i >= 0 && pnode->critbit < stack[i]->critbit);

  if (res > 0) {
    // Key is bigger, therefore it goes on the right.
    pnode->left = stack[i + 1];
    pnode->right = (memo_node_ptr) pleaf;
  } else {
    // Key is smaller, therefore it goes on the left.
    pnode->left = (memo_node_ptr) pleaf;
    pnode->right = stack[i + 1];
  }
  if (i < 0) {
    memo->root = pnode;
  } else {
    if (stack[i]->left == stack[i + 1]) {
      stack[i]->left = pnode;
    } else {
      stack[i]->right = pnode;
    }
  }
  if (stack != stack_space) free(stack);
  return pleaf;
}

static void remove_all_recurse(memo_node_ptr t,
    void (*fn)(void *, const char *)) {
  if (MEMO_LEAF == t->type) {
    memo_leaf_ptr p = (memo_leaf_ptr) t;
    if (fn) fn(p->data, p->key);
    free(p->key);
    free(p);
    return;
  }
  remove_all_recurse(t->left, fn);
  remove_all_recurse(t->right, fn);
  free(t);
}

void memo_remove_all_with(memo_ptr memo, void (*fn)(void *data, const char *key)) {
  if (memo->root) {
    remove_all_recurse(memo->root, fn);
    memo->root = NULL;
  }
}

void memo_remove_all(memo_ptr memo) {
  memo_remove_all_with(memo, NULL);
}

void memo_forall(memo_t memo, void (*fn)(void *data, const char *key)) {
  void recurse(memo_node_ptr n) {
    if (MEMO_LEAF == n->type) {
      memo_leaf_ptr p = (memo_leaf_ptr) n;
      fn(p->data, p->key);
    } else {
      recurse(n->left);
      recurse(n->right);
    }
  }
  if (memo->root) {
    recurse(memo->root);
  }
}

int memo_has_u(memo_ptr memo, const unsigned char *key, int len) {
  if (!memo->root) return 0;
  memo_leaf_ptr p = memo_leaf_at_u(memo->root, key, len);
  return (!memcmp(p->key, key, len));
}

memo_it memo_it_at_u(memo_ptr memo, const unsigned char *key, int len) {
  if (!memo->root) return NULL;
  memo_leaf_ptr p = memo_leaf_at_u(memo->root, key, len);
  return memcmp(p->key, key, len) ? NULL : p;
}

int memo_it_insert_u(memo_it *it,
    memo_ptr memo, const unsigned char *key, int len) {
  if (!memo->root) {
    memo_leaf_ptr leaf = malloc(sizeof(memo_leaf_t));
    memo_leaf_put_u(leaf, NULL, key, len);
    memo->root = (memo_node_ptr) leaf;
    *it = leaf;
    return 1;
  }

  // The stack may seem cumbersome, but it seems to beat simpler solutions:
  // it appears we usually backtrack only a little, so rather than start from
  // the root again, we walk back up the tree.
  memo_node_ptr stack_space[put_stack_size];
  memo_node_ptr *stack = stack_space;
  int stack_max = put_stack_size;
  int i = 0;
  memo_node_ptr t = memo->root;
  stack[i] = t;
  while (MEMO_NODE == t->type) {
    // No length check: all keys should be the same length.
    if (!testbit((char *) key, t->critbit)) {
      t = t->left;
    } else {
      t = t->right;
    }
    i++;
    if (i == stack_max) {
      // Badly balanced tree? This is going to cost us.
      stack_max *= 2;
      if (stack == stack_space) {
	stack = malloc(stack_max * sizeof(memo_node_ptr));
	memcpy(stack, stack_space, put_stack_size * sizeof(memo_node_ptr));
      } else stack = realloc(stack, stack_max * sizeof(memo_node_ptr));
    }
    stack[i] = t;
  }

  memo_leaf_ptr leaf = (memo_leaf_ptr) t;
  int res = firstcritbit_u(key, (unsigned char *) leaf->key, len);
  if (!res) {
    *it = leaf;
    return 0;
  }

  memo_leaf_ptr pleaf = malloc(sizeof(memo_leaf_t));
  memo_node_ptr pnode = malloc(sizeof(memo_node_t));
  memo_leaf_put_u(pleaf, NULL, (unsigned char *) key, len);
  pnode->type = MEMO_NODE;
  pnode->critbit = abs(res) - 1;

  // Walk back up tree to find place to insert new node.
  while(i > 0 && pnode->critbit < stack[i - 1]->critbit) i--;

  if (res > 0) {
    // Key is bigger, therefore it goes on the right.
    pnode->left = stack[i];
    pnode->right = (memo_node_ptr) pleaf;
  } else {
    // Key is smaller, therefore it goes on the left.
    pnode->left = (memo_node_ptr) pleaf;
    pnode->right = stack[i];
  }
  if (!i) {
    memo->root = pnode;
  } else {
    if (stack[i - 1]->left == stack[i]) {
      stack[i - 1]->left = pnode;
    } else {
      stack[i - 1]->right = pnode;
    }
  }
  if (stack != stack_space) free(stack);
  *it = pleaf;
  return 1;
}

int memo_it_insert(memo_it *it, memo_ptr memo, const char *key) {
  if (!memo->root) {
    memo_leaf_ptr leaf = malloc(sizeof(memo_leaf_t));
    memo_leaf_put(leaf, NULL, key);
    memo->root = (memo_node_ptr) leaf;
    *it = leaf;
    return 1;
  }

  // The stack may seem cumbersome, but it seems to beat simpler solutions:
  // it appears we usually backtrack only a little, so rather than start from
  // the root again, we walk back up the tree.
  memo_node_ptr stack_space[put_stack_size];
  memo_node_ptr *stack = stack_space;
  int stack_max = put_stack_size;
  int i = 0;
  int keylen = (strlen(key) << 3) - 1;
  memo_node_ptr t = memo->root;
  stack[i] = t;
  while (MEMO_NODE == t->type) {
    // If the key is shorter than the remaining keys on this subtree,
    // we can compare it against any of them (and are guaranteed the new
    // node must be inserted above this node). However, this is uncommon thus
    // not worth optimizing. We let it loop through each time down the
    // left path.
    if (keylen < t->critbit || !testbit(key, t->critbit)) {
      t = t->left;
    } else {
      t = t->right;
    }
    i++;
    if (i == stack_max) {
      // Badly balanced tree? This is going to cost us.
      stack_max *= 2;
      if (stack == stack_space) {
	stack = malloc(stack_max * sizeof(memo_node_ptr));
	memcpy(stack, stack_space, put_stack_size * sizeof(memo_node_ptr));
      } else stack = realloc(stack, stack_max * sizeof(memo_node_ptr));
    }
    stack[i] = t;
  }

  memo_leaf_ptr leaf = (memo_leaf_ptr) t;
  int res = firstcritbit(key, leaf->key);
  if (!res) {
    *it = leaf;
    return 0;
  }

  memo_leaf_ptr pleaf = malloc(sizeof(memo_leaf_t));
  memo_node_ptr pnode = malloc(sizeof(memo_node_t));
  memo_leaf_put(pleaf, NULL, key);
  pnode->type = MEMO_NODE;
  pnode->critbit = abs(res) - 1;

  // Walk back up tree to find place to insert new node.
  while(i > 0 && pnode->critbit < stack[i - 1]->critbit) i--;

  if (res > 0) {
    // Key is bigger, therefore it goes on the right.
    pnode->left = stack[i];
    pnode->right = (memo_node_ptr) pleaf;
  } else {
    // Key is smaller, therefore it goes on the left.
    pnode->left = (memo_node_ptr) pleaf;
    pnode->right = stack[i];
  }
  if (!i) {
    memo->root = pnode;
  } else {
    if (stack[i - 1]->left == stack[i]) {
      stack[i - 1]->left = pnode;
    } else {
      stack[i - 1]->right = pnode;
    }
  }
  if (stack != stack_space) free(stack);
  *it = pleaf;
  return 1;
}
