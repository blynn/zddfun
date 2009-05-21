#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>

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

void intersect(uint16_t k0, uint16_t k1) {
  node_ptr n0 = pool[k0];
  node_ptr n1 = pool[k1];
  if (n0->v == n1->v) {
    printf("template %d:%d\n", n0->v, n1->v);
    printf("lo %d:%d\n", n0->lo, n1->lo);
    printf("hi %d:%d\n", n0->hi, n1->hi);
  }
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
  one_digit_per_box(0, 1);

  intersect(k0, k1);

  get_count(k1);
  return 0;
}
