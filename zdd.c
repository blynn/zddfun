#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>
#include "cbt.h"

struct node_s {
  uint16_t v;
  uint16_t lo, hi;
};
typedef struct node_s *node_ptr;
typedef struct node_s node_t[1];

node_t pool[1<<16];
uint16_t freenode;
mpz_ptr count[1<<16];

void set_node(uint16_t n, uint16_t v, uint16_t lo, uint16_t hi) {
  pool[n]->v = v;
  pool[n]->lo = lo;
  pool[n]->hi = hi;
}

void pool_swap(uint16_t x, uint16_t y) {
  struct node_s tmp = *pool[y];
  *pool[y] = *pool[x];
  *pool[x] = tmp;
  for(uint16_t i = 2; i < freenode; i++) {
    if (pool[i]->lo == x) pool[i]->lo = y;
    else if (pool[i]->lo == y) pool[i]->lo = x;
    if (pool[i]->hi == x) pool[i]->hi = y;
    else if (pool[i]->hi == y) pool[i]->hi = y;
  }
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
//
// This ZDD has 9*2^720 members.
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

// Construct ZDD of sets containing exactly 1 digit at for all boxes
// (r, c), starting at pool entry d.
//
// The ZDD begins:
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
// and repeats every 10 levels:
//   10 ... 11a
//   10 --- 11
// and so on until 729a --- T, 729b ... T.
//
// This ZDD has 9^81 members.
void global_one_digit_per_box() {
  // The order will be 1, 2a to 9a, 2b to 9b, 10, ...
  int d = freenode;
  int k;
  for (k = 0; k < 81; k++) {
    int i;
    for (i = 1; i < 9; i++) {
      // Nodes 9k + 1, 9k + 2a to 9k + 8a.
      set_node(d, 9 * k + i, d + 1, d + 9);
      // Nodes 9k + 2b to 9k + 9b.
      set_node(d + 9, 9 * k + i + 1, d + 9 + 1, 0);
      d++;
    }
    // Node 9k + 9a.
    set_node(d, 9 * k + i, 0, d + 9);
    // Node 9k + 9b.
    set_node(d + 9 - 1, 9 * k + i, d + 9, 0);
    d += 9;
  }
  // Fix 729a and 729b.
  pool[d - 1 - 8]->hi = 1;
  pool[d - 1]->lo = 1;
  freenode = d;
}

// Construct ZDD of sets containing exactly 1 occurrence of digit d in row r.
// For instance, if d = 3, r = 0:
//
//   1 === 2 === 3
//   3 ... 4a, --- 4b
//   4a === 5a === ... === 9a === ... === 12a
//   4b === 5b === ... === 9b === ... === 12b
//   12a ... 13a, --- 13b
//   12b ... 13b, --- F
//   13a === ... === 21a
//   13b === ... === 21b
// and so on until:
//   75a ... F, --- 76
//   75b ... 76, --- F
//   76 === ... === 729 === T
//
// This ZDD has 9*2^720 members.
// When intersected with the one-digit-per-box set, the result has
// 9*8^8*9^72 members. (Pick 1 of 9 positions for d, leaving 8 possible choices
// for the remaining 8 boxes in that row. The other 81 - 9 boxes can contain
// any single digitS.)
// The intersection for all d and a fixed r has 9!*9^72 members.
// The intersection for all r and a fixed d has 9^9*8^72 members.
void unique_digit_per_row(int d, int r) {
  // The order is determined by sorting by number, then by letter.
  int n = freenode;
  int next = 81 * r + d;  // The next node involving the digit d.
  int v = 1;
  int state = 0;
  while (v <= 729) {
    if (v == next) {
      next += 9;
      state++;
      if (state == 1) {
	// The first split in the ZDD.
	set_node(n, v, n + 1, n + 2);
	n++;
      } else if (state < 9) {
	// If this is the first occurrence of d, we're on notice.
	set_node(n, v, n + 2, n + 3);
	n++;
	// If we see a second occurrence of d, then branch to FALSE.
	set_node(n, v, n + 2, 0);
	n++;
      } else {
	// If we never saw d, then branch to FALSE.
	set_node(n, v, 0, n + 2);
	n++;
	// If we see a second occurrence of d, then branch to FALSE.
	// Otherwise reunite the branches.
	set_node(n, v, n + 1, 0);
	n++;
	next = -1;
      }
    } else if (state == 0 || state == 9) {
      set_node(n, v, n + 1, n + 1);
      n++;
    } else {
      set_node(n, v, n + 2, n + 2);
      n++;
      set_node(n, v, n + 2, n + 2);
      n++;
    }
    v++;
  }
  // Fix 729, or 729a and 729b.
  if (pool[n - 2]->hi == n) pool[n - 2]->hi = 1;
  if (pool[n - 1]->lo == n) pool[n - 1]->lo = 1;
  if (pool[n - 1]->hi == n) pool[n - 1]->hi = 1;
  freenode = n;
}

void intersect(uint16_t z0, uint16_t z1) {
  struct node_template_s {
    uint16_t v;
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

  node_template_t top, bot;
  bot->v = 0;
  bot->lo = NULL;
  bot->n = 0;
  top->v = 1;
  top->lo = NULL;
  top->n = 1;

  // Naive implementation with two tries. One stores templates, the other
  // unique nodes. Knuth describes how to do meld using just memory
  // allocated for a pool of nodes.
  cbt_t tab;
  cbt_init(tab);

  cbt_t node_tab;
  cbt_init(node_tab);

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
    } else {
      printf("TODO: v != w\n");
      exit(1);
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

  uint16_t get_node(uint16_t v, uint16_t lo, uint16_t hi) {
    // Create or return existing node representing !v ? lo : hi.
    uint16_t key[3];
    key[0] = v;
    key[1] = lo;
    key[2] = hi;
    cbt_it it;
    int just_created = cbt_it_insert_u(&it, node_tab, (void *) key, 6);
    if (just_created) {
      cbt_it_put(it, (void *) freenode);
      node_ptr n = pool[freenode];
      n->v = v;
      n->lo = lo;
      n->hi = hi;
      return freenode++;
    }
    return (uint16_t) cbt_it_data(it);
  }

  uint16_t instantiate(cbt_it it) {
    node_template_ptr t = (node_template_ptr) cbt_it_data(it);
    // Return if already converted to node.
    if (!t->lo) return t->n;
    // Recurse on LO, HI edges.
    uint16_t lo = instantiate(t->lo);
    uint16_t hi = instantiate(t->hi);
    // Remove HI edges pointing to FALSE.
    if (!hi) {
      t->lo = NULL;
      t->n = lo;
      return lo;
    }
    // Convert to node.
    uint16_t r = get_node(t->v, lo, hi);
    t->lo = NULL;
    t->n = r;
    return r;
  }

  insert_template(z0, z1);
  freenode = z0;  // Overwrite input trees.
  //cbt_forall(tab, dump);
  uint16_t key[2];
  key[0] = z0;
  key[1] = z1;
  cbt_it it = cbt_it_at_u(tab, (void *) key, 4);
  uint16_t root = instantiate(it);
  if (root < z0) {
    *pool[z0] = *pool[root];
  } else if (root > z0)  {
    pool_swap(z0, root);
  }
  void clear_it(void* data, const char* key) {
    node_template_ptr t = (node_template_ptr) data;
    if (t != top && t != bot) free(t);
  }
  cbt_forall(tab, clear_it);
  cbt_clear(tab);

  cbt_clear(node_tab);
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
  /*
  one_digit_per_box(0, 0);
  uint16_t k1 = freenode;
  one_digit_per_box(0, 1);
  intersect(k0, k1);
  k1 = freenode;
  one_digit_per_box(0, 2);
  intersect(k0, k1);
  k1 = freenode;
  one_digit_per_box(0, 3);
  intersect(k0, k1);
  */

  int i;
  global_one_digit_per_box();
  for (int r = 0; r < 2; r++) {
    for (i = 1; i <= 1; i++) {
      uint16_t k1 = freenode;
      unique_digit_per_row(i, r);
      intersect(k0, k1);
    }
  }

  for(i = k0; i < freenode; i++) {
    printf("I%d: !%d ? %d : %d\n", i, pool[i]->v, pool[i]->lo, pool[i]->hi);
  }

  get_count(k0);
  return 0;
}
