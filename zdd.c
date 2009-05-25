#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>
#include "cbt.h"
#include "darray.h"

struct node_s {
  uint16_t v;
  uint32_t lo, hi;
};
typedef struct node_s *node_ptr;
typedef struct node_s node_t[1];

node_t pool[1<<24];
uint32_t freenode, POOL_MAX = (1<<24) - 1;
mpz_ptr count[1<<24];

void set_node(uint32_t n, uint16_t v, uint32_t lo, uint32_t hi) {
  pool[n]->v = v;
  pool[n]->lo = lo;
  pool[n]->hi = hi;
}

void pool_swap(uint32_t x, uint32_t y) {
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

// Count elements in ZDD rooted at node n.
void get_count(uint32_t n) {
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

// Construct ZDD of sets containing exactly 1 digit at for all boxes
// (r, c), starting at pool entry d.
//
// The ZDD begins:
//   1 ... 2
//   1 --- 10
//   2 ... 3
//   2 --- 10
//   ...
//   9 ... F
//   9 --- 10
//
// and repeats every 10 levels:
//   10 ... 11
//   10 --- 19
// and so on until 729 --- F, 729 ... T.
//
// This ZDD has 9^81 members.
void global_one_digit_per_box() {
  int d = freenode;
  int next = 9;
  for (int i = 1; i <= 729; i++) {
    set_node(d, i, (i % 9) ? d + 1 : 0, next < 729 ? freenode + next : 1);
    if (!(i % 9)) next += 9;
    d++;
  }
  freenode = d;
}

// Construct ZDD of sets containing exactly 1 occurrence of digit d in row r.
// For instance, if d = 3, r = 0:
//
//   1 === 2 === 3
//   3 ... 4a, --- 4b
//   4a === 5a === ... === 9a === ... === 11a === 12
//   4b === 5b === ... === 9b === ... === 11b === 13b
//   12 ... 13a, --- 13b
//   13a === ... === 20a === 21
//   13b === ... === 20b === 22b
// and so on until:
//   74a === 75 ... F, --- 76
//   74b === 76
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
	// Fix previous node. We must not have a second occurrence of d.
	set_node(n - 1, v - 1, n + 2, n + 2);
	// If this is the first occurrence of d, we're on notice.
	set_node(n, v, n + 1, n + 2);
	n++;
      } else {
	// If we never saw d, then branch to FALSE.
	// Otherwise reunite the branches.
	set_node(n, v, 0, n + 1);
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
  // Fix last nodes.
  if (pool[n - 2]->lo >= n) pool[n - 2]->lo = 1;
  if (pool[n - 2]->hi >= n) pool[n - 2]->hi = 1;
  if (pool[n - 1]->lo >= n) pool[n - 1]->lo = 1;
  if (pool[n - 1]->hi >= n) pool[n - 1]->hi = 1;
  freenode = n;
}

// Construct ZDD of sets containing exactly 1 of the elements in the given list
// The list is terminated by -1. Only valid when there are no consecutive
// elements. In particular, will not work for a 9 followed by a 1!
// Generalization of previous function.
void contains_exactly_one(int *list) {
  // The order is determined by sorting by number, then by letter.
  int n = freenode;
  int v = 1;
  int *next = list;
  while (v <= 729) {
    if (v == *next) {
      next++;
      if (list + 1 == next) {
	// The first split in the ZDD.
	set_node(n, v, n + 1, n + 2);
	n++;
      } else if (*next >= 1) {
	// One of the boxes in the middle the list.
	// Fix previous node.
	set_node(n - 1, v - 1, n + 2, n + 2);
	// If this is the first occurrence, we're on notice.
	set_node(n, v, n + 1, n + 2);
	n++;
      } else {
	// Last box in the list: -1 == *next.
	// If we never saw anything from the list, then branch to FALSE.
	// Otherwise reunite the branches.
	set_node(n, v, 0, n + 1);
	n++;
      }
    } else if (list == next || *next == -1) {
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
  // Fix last nodes.
  if (pool[n - 2]->lo >= n) pool[n - 2]->lo = 1;
  if (pool[n - 2]->hi >= n) pool[n - 2]->hi = 1;
  if (pool[n - 1]->lo >= n) pool[n - 1]->lo = 1;
  if (pool[n - 1]->hi >= n) pool[n - 1]->hi = 1;
  freenode = n;
}

// Construct ZDD of sets containing all elements in the given list.
// The list is terminated by -1.
void contains_all(int *list) {
  int n = freenode;
  int v = 1;
  int *next = list;
  while (v <= 729) {
    if (v == *next) {
      next++;
      set_node(n, v, 0, n + 1);
      n++;
    } else {
      set_node(n, v, n + 1, n + 1);
      n++;
    }
    v++;
  }
  // Fix 729.
  if (pool[n - 1]->lo == n) pool[n - 1]->lo = 1;
  if (pool[n - 1]->hi == n) pool[n - 1]->hi = 1;
  freenode = n;
}

void unique_digit_per_col(int d, int col) {
  int list[10];
  for (int i = 0; i < 9; i++) {
    list[i] = 81 * i + 9 * col + d;
  }
  list[9] = -1;
  contains_exactly_one(list);
}

void unique_digit_per_3x3(int d, int row, int col) {
  int list[10];
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      list[i * 3 + j] = 81 * (i + 3 * row) + 9 * (j + 3 * col) + d;
    }
  }
  list[9] = -1;
  contains_exactly_one(list);
}

void intersect(uint32_t z0, uint32_t z1) {
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
  // by level, from the bottom up.
  cbt_t tab;
  cbt_init(tab);

  cbt_t node_tab;
  cbt_init(node_tab);

  cbt_it insert_template(uint32_t k0, uint32_t k1) {
    uint32_t key[2];
    // Taking advantage of symmetry of intersection appears to help a tiny bit.
    if (k0 < k1) {
      key[0] = k0;
      key[1] = k1;
    } else {
      key[0] = k1;
      key[1] = k0;
    }
    cbt_it it;
    int just_created = cbt_it_insert_u(&it, tab, (void *) key, 8);
    if (!just_created) return it;
    if (!k0 || !k1) {
      cbt_it_put(it, bot);
      return it;
    }
    if (k0 == 1 && k1 == 1) {
      cbt_it_put(it, top);
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
      cbt_it_put(it, t);
      return it;
    } else if (n0->v < n1->v) {
      cbt_it it2 = insert_template(n0->lo, k1);
      cbt_it_put(it, cbt_it_data(it2));
      return it2;
    } else {
      cbt_it it2 = insert_template(k0, n1->lo);
      cbt_it_put(it, cbt_it_data(it2));
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
    uint32_t *l = (uint32_t *) cbt_it_key(t->lo);
    uint32_t *h = (uint32_t *) cbt_it_key(t->hi);
    printf("%d:%d = %d:%d, %d:%d\n", n[0], n[1], l[0], l[1], h[0], h[1]);
  }

  uint32_t get_node(uint16_t v, uint32_t lo, uint32_t hi) {
    // Create or return existing node representing !v ? lo : hi.
    uint32_t key[3];
    key[0] = lo;
    key[1] = hi;
    key[2] = v;
    cbt_it it;
    int just_created = cbt_it_insert_u(&it, node_tab, (void *) key, 12);
    if (just_created) {
      cbt_it_put(it, (void *) freenode);
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
    return (uint32_t) cbt_it_data(it);
  }

  uint32_t instantiate(cbt_it it) {
    node_template_ptr t = (node_template_ptr) cbt_it_data(it);
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
  //cbt_forall(tab, dump);
  uint32_t key[2];
  key[0] = z0;
  key[1] = z1;
  cbt_it it = cbt_it_at_u(tab, (void *) key, 8);
  uint32_t root = instantiate(it);
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
  cbt_forall(tab, clear_it);
  cbt_clear(tab);

  cbt_clear(node_tab);
}

void check_reduced() {
  cbt_t node_tab;
  cbt_init(node_tab);
  for (uint32_t i = 2; i < freenode; i++) {
    cbt_it it;
    uint32_t key[3];
    key[0] = pool[i]->lo;
    key[1] = pool[i]->hi;
    key[2] = pool[i]->v;
    if (!cbt_it_insert_u(&it, node_tab, (void *) key, 12)) {
      printf("duplicate: %d %d\n", i, (int) it->data);
    } else {
      it->data = (void *) i;
    }
  }
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
  uint32_t k0 = freenode;

  darray_t list;
  darray_init(list);
  /*
  for(int i = 0; i < 9; i++) {
    for(int j = 0; j < 9; j++) {
      int c = getchar();
      if (EOF == c) {
	fprintf(stderr, "unexpected EOF\n");
	exit(1);
      }
      if ('\n' == c) {
	fprintf(stderr, "unexpected newline\n");
	exit(1);
      }
      if (c >= '1' && c <= '9') {
	darray_append(list, (void *) (81 * i + 9 * j + c - '0'));
      }
    }
    int c = getchar();
    if (EOF == c) {
      fprintf(stderr, "unexpected EOF\n");
      exit(1);
    }
    if ('\n' != c) {
      fprintf(stderr, "expected newline\n");
      exit(1);
    }
  }
  */
  darray_append(list, (void *) -1);
  contains_all((int *) list->item);
  darray_clear(list);

  uint32_t k1 = freenode;
  global_one_digit_per_box();
  intersect(k0, k1);

  // Number of ways you can put nine 1s into a sudoku is
  //   9*6*3*6*3*4*2*2.
  printf("rows\n");
  fflush(stdout);
  for (int i = 1; i <= 9; i++) {
    uint32_t k1 = freenode;
    for (int r = 0; r < 9; r++) {
      uint32_t k2 = freenode;
      unique_digit_per_row(i, r);
      if (r) intersect(k1, k2);
    }
    intersect(k0, k1);
  }
  for (int i = 1; i <= 9; i++) {
    uint32_t k1 = freenode;
    for (int c = 0; c < 3; c++) {
      uint32_t k2 = freenode;
      for (int r = 0; r < 3; r++) {
	printf("3x3 %d: %d, %d\n", i, r, c);
	fflush(stdout);
	uint32_t k3 = freenode;
	unique_digit_per_3x3(i, r, c);
	if (r) intersect(k2, k3);
      }
      if (c) intersect(k1, k2);
    }
    intersect(k0, k1);
  }
  for (int i = 1; i <= 1; i++) {
    for (int c = 0; c < 9; c++) {
      printf("cols %d: %d\n", i, c);
      fflush(stdout);
      uint32_t k1 = freenode;
      unique_digit_per_col(i, c);
      intersect(k0, k1);
    }
  }

  for(int i = k0; i < freenode; i++) {
    printf("I%d: !%d ? %d : %d\n", i, pool[i]->v, pool[i]->lo, pool[i]->hi);
  }

  get_count(k0);
  return 0;
}
