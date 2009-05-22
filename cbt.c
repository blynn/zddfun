#include <stdlib.h>
#include <string.h>
#include "cbt.h"

#define NDEBUG
#include <assert.h>

enum {
  put_stack_size = 48,
};

enum cbt_type {
  CBT_NODE,
  CBT_LEAF,
};

void cbt_node_free(cbt_node_ptr t) {
  if (!t) return;
  if (CBT_LEAF == t->type) {
    free(((cbt_leaf_ptr) t)->key);
  } else {
    cbt_node_free(t->left);
    cbt_node_free(t->right);
  }
  free(t);
}

void cbt_init(cbt_ptr cbt) {
  cbt->count = 0;
  // Maximum depth reached, until cleared. Maintaining the true maximum
  // depth would be expensive because of deletion, though it could
  // be done by maintaining an array of integers.
  cbt->root = NULL;
  cbt->first = NULL;
  cbt->last = NULL;
}

void *cbt_alloc() {
  return malloc(sizeof(struct cbt_s));
}

void cbt_clear(cbt_ptr cbt) {
  cbt_node_free(cbt->root);
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

static int testbit(const char *key, int bit) {
  int i = bit >> 3;
  return chartestbit(key[i], (bit & 7));
}

static cbt_leaf_ptr cbt_leaf_at(cbt_node_ptr n, const char *key) {
  cbt_node_ptr p = n;
  int len = (strlen(key) << 3) - 1;
  for (;;) {
    if (CBT_LEAF == p->type) {
      return (cbt_leaf_ptr) p;
    }
    if (len < p->critbit) {
      do {
	  p = p->left;
      } while (CBT_NODE == p->type);
      return (cbt_leaf_ptr) p;
    }
    if (testbit(key, p->critbit)) {
      p = p->right;
    } else {
      p = p->left;
    }
  }
}

int cbt_has(cbt_ptr cbt, const char *key) {
  if (!cbt->root) return 0;
  cbt_leaf_ptr p = cbt_leaf_at(cbt->root, key);
  return (!strcmp(p->key, key));
}

cbt_it cbt_it_at(cbt_ptr cbt, const char *key) {
  if (!cbt->root) return NULL;
  cbt_leaf_ptr p = cbt_leaf_at(cbt->root, key);
  if (!strcmp(p->key, key)) {
      return p;
  }
  return NULL;
}

void *cbt_at(cbt_ptr cbt, const char *key) {
  cbt_leaf_ptr p = cbt_it_at(cbt, key);
  if (!p) return NULL;
  return p->data;
}

static void cbt_leaf_put(cbt_leaf_ptr n, void *data, const char *key) {
  n->type = CBT_LEAF;
  n->data = data;
  n->key = strdup(key);
}

cbt_it cbt_put_with(cbt_ptr cbt, void *(*fn)(void *), const char *key) {
  if (!cbt->root) {
    cbt_leaf_ptr leaf = malloc(sizeof(cbt_leaf_t));
    cbt_leaf_put(leaf, fn(NULL), key);
    cbt->root = (cbt_node_ptr) leaf;
    cbt->first = leaf;
    cbt->last = leaf;
    leaf->next = NULL;
    leaf->prev = NULL;
    cbt->count++;
    return leaf;
  }

  cbt_node_ptr stack_space[put_stack_size];
  cbt_node_ptr *stack = stack_space;
  int stack_max = put_stack_size;
  int i = 0;
  cbt_node_ptr t = cbt->root;
  int keylen = (strlen(key) << 3) - 1;
  stack[i] = t;
  while (CBT_NODE == t->type) {
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
	stack = malloc(stack_max * sizeof(cbt_node_ptr));
	memcpy(stack, stack_space, put_stack_size * sizeof(cbt_node_ptr));
      } else stack = realloc(stack, stack_max * sizeof(cbt_node_ptr));
    }
    stack[i] = t;
  }

  cbt_leaf_ptr leaf = (cbt_leaf_ptr) t;
  int res = firstcritbit(key, leaf->key);
  if (!res) {
    leaf->data = fn(leaf->data);
    return leaf;
  }

  cbt->count++;
  cbt_leaf_ptr pleaf = malloc(sizeof(cbt_leaf_t));
  cbt_node_ptr pnode = malloc(sizeof(cbt_node_t));
  cbt_leaf_put(pleaf, fn(NULL), key);
  pnode->type = CBT_NODE;
  pnode->critbit = abs(res) - 1;

  // Last pointer on stack is a leaf, hence decrement before test.
  do {
    i--;
  } while(i >= 0 && pnode->critbit < stack[i]->critbit);

  if (res > 0) {
    // Key is bigger, therefore it goes on the right.
    pnode->left = stack[i + 1];
    pnode->right = (cbt_node_ptr) pleaf;
    // The rightmost child of the left subtree must be the predecessor.
    for (t = pnode->left; t->type == CBT_NODE; t = t->right);
    cbt_leaf_ptr leaf = (cbt_leaf_ptr) t;
    pleaf->next = leaf->next;
    pleaf->prev = leaf;
    if (leaf->next) leaf->next->prev = pleaf;
    else cbt->last = pleaf;
    leaf->next = pleaf;
  } else {
    // Key is smaller, therefore it goes on the left.
    pnode->left = (cbt_node_ptr) pleaf;
    pnode->right = stack[i + 1];
    // The leftmost child of the right subtree must be the successor.
    for (t = pnode->right; t->type == CBT_NODE; t = t->left);
    cbt_leaf_ptr leaf = (cbt_leaf_ptr) t;
    pleaf->prev = leaf->prev;
    pleaf->next = leaf;
    if (leaf->prev) leaf->prev->next = pleaf;
    else cbt->first = pleaf;
    leaf->prev = pleaf;
  }
  if (i < 0) {
    cbt->root = pnode;
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

cbt_it cbt_put(cbt_ptr cbt, void *data, const char *key) {
  void *returndata(void *p) { return data; }
  return cbt_put_with(cbt, returndata, key);
}

void *cbt_remove(cbt_ptr cbt, const char *key) {
  assert(cbt->root);
  // assert(cbt_has(cbt, key));  // TODO: write cbt_has
  cbt_node_ptr stack_space[put_stack_size];
  cbt_node_ptr *stack = stack_space;
  int stack_max = put_stack_size;
  int i = 0;
  cbt_node_ptr t = cbt->root;
  stack[i] = t;
  while (CBT_NODE == t->type) {
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
	stack = malloc(stack_max * sizeof(cbt_node_ptr));
	memcpy(stack, stack_space, put_stack_size * sizeof(cbt_node_ptr));
      } else stack = realloc(stack, stack_max * sizeof(cbt_node_ptr));
    }
    stack[i] = t;
  }
  cbt->count--;
  cbt_leaf_ptr p = (cbt_leaf_ptr) t;
  if (0 == i) {  // We found it at the root.
    cbt->root = NULL;
  } else {
    cbt_node_ptr sibling;
    if (stack[i - 1]->left == stack[i]) {
      sibling = stack[i - 1]->right;
    } else {
      sibling = stack[i - 1]->left;
    }
    if (1 == i) {  // One-level down: reassign root.
      cbt->root = sibling;
    } else {  // Reassign grandparent.
      if (stack[i - 2]->left == stack[i - 1]) {
	stack[i - 2]->left = sibling;
      } else {
	stack[i - 2]->right = sibling;
      }
    }
    free(stack[i - 1]);
  }
  if (p->next) p->next->prev = p->prev;
  else cbt->last = p->prev;
  if (p->prev) p->prev->next = p->next;
  else cbt->first = p->next;
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

static cbt_leaf_ptr cbt_leaf_at_u(cbt_node_ptr n,
    const unsigned char *key, int len) {
  cbt_node_ptr p = n;
  int bitlen = (len << 3) - 1;
  for (;;) {
    if (CBT_LEAF == p->type) {
      return (cbt_leaf_ptr) p;
    }
    // TODO: See comment in put_u about fixed length keys.
    if (bitlen < p->critbit) {
      do {
	  p = p->left;
      } while (CBT_NODE == p->type);
      return (cbt_leaf_ptr) p;
    }
    if (testbit((char *) key, p->critbit)) {
      p = p->right;
    } else {
      p = p->left;
    }
  }
}

void *cbt_at_u(cbt_ptr cbt, const unsigned char *key, int len) {
  if (!cbt->root) return NULL;
  cbt_leaf_ptr p = cbt_leaf_at_u(cbt->root, key, len);
  if (!memcmp(p->key, key, len)) {
      return p->data;
  }
  return NULL;
}

void *cbt_sure_at_u(cbt_ptr cbt, const unsigned char *key, int len) {
  assert(cbt_has_u(cbt, key, len));
  return cbt_leaf_at_u(cbt->root, key, len)->data;
}

static void cbt_leaf_put_u(cbt_leaf_ptr n, void *data,
    const unsigned char *key, int len) {
  n->type = CBT_LEAF;
  n->data = data;
  n->key = malloc(len);
  memcpy(n->key, key, len);
}

cbt_leaf_ptr cbt_put_u(cbt_ptr cbt,
    void *data, const unsigned char *key, int len) {
  if (!cbt->root) {
    cbt_leaf_ptr leaf = malloc(sizeof(cbt_leaf_t));
    cbt_leaf_put_u(leaf, data, key, len);
    cbt->root = (cbt_node_ptr) leaf;
    cbt->first = leaf;
    cbt->last = leaf;
    leaf->next = NULL;
    leaf->prev = NULL;
    cbt->count++;
    return leaf;
  }

  cbt_node_ptr stack_space[put_stack_size];
  cbt_node_ptr *stack = stack_space;
  int stack_max = put_stack_size;
  int i = 0;
  cbt_node_ptr t = cbt->root;
  stack[i] = t;
  i++;
  while (CBT_NODE == t->type) {
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
	stack = malloc(stack_max * sizeof(cbt_node_ptr));
	memcpy(stack, stack_space, put_stack_size * sizeof(cbt_node_ptr));
      } else stack = realloc(stack, stack_max * sizeof(cbt_node_ptr));
    }
    stack[i] = t;
    i++;
  }

  cbt_leaf_ptr leaf = (cbt_leaf_ptr) t;
  int res = firstcritbit_u(key, (unsigned char *) leaf->key, len);
  if (!res) {
    leaf->data = data;
    return leaf;
  }

  cbt->count++;
  cbt_leaf_ptr pleaf = malloc(sizeof(cbt_leaf_t));
  cbt_node_ptr pnode = malloc(sizeof(cbt_node_t));
  cbt_leaf_put_u(pleaf, data, (unsigned char *) key, len);
  pnode->type = CBT_NODE;
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
    pnode->right = (cbt_node_ptr) pleaf;
    // The rightmost child of the left subtree must be the predecessor.
    for (t = pnode->left; t->type == CBT_NODE; t = t->right);
    cbt_leaf_ptr leaf = (cbt_leaf_ptr) t;
    pleaf->next = leaf->next;
    pleaf->prev = leaf;
    if (leaf->next) leaf->next->prev = pleaf;
    else cbt->last = pleaf;
    leaf->next = pleaf;
  } else {
    // Key is smaller, therefore it goes on the left.
    pnode->left = (cbt_node_ptr) pleaf;
    pnode->right = stack[i + 1];
    // The leftmost child of the right subtree must be the successor.
    for (t = pnode->right; t->type == CBT_NODE; t = t->left);
    cbt_leaf_ptr leaf = (cbt_leaf_ptr) t;
    pleaf->prev = leaf->prev;
    pleaf->next = leaf;
    if (leaf->prev) leaf->prev->next = pleaf;
    else cbt->first = pleaf;
    leaf->prev = pleaf;
  }
  if (i < 0) {
    cbt->root = pnode;
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

static void remove_all_recurse(cbt_node_ptr t,
    void (*fn)(void *, const char *)) {
  if (CBT_LEAF == t->type) {
    cbt_leaf_ptr p = (cbt_leaf_ptr) t;
    if (fn) fn(p->data, p->key);
    free(p->key);
    free(p);
    return;
  }
  remove_all_recurse(t->left, fn);
  remove_all_recurse(t->right, fn);
  free(t);
}

void cbt_remove_all_with(cbt_ptr cbt, void (*fn)(void *data, const char *key)) {
  if (cbt->root) {
    remove_all_recurse(cbt->root, fn);
    cbt->root = NULL;
    cbt->count = 0;
    cbt->first = NULL;
    cbt->last = NULL;
  }
}

void cbt_remove_all(cbt_ptr cbt) {
  if (cbt->root) cbt_remove_all_with(cbt, NULL);
}

void cbt_it_forall(cbt_t cbt, void (*fn)(cbt_it)) {
  cbt_leaf_ptr p;
  for (p = cbt->first; p; p = p->next) fn(p);
}

void cbt_forall(cbt_t cbt, void (*fn)(void *data, const char *key)) {
  cbt_leaf_ptr p;
  for (p = cbt->first; p; p = p->next) fn(p->data, p->key);
}

int cbt_has_u(cbt_ptr cbt, const unsigned char *key, int len) {
  if (!cbt->root) return 0;
  cbt_leaf_ptr p = cbt_leaf_at_u(cbt->root, key, len);
  return (!memcmp(p->key, key, len));
}

cbt_it cbt_it_at_u(cbt_ptr cbt, const unsigned char *key, int len) {
  if (!cbt->root) return NULL;
  cbt_leaf_ptr p = cbt_leaf_at_u(cbt->root, key, len);
  return memcmp(p->key, key, len) ? NULL : p;
}

int cbt_it_insert_u(cbt_it *it,
    cbt_ptr cbt, const unsigned char *key, int len) {
  if (!cbt->root) {
    cbt_leaf_ptr leaf = malloc(sizeof(cbt_leaf_t));
    cbt_leaf_put_u(leaf, NULL, key, len);
    cbt->root = (cbt_node_ptr) leaf;
    cbt->first = leaf;
    cbt->last = leaf;
    leaf->next = NULL;
    leaf->prev = NULL;
    cbt->count++;
    *it = leaf;
    return 1;
  }

  cbt_node_ptr stack_space[put_stack_size];
  cbt_node_ptr *stack = stack_space;
  int stack_max = put_stack_size;
  int i = 0;
  cbt_node_ptr t = cbt->root;
  stack[i] = t;
  i++;
  while (CBT_NODE == t->type) {
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
	stack = malloc(stack_max * sizeof(cbt_node_ptr));
	memcpy(stack, stack_space, put_stack_size * sizeof(cbt_node_ptr));
      } else stack = realloc(stack, stack_max * sizeof(cbt_node_ptr));
    }
    stack[i] = t;
    i++;
  }

  cbt_leaf_ptr leaf = (cbt_leaf_ptr) t;
  int res = firstcritbit_u(key, (unsigned char *) leaf->key, len);
  if (!res) {
    *it = leaf;
    return 0;
  }

  cbt->count++;
  cbt_leaf_ptr pleaf = malloc(sizeof(cbt_leaf_t));
  cbt_node_ptr pnode = malloc(sizeof(cbt_node_t));
  cbt_leaf_put_u(pleaf, NULL, (unsigned char *) key, len);
  pnode->type = CBT_NODE;
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
    pnode->right = (cbt_node_ptr) pleaf;
    // The rightmost child of the left subtree must be the predecessor.
    for (t = pnode->left; t->type == CBT_NODE; t = t->right);
    cbt_leaf_ptr leaf = (cbt_leaf_ptr) t;
    pleaf->next = leaf->next;
    pleaf->prev = leaf;
    if (leaf->next) leaf->next->prev = pleaf;
    else cbt->last = pleaf;
    leaf->next = pleaf;
  } else {
    // Key is smaller, therefore it goes on the left.
    pnode->left = (cbt_node_ptr) pleaf;
    pnode->right = stack[i + 1];
    // The leftmost child of the right subtree must be the successor.
    for (t = pnode->right; t->type == CBT_NODE; t = t->left);
    cbt_leaf_ptr leaf = (cbt_leaf_ptr) t;
    pleaf->prev = leaf->prev;
    pleaf->next = leaf;
    if (leaf->prev) leaf->prev->next = pleaf;
    else cbt->first = pleaf;
    leaf->prev = pleaf;
  }
  if (i < 0) {
    cbt->root = pnode;
  } else {
    if (stack[i]->left == stack[i + 1]) {
      stack[i]->left = pnode;
    } else {
      stack[i]->right = pnode;
    }
  }
  if (stack != stack_space) free(stack);
  *it = pleaf;
  return 1;
}
