// Solve a Light Up puzzle.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>
#include "darray.h"
#include "zdd.h"

#include <stdarg.h>

extern void die(const char *err, ...)
    __attribute__((__noreturn__))  __attribute__((format (printf, 1, 2)));

void die(const char *err, ...) {
  va_list params;

  va_start(params, err);
  vfprintf(stderr, err, params);
  fputc('\n', stderr);
  exit(1);
  va_end(params);
}

uint32_t rcount, ccount;
uint32_t vmax;

// Construct ZDD of sets containing exactly n of the elements in the
// given list.
void contains_exactly_n(darray_t a, int n) {
  uint32_t tab[darray_count(a)][n + 1];
  memset(tab, 0, darray_count(a) * (n + 1) * sizeof(uint32_t));
  uint32_t recurse(int i, int n) {
    int v = -1 == i ? 1 : (int) darray_at(a, i) + 1;
    uint32_t root;
    if (i == darray_count(a) - 1) {
      // n is irrelevant when we reach the end of the list.
      if (-1 != i && tab[i][0]) return tab[i][0];
      if (vmax < v) {
	root = 1;
      } else {
        root = zdd_next_node();
	while(v < vmax) zdd_add_node(v++, 1, 1);
	zdd_add_node(v, -1, -1);
      }
      if (-1 != i) tab[i][0] = root;
      return root;
    }
    if (-1 != i && tab[i][n]) return tab[i][n];
    int v1 = (int) darray_at(a, i + 1);
    int is_empty = v == v1;
    root = zdd_next_node();
    while(v < v1) zdd_add_node(v++, 1, 1);
    if (!n) {
      if (is_empty) {
	root = recurse(i + 1, n);
      } else {
	uint32_t last = zdd_last_node();
	zdd_set_hilo(last, recurse(i + 1, n));
      }
      if (-1 != i) tab[i][n] = root;
      return root;
    }
    uint32_t last = zdd_add_node(v, 0, 0);
    zdd_set_hi(last, recurse(i + 1, n - 1));
    if (n < darray_count(a) - i - 1) {
      zdd_set_lo(last, recurse(i + 1, n));
    }
    if (-1 != i) tab[i][n] = root;
    return root;
  }
  recurse(-1, n);
}

// Construct ZDD of sets containing at least 1 of the elements in the
// given list.
void contains_at_least_one(darray_t a) {
  uint32_t n = zdd_last_node();
  // Start with ZDD of all sets.
  int v = 1;
  while(v < vmax) {
    zdd_add_node(v++, 1, 1);
  }
  zdd_add_node(v, -1, -1);
  if (darray_is_empty(a)) return;

  // Construct new branch for when elements of the list are not found.
  v = (int) darray_first(a);
  if (1 == darray_count(a)) {
    zdd_set_lo(n + v, 0);
    return;
  }

  uint32_t n1 = zdd_next_node();
  zdd_set_lo(n + v, n1);
  v++;
  for(int i = 1; i < darray_count(a); i++) {
    int v1 = (int) darray_at(a, i);
    while(v <= v1) {
      zdd_add_node(v++, 1, 1);
    }
    zdd_set_hi(zdd_last_node(), n + v);
  }

  zdd_set_lo(zdd_last_node(), 0);
  if (vmax < v) zdd_set_hi(zdd_last_node(), 1);
}

// Construct ZDD of sets containing at most 1 of the elements in the given
// list.
void contains_at_most_one(darray_t a) {
  uint32_t n = zdd_last_node();
  // Start with ZDD of all sets.
  int v = 1;
  while(v < vmax) {
    zdd_add_node(v++, 1, 1);
  }
  zdd_add_node(v, -1, -1);
  // If there is nothing or only one element in the list then we are done.
  if (darray_count(a) <= 1) return;

  // At this point, there are at least two elements in the list.
  // Construct new branch for when elements of the list are detected. We
  // branch off at the first element, then hop over all remaining elements,
  // then rejoin.
  v = (int) darray_first(a);

  uint32_t n1 = zdd_next_node();
  zdd_set_hi(n + v, n1);
  v++;
  uint32_t last = 0;
  for(int i = 1; i < darray_count(a); i++) {
    int v1 = (int) darray_at(a, i);
    while(v < v1) {
      last = zdd_add_node(v++, 1, 1);
    }
    zdd_set_hi(n + v, zdd_next_node());
    v++;
  }
  // v = last element of list + 1

  // The HI edges of the last element of the list, and more generally, the last
  // sequence of the list must be corrected.
  for(int v1 = (int) darray_last(a); zdd_hi(n + v1) == zdd_next_node(); v1--) {
    zdd_set_hi(n + v1, n + v);
  }

  if (vmax < v) {
    // Special case: list ends with vmax. Especially troublesome if there's
    // a little sequence, e.g. vmax - 2, vmax - 1, vmax.
    for(v = vmax; zdd_hi(n + v) > n + vmax; v--) {
      zdd_set_hi(n + v, 1);
    }
    // The following line is only needed if we added any nodes to the branch,
    // but is harmless if we execute it unconditionally since the last node
    // to be added was (!vmax ? 1 : 1).
    zdd_set_hilo(zdd_last_node(), 1);
    return;
  }

  // Rejoin main branch.
  if (last) zdd_set_hilo(last, n + v);
}

// Construct ZDD of sets not containing any elements from the given list.
// Assumes not every variable is on the list.
void contains_none(darray_t a) {
  int i = 1;
  int v1 = darray_is_empty(a) ? -1 : (int) darray_first(a);
  for(int v = 1; v <= vmax; v++) {
    if (v1 == v) {
      if (i < darray_count(a)) {
	v1 = (int) darray_at(a, i++);
      } else {
	v1 = -1;
      }
    } else {
      zdd_add_node(v, 1, 1);
    }
  }
  uint32_t n = zdd_last_node();
  zdd_set_lo(n, 1);
  zdd_set_hi(n, 1);
}

int main() {
  zdd_init();

  if (!scanf("%d %d\n", &rcount, &ccount)) die("input error");
  int board[rcount][ccount];
  uint32_t getv(uint32_t i, uint32_t j) { return ccount * i + j + 1; }
  vmax = getv(rcount - 1, ccount - 1);
  darray_t a;
  darray_init(a);

  for (int i = 0; i < rcount; i++) {
    int c;
    for (int j = 0; j < ccount; j++) {
      c = getchar();
      if (c == EOF || c == '\n') die("input error");
      int encode(char c) {
	switch(c) {
	  case '.':
	    return -1;
	    break;
	  case 'X':
	  case 'x':
	    darray_append(a, (void *) getv(i, j));
	    return -2;
	    break;
	  case '0' ... '4':
	    darray_append(a, (void *) getv(i, j));
	    return c - '0';
	    break;
	}
	die("input error");
      }
      board[i][j] = encode(c);
    }
    c = getchar();
    if (c != '\n') die("input error");
  }
  zdd_push();
  // Instead of constructing a ZDD excluding particular elements, we could
  // reduce the number of variables, but then we need to record which variable
  // represents which square.
  contains_none(a);

  for (uint32_t i = 0; i < rcount; i++) {
    for (uint32_t j = 0; j < ccount; j++) {
      switch(board[i][j]) {
	case -1:
	  // There must be at least one light bulb in this square or a
	  // square reachable in one rook move.
	  darray_remove_all(a);
	  int r0, c0;
	  for(r0 = i; r0 > 0 && board[r0 - 1][j] == -1; r0--);
	  for(int k = r0; k != i; k++) {
	    darray_append(a, (void *) getv(k, j));
	  }
	  for(c0 = j; c0 > 0 && board[i][c0 - 1] == -1; c0--);
	  for(int k = c0; k != j; k++) {
	    darray_append(a, (void *) getv(i, k));
	  }
	  darray_append(a, (void *) getv(i, j));
	  for(int k = j + 1; k < ccount && board[i][k] == -1; k++) {
	    darray_append(a, (void *) getv(i, k));
	  }
	  for(int k = i + 1; k < rcount && board[k][j] == -1; k++) {
	    darray_append(a, (void *) getv(k, j));
	  }
	  zdd_push();
	  contains_at_least_one(a);
	  // There is at most one light bulb in this row. We record this when
	  // we first enter the row.
	  if (r0 == i) {
	    darray_remove_all(a);
	    for(int k = i; k < rcount && board[k][j] == -1; k++) {
	      darray_append(a, (void *) getv(k, j));
	    }
	    zdd_push();
	    contains_at_most_one(a);
	    zdd_intersection();
	  }
	  // Similarly for columns.
	  if (c0 == j) {
	    darray_remove_all(a);
	    for(int k = j; k < ccount && board[i][k] == -1; k++) {
	      darray_append(a, (void *) getv(i, k));
	    }
	    zdd_push();
	    contains_at_most_one(a);
	    zdd_intersection();
	  }
	  zdd_intersection();
	  break;
	case 0:
	  darray_remove_all(a);
	  void check(int r, int c) {
	    if (r >= 0 && r < rcount && c >= 0 && c < ccount
		&& board[r][c] == -1) {
	      darray_append(a, (void *) getv(r, c));
	    }
	  }
	  check(i - 1, j);
	  check(i, j - 1);
	  check(i, j + 1);
	  check(i + 1, j);
void outwithit(void *data) {
  printf(" %d", (int) data);
}
printf("%d %d:", i, j);
darray_forall(a, outwithit); printf("\n");
	  zdd_push();
          //contains_none(a);
          contains_exactly_n(a, 0);
	  zdd_intersection();
      }
    }
  }

  zdd_dump();
  zdd_count();

  // Print lexicographically largest solution, assuming it exists.
  uint32_t v = zdd_root();
  while(v != 1) {
    int r = zdd_v(v) - 1;
    int c = r % ccount;
    r /= ccount;
    board[r][c] = -3;
    v = zdd_hi(v);
  }
  for (int i = 0; i < rcount; i++) {
    for (int j = 0; j < ccount; j++) {
      switch(board[i][j]) {
	case -3:
	  putchar('*');
	  break;
	case -2:
	  putchar('x');
	  break;
	case -1:
	  putchar('.');
	  break;
	default:
	  putchar('0' + board[i][j]);
	  break;
      }
    }
    putchar('\n');
  }
  return 0;
}
