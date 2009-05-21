#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>
#include "cbt.h"

struct node_s {
  uint8_t v;
  uint16_t lo, hi;
};
typedef struct node_s *node_ptr;
typedef struct node_s node_t[1];

node_t pool[1<<16];
uint16_t freenode;
mpz_ptr count[1<<16];

void set_node(uint16_t n, uint8_t v, uint16_t lo, uint16_t hi) {
  pool[n]->v = v;
  pool[n]->lo = lo;
  pool[n]->hi = hi;
}

// Count elements in ZDD rooted at node n.
void get_count(uint16_t n) {
  if (count[n]) return;
  count[n] = malloc(sizeof(mpz_t));
  mpz_init(count[n]);
  if (n <= 1) {
    mpz_set_ui(count[n], n);
    return;
  }
  uint16_t x = pool[n]->lo;
  uint16_t y = pool[n]->hi;
  if (!count[x]) get_count(x);
  if (!count[y]) get_count(y);
  mpz_add(count[n], count[x], count[y]);
  gmp_printf("%d: %Zd\n", n, count[n]);
}

// Construct ZDD of sets containing exactly 1 digit at (r, c), starting
// at pool entry d.
//
// For (0, 0) the graph is:
//   1 ... 2a
//   1 --- 2b
//   2a ... 3a
//   2a --- 3b
//   ...
//   9a ... F
//   9a --- 10
//
//   2b ... 3b
//   2b --- F
//   3b ... 4b
//   3b --- F
//   ...
//   9b ... 10
//   9b --- F
//
//   10 ... 11, 10 --- 11 until 729 ... T, 729 --- T
// In general we have 1 ... 2, 1 --- 2 until we reach 81 r + 9 c where
// we switch to a ZDD like the above.
void one_digit_per_box(int r, int c) {
  // For (0,0) the order in pool will be 1, 2a to 9a, 2b to 9b, 10 ... 729.
  // More generally we have the 1 ... 81 r + 9 c nodes first.
  int d = freenode - 1;
  int i;
  int n = 81 * r + 9 * c;
  for (i = 1; i <= n; i++) {
    // Nodes 1 to 81 r + 9 c.
    set_node(d + i, i, d + i + 1, d + i + 1);
  }
  d += n;

  for (i = 1; i < 9; i++) {
    // In the following comments, (+n) means we add n to the node numbers.
    // (+n) Nodes 1 and 2a to 8a.
    set_node(d + i, n + i, d + 1 + i, d + 9 + i);
    // (+n) Nodes 2b to 9b.
    set_node(d + 9 + i, n + i + 1, d + 9 + i + 1, 0);
  }
  // (+n) Node 9a.
  set_node(d + i, n + i, 0, d + 9 + i);
  // Nodes (+n) 10 to 728.
  d += 9 + 9 - (n + i + 1);
  for (i = n + i + 1; i < 729; i++) {
    set_node(d + i, i, d + i + 1, d + i + 1);
  }
  // Node 729.
  set_node(d + i, i, 1, 1);
  freenode += 729 + 8;
}

void intersect(uint16_t z0, uint16_t z1) {
  struct node_template_s {
    uint8_t v;
    // NULL means this template have been instantiated.
    // Otherwise it points to the LO template.
    cbt_it lo;
    union {
      // Points to HI template when template is not yet instantiated.
      cbt_it hi;
      // During template instantiation we set n to the pool index
      // of the newly created node.
      uint16_t n;
    };
  };
  typedef struct node_template_s *node_template_ptr;
  typedef struct node_template_s node_template_t[1];

  node_template_t bot, top;
  bot->v = 0;
  bot->lo = NULL;
  bot->n = 0;
  top->v = 1;
  top->lo = NULL;
  top->n = 1;

  cbt_t tab;
  cbt_init(tab);

  void insert_template(uint16_t k0, uint16_t k1) {
    uint16_t key[2];
    if (!k0 || !k1) {
      key[0] = k0;
      key[1] = k1;
      cbt_put_u(tab, bot, (void *) key, 4);
      return;
    }
    if (k0 == 1 && k1 == 1) {
      key[0] = k0;
      key[1] = k1;
      cbt_put_u(tab, top, (void *) key, 4);
      return;
    }
    node_ptr n0 = pool[k0];
    node_ptr n1 = pool[k1];
    if (n0->v == n1->v) {
      node_template_ptr t = malloc(sizeof(*t));
      t->v = n0->v;
      // Find or create trie entry for LO ZDD meld.
      key[0] = n0->lo;
      key[1] = n1->lo;
      int recurselo = cbt_it_insert_u(&t->lo, tab, (void *) key, 4);
      // Find or create trie entry for HI ZDD meld.
      key[0] = n0->hi;
      key[1] = n1->hi;
      int recursehi = cbt_it_insert_u(&t->hi, tab, (void *) key, 4);
      // Insert template.
      key[0] = k0;
      key[1] = k1;
      cbt_put_u(tab, t, (void *) key, 4);
      if (recurselo) insert_template(n0->lo, n1->lo);
      if (recursehi) insert_template(n0->hi, n1->hi);
    }
  }

  void dump(void* data, const char* key) {
    uint16_t *n = (uint16_t *) key;
    if (!data) {
      printf("NULL %d:%d\n", n[0], n[1]);
      return;
    }
    node_template_ptr t = (node_template_ptr) data;
    if (!t->lo)  {
      printf("%d:%d = (%d)\n", n[0], n[1], t->n);
      return;
    }
    uint16_t *l = (uint16_t *) cbt_it_key(t->lo);
    uint16_t *h = (uint16_t *) cbt_it_key(t->hi);
    printf("%d:%d = %d:%d, %d:%d\n", n[0], n[1], l[0], l[1], h[0], h[1]);
  }

  int instantiate(cbt_it it) {
    node_template_ptr t = (node_template_ptr) cbt_it_data(it);
    // Return if already converted to node.
    if (!t->lo) return t->n;
    // Convert to node. Take next free node, and recurse.
    uint16_t r = freenode++;
    node_ptr n = pool[r];
    n->v = t->v;
    n->lo = instantiate(t->lo);
    n->hi = instantiate(t->hi);
    t->lo = NULL;
    t->n = r;
    return r;
  }

  insert_template(z0, z1);
  freenode = z0;  // Overwrite input trees.
  cbt_forall(tab, dump);
  uint16_t key[2];
  key[0] = z0;
  key[1] = z1;
  printf("%d %d\n", key[0], key[1]);
  cbt_it it = cbt_it_at_u(tab, (void *) key, 4);
  printf("Inst\n");
  instantiate(it);
  int i;
  for(i = 0; i < freenode; i++) {
    printf("I%d: %d ? %d : %d\n", i, pool[i]->v, pool[i]->lo, pool[i]->hi);
  }
  cbt_clear(tab);
}

int main() {
  // Initialize TRUE and FALSE nodes.
  pool[0]->v = ~0;
  pool[0]->lo = 0;
  pool[0]->hi = 0;
  pool[1]->v = ~0;
  pool[1]->lo = 1;
  pool[1]->hi = 1;
  freenode = 2;

  // The universe is {1, ..., 9^3 = 729}.
  // Number rows and columns from 0. Digits are integers [1..9].
  // The digit d at (r, c) is represented by element 81 r + 9 c + d.
  uint16_t k0 = freenode;
  one_digit_per_box(0, 0);
  uint16_t k1 = freenode;
  one_digit_per_box(0, 2);

  intersect(k0, k1);

  //get_count(k1);
  return 0;
}
