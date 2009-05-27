// Nonogram solver.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>
#include "cbt.h"

struct node_s {
  uint16_t v;
  uint32_t lo, hi;
};
typedef struct node_s *node_ptr;
typedef struct node_s node_t[1];

node_t pool[1<<24];
uint32_t freenode, POOL_MAX = (1<<24) - 1;
mpz_ptr count[1<<24];

int max;

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

uint32_t add_node(uint32_t v, int offlo, int offhi) {
  int n = freenode;
  uint32_t adjust(int off) {
    if (!off) return 0;
    if (-1 == off) return 1;
    return n + off;
  }
  set_node(n, v, adjust(offlo), adjust(offhi));
  return freenode++;
}

uint32_t add_row_clue(int row, int *a, int size, int root) {
  uint32_t v = row * max + 1;
  uint32_t table[max][max + 1];
  uint32_t partial_row(uint32_t i, int *a, int count, int sum) {
    if (table[i][count] != 0) return table[i][count];
    uint32_t first, last;
    first = last = table[i][count] = add_node(v + i, 0, 1);
    for(int j = 1; j < *a; j++) {
      last = add_node(v + i + j, 0, 1);
    }
    if (1 == count) {
      pool[last]->hi = root;
    } else {
      pool[last]->hi = partial_row(i + *a + 1, a + 1, count - 1, sum - *a);
    }
    if (max - count - sum + 1 - i > 0) {
      pool[first]->lo = partial_row(i + 1, a, count, sum);
    }
    return table[i][count];
  }

  memset(table, 0, sizeof(uint32_t) * max * (max + 1));
  int sum = 0;
  for (int i = 0; i < size; i++) {
    sum += a[i];
  }
  return partial_row(0, a, size, sum);
}

uint32_t compute_col_clue(int col, int *a, int size) {
  uint32_t table[max + 1][max + 1];
  uint32_t tail(uint32_t v, uint32_t i) {
    if (table[i][0] != 0) return table[i][0];
    table[i][0] = freenode;
    if (max == i) {
      while(v < max * max) add_node(v++, 1, 1);
      add_node(v, -1, -1);
    } else {
      while(v < max * i + col + 1) add_node(v++, 1, 1);
      v++;
      uint32_t last = freenode - 1;
      pool[last]->lo = pool[last]->hi = tail(v, i + 1);
    }
    return table[i][0];
  }
  uint32_t partial_col(uint32_t v, uint32_t i, int *a, int count, int sum) {
    if (table[i][count] != 0) return table[i][count];
    uint32_t first, last;
    table[i][count] = freenode;
    while(v < col + 1 + i * max) add_node(v++, 1, 1);
    first = last = add_node(v++, 0, 1);
    uint32_t oldv = v;
    for(int j = 1; j < *a; j++) {
      while(v < col + 1 + (i + j) * max) add_node(v++, 1, 1);
      last = add_node(v++, 0, 1);
    }
    if (1 == count) {
      pool[last]->hi = tail(v, i + *a);
    } else {
      while(v < col + 1 + (i + *a) * max) add_node(v++, 1, 1);
      v++;
      last = freenode - 1;
      pool[last]->lo = pool[last]->hi =
	  partial_col(v, i + *a + 1, a + 1, count - 1, sum - *a);
    }
    if (max - count - sum + 1 - i > 0) {
      pool[first]->lo = partial_col(oldv, i + 1, a, count, sum);
    }
    return table[i][count];
  }

  int sum = 0;
  for (int i = 0; i <= max; i++) for (int j = 0; j <= max; j++) table[i][j] = 0;
  for (int i = 0; i < size; i++) sum += a[i];
  if (!size) return tail(1, 0);
  return partial_col(1, 0, a, size, sum);
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
  // by level from the bottom up.
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
  // TODO: What if the intersection is node 0 or 1?
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

  /*
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      void print_it(void *data) {
	int i = (int) data;
	printf(" %d", i);
      }
      darray_forall(board[i][j], print_it);
      printf("\t\t");
    }
    printf("\n");
  }
  */
  if (!scanf("%d\n", &max)) {
    fprintf(stderr, "input error\n");
    exit(1);
  }
  // Read max row clues, then max column clues.
  int clue[max * 2][max + 1];
  for(int i = 0; i < max * 2; i++) {
    char s[80];
    if (!fgets(s, 80, stdin)) {
      fprintf(stderr, "input error\n");
      exit(1);
    }
    char *w = s;
    for(int j = 0; j < max; j++) {
      if (!sscanf(w, "%d", &clue[i][j + 1]) || !clue[i][j + 1]) {
	clue[i][0] = j;
	break;
      }
      w = strchr(w + 1, ' ');
      if (!w) {
	clue[i][0] = j + 1;
	break;
      }
    }
  }

  // Construct ZDD for all row clues.
  uint32_t k0 = freenode;
  uint32_t root = 1;
  for(int i = max - 1; i >= 0; i--) {
    if (clue[i][0]) {
      root = add_row_clue(i, &clue[i][1], clue[i][0], root);
    }
  }
  pool_swap(k0, root);

  //printf("all rows: %d\n", freenode - k0);
  // Intersect each column clue into the ZDD
  for(int i = 0; i < max; i++) {
    uint32_t k1 = freenode;
    compute_col_clue(i, &clue[i + max][1], clue[i + max][0]);
    //printf("column %d: %d\n", i, freenode - k1);
    intersect(k0, k1);
    //printf("intersected: %d\n", freenode - k0);
  }

  /*
  for(int i = k0; i < freenode; i++) {
    printf("I%d: !%d ? %d : %d\n", i, pool[i]->v, pool[i]->lo, pool[i]->hi);
  }

  get_count(k0);
  */

  check_reduced();

  // Assumes there is only one solution.
  int board[max][max];
  memset(board, 0, sizeof(int) * max * max);
  uint32_t v = k0;
  while(v != 1) {
    int r = pool[v]->v - 1;
    int c = r % max;
    r /= max;
    board[r][c] = 1;
    v = pool[v]->hi;
  }
  for (int i = 0; i < max; i++) {
    for (int j = 0; j < max; j++) {
      putchar(board[i][j] ? 'X' : '.');
    }
    putchar('\n');
  }

  return 0;
}
