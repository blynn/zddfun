// ZDD stack-based calculator library.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>
#include "memo.h"
#include "darray.h"
#include "zdd.h"

struct node_s {
  uint16_t v;
  uint32_t lo, hi;
};
typedef struct node_s *node_ptr;
typedef struct node_s node_t[1];

static node_t pool[1<<24];
static uint32_t freenode, POOL_MAX = (1<<24) - 1;
static mpz_ptr count[1<<24];
static darray_t stack;
static uint16_t vmax;
static char vmax_is_set;

uint16_t zdd_set_vmax(int i) {
  vmax_is_set = 1;
  return vmax = i;
}

void zdd_push() { darray_append(stack, (void *) freenode); }
void zdd_pop() { darray_remove_last(stack); }

void set_node(uint32_t n, uint16_t v, uint32_t lo, uint32_t hi) {
  pool[n]->v = v;
  pool[n]->lo = lo;
  pool[n]->hi = hi;
}

uint32_t zdd_v(uint32_t n) { return pool[n]->v; }
uint32_t zdd_hi(uint32_t n) { return pool[n]->hi; }
uint32_t zdd_lo(uint32_t n) { return pool[n]->lo; }
uint32_t zdd_set_lo(uint32_t n, uint32_t lo) { return pool[n]->lo = lo; }
uint32_t zdd_set_hi(uint32_t n, uint32_t hi) { return pool[n]->hi = hi; }
uint32_t zdd_set_hilo(uint32_t n, uint32_t hilo) {
  return pool[n]->lo = pool[n]->hi = hilo;
}
uint32_t zdd_next_node() { return freenode; }
uint32_t zdd_last_node() { return freenode - 1; }

static void pool_swap(uint32_t x, uint32_t y) {
  struct node_s tmp = *pool[y];
  *pool[y] = *pool[x];
  *pool[x] = tmp;
  for(uint32_t i = 2; i < freenode; i++) {
    if (pool[i]->lo == x) pool[i]->lo = y;
    else if (pool[i]->lo == y) pool[i]->lo = x;
    if (pool[i]->hi == x) pool[i]->hi = y;
    else if (pool[i]->hi == y) pool[i]->hi = y;
  }
}

uint32_t zdd_root() { return (uint32_t) darray_last(stack); }
uint32_t zdd_set_root(uint32_t root) {
  uint32_t i = zdd_root();
  if (i != root) pool_swap(i, root);
  return i;
}

// Count elements in ZDD rooted at node n.
static void get_count(uint32_t n) {
  if (count[n]) return;
  count[n] = malloc(sizeof(mpz_t));
  mpz_init(count[n]);
  if (n <= 1) {
    mpz_set_ui(count[n], n);
    return;
  }
  uint32_t x = pool[n]->lo;
  uint32_t y = pool[n]->hi;
  if (!count[x]) get_count(x);
  if (!count[y]) get_count(y);
  mpz_add(count[n], count[x], count[y]);
  gmp_printf("%d: %Zd\n", n, count[n]);
}

void zdd_count() {
  get_count(zdd_root());
}

uint32_t zdd_abs_node(uint32_t v, uint32_t lo, uint32_t hi) {
  set_node(freenode, v, lo, hi);
  return freenode++;
}

uint32_t zdd_add_node(uint32_t v, int offlo, int offhi) {
  int n = freenode;
  uint32_t adjust(int off) {
    if (!off) return 0;
    if (-1 == off) return 1;
    return n + off;
  }
  set_node(n, v, adjust(offlo), adjust(offhi));
  return freenode++;
}

uint32_t zdd_intersection() {
  if (darray_count(stack) == 0) return 0;
  if (darray_count(stack) == 1) return (uint32_t) darray_last(stack);
  uint32_t z0 = (uint32_t) darray_at(stack, darray_count(stack) - 2);
  uint32_t z1 = (uint32_t) darray_remove_last(stack);
  struct node_template_s {
    uint16_t v;
    // NULL means this template have been instantiated.
    // Otherwise it points to the LO template.
    memo_it lo;
    union {
      // Points to HI template when template is not yet instantiated.
      memo_it hi;
      // During template instantiation we set n to the pool index
      // of the newly created node.
      uint32_t n;
    };
  };
  typedef struct node_template_s *node_template_ptr;
  typedef struct node_template_s node_template_t[1];

  node_template_t top, bot;
  bot->v = 0;
  bot->lo = NULL;
  bot->n = 0;
  top->v = 1;
  top->lo = NULL;
  top->n = 1;

  // Naive implementation with two tries. One stores templates, the other
  // unique nodes. Knuth describes how to meld using just memory allocated for
  // a pool of nodes. Briefly, handle duplicates by executing bucket sort level
  // by level from the bottom up.
  memo_t tab;
  memo_init(tab);

  memo_t node_tab;
  memo_init(node_tab);

  memo_it insert_template(uint32_t k0, uint32_t k1) {
    uint32_t key[2];
    // Taking advantage of symmetry of intersection appears to help a tiny bit.
    if (k0 < k1) {
      key[0] = k0;
      key[1] = k1;
    } else {
      key[0] = k1;
      key[1] = k0;
    }
    memo_it it;
    int just_created = memo_it_insert_u(&it, tab, (void *) key, 8);
    if (!just_created) return it;
    if (!k0 || !k1) {
      memo_it_put(it, bot);
      return it;
    }
    if (k0 == 1 && k1 == 1) {
      memo_it_put(it, top);
      return it;
    }
    node_ptr n0 = pool[k0];
    node_ptr n1 = pool[k1];
    if (n0->v == n1->v) {
      node_template_ptr t = malloc(sizeof(*t));
      t->v = n0->v;
      if (n0->lo == n0->hi && n1->lo == n0->hi) {
	t->lo = t->hi = insert_template(n0->lo, n1->lo);
      } else {
	t->lo = insert_template(n0->lo, n1->lo);
	t->hi = insert_template(n0->hi, n1->hi);
      }
      memo_it_put(it, t);
      return it;
    } else if (n0->v < n1->v) {
      memo_it it2 = insert_template(n0->lo, k1);
      memo_it_put(it, memo_it_data(it2));
      return it2;
    } else {
      memo_it it2 = insert_template(k0, n1->lo);
      memo_it_put(it, memo_it_data(it2));
      return it2;
    }
  }

  void dump(void* data, const char* key) {
    uint32_t *n = (uint32_t *) key;
    if (!data) {
      printf("NULL %d:%d\n", n[0], n[1]);
      return;
    }
    node_template_ptr t = (node_template_ptr) data;
    if (!t->lo)  {
      printf("%d:%d = (%d)\n", n[0], n[1], t->n);
      return;
    }
    uint32_t *l = (uint32_t *) memo_it_key(t->lo);
    uint32_t *h = (uint32_t *) memo_it_key(t->hi);
    printf("%d:%d = %d:%d, %d:%d\n", n[0], n[1], l[0], l[1], h[0], h[1]);
  }

  uint32_t get_node(uint16_t v, uint32_t lo, uint32_t hi) {
    // Create or return existing node representing !v ? lo : hi.
    uint32_t key[3];
    key[0] = lo;
    key[1] = hi;
    key[2] = v;
    memo_it it;
    int just_created = memo_it_insert_u(&it, node_tab, (void *) key, 12);
    if (just_created) {
      memo_it_put(it, (void *) freenode);
      node_ptr n = pool[freenode];
      n->v = v;
      n->lo = lo;
      n->hi = hi;
      if (!(freenode % 100000)) printf("freenode = %d\n", freenode);
      if (POOL_MAX == freenode) {
	fprintf(stderr, "pool is full\n");
	exit(1);
      }
      return freenode++;
    }
    return (uint32_t) memo_it_data(it);
  }

  uint32_t instantiate(memo_it it) {
    node_template_ptr t = (node_template_ptr) memo_it_data(it);
    // Return if already converted to node.
    if (!t->lo) return t->n;
    // Recurse on LO, HI edges.
    uint32_t lo = instantiate(t->lo);
    uint32_t hi = instantiate(t->hi);
    // Remove HI edges pointing to FALSE.
    if (!hi) {
      t->lo = NULL;
      t->n = lo;
      return lo;
    }
    // Convert to node.
    uint32_t r = get_node(t->v, lo, hi);
    t->lo = NULL;
    t->n = r;
    return r;
  }

  insert_template(z0, z1);
  freenode = z0;  // Overwrite input trees.
  //memo_forall(tab, dump);
  uint32_t key[2];
  key[0] = z0;
  key[1] = z1;
  memo_it it = memo_it_at_u(tab, (void *) key, 8);
  uint32_t root = instantiate(it);
  // TODO: What if the intersection is node 0 or 1?
  if (root <= 1) {
    fprintf(stderr, "root is 0 or 1!\n");
    exit(1);
  }
  if (root < z0) {
    *pool[z0] = *pool[root];
  } else if (root > z0)  {
    pool_swap(z0, root);
  }
  void clear_it(void* data, const char* key) {
    node_template_ptr t = (node_template_ptr) data;
    uint32_t *k = (uint32_t *) key;
    if (k[0] == k[1] && t != top && t != bot) free(t);
  }
  memo_forall(tab, clear_it);
  memo_clear(tab);

  memo_clear(node_tab);
  return z0;
}

void zdd_check() {
  memo_t node_tab;
  memo_init(node_tab);
  for (uint32_t i = 2; i < freenode; i++) {
    memo_it it;
    uint32_t key[3];
    key[0] = pool[i]->lo;
    key[1] = pool[i]->hi;
    key[2] = pool[i]->v;
    if (!memo_it_insert_u(&it, node_tab, (void *) key, 12)) {
      printf("duplicate: %d %d\n", i, (int) it->data);
    } else {
      it->data = (void *) i;
    }
    if (!pool[i]->hi) {
      printf("HI -> FALSE: %d\n", i);
    }
    if (i == pool[i]->lo) {
      printf("LO self-loop: %d\n", i);
    }
    if (i == pool[i]->hi) {
      printf("HI self-loop: %d\n", i);
    }
  }
  memo_clear(node_tab);
}

void zdd_init() {
  // Initialize TRUE and FALSE nodes.
  pool[0]->v = ~0;
  pool[0]->lo = 0;
  pool[0]->hi = 0;
  pool[1]->v = ~0;
  pool[1]->lo = 1;
  pool[1]->hi = 1;
  freenode = 2;
  darray_init(stack);
}

void zdd_dump() {
  for(uint32_t i = (uint32_t) darray_last(stack); i < freenode; i++) {
    printf("I%d: !%d ? %d : %d\n", i, pool[i]->v, pool[i]->lo, pool[i]->hi);
  }
}

void vmax_check() {
  if (!vmax_is_set) {
    fprintf(stderr, "vmax not set\n");
    exit(1);
  }
}

uint32_t zdd_powerset() {
  vmax_check();
  uint16_t r = zdd_next_node();
  zdd_push();
  for(int v = 1; v < vmax; v++) zdd_add_node(v, 1, 1);
  zdd_add_node(vmax, -1, -1);
  return r;
}
