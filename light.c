// Solve a Light Up puzzle.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inta.h"
#include "zdd.h"
#include <stdarg.h>
#include "io.h"

uint32_t rcount, ccount;
uint32_t vmax;

// Construct ZDD of sets containing exactly n of the elements in the
// given list.
void contains_exactly_n(inta_t a, int n) {
  zdd_push();
  uint32_t tab[inta_count(a)][n + 1];
  memset(tab, 0, inta_count(a) * (n + 1) * sizeof(uint32_t));
  uint32_t recurse(int i, int n) {
    int v = -1 == i ? 1 : inta_at(a, i) + 1;
    uint32_t root;
    if (i == inta_count(a) - 1) {
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
    int v1 = inta_at(a, i + 1);
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
    if (n < inta_count(a) - i - 1) {
      zdd_set_lo(last, recurse(i + 1, n));
    }
    if (-1 != i) tab[i][n] = root;
    return root;
  }
  recurse(-1, n);
}

// Construct ZDD of sets containing at least 1 of the elements in the
// given list.
void contains_at_least_one(inta_t a) {
  uint32_t n = zdd_last_node();
  // Start with ZDD of all sets.
  int v = 1;
  while(v < vmax) {
    zdd_add_node(v++, 1, 1);
  }
  zdd_add_node(v, -1, -1);
  if (inta_is_empty(a)) return;

  // Construct new branch for when elements of the list are not found.
  v = inta_first(a);
  if (1 == inta_count(a)) {
    zdd_set_lo(n + v, 0);
    return;
  }

  uint32_t n1 = zdd_next_node();
  zdd_set_lo(n + v, n1);
  v++;
  for(int i = 1; i < inta_count(a); i++) {
    int v1 = inta_at(a, i);
    while(v <= v1) {
      zdd_add_node(v++, 1, 1);
    }
    zdd_set_hi(zdd_last_node(), n + v);
  }

  zdd_set_lo(zdd_last_node(), 0);
  if (vmax < v) zdd_set_hi(zdd_last_node(), 1);
}

// Construct ZDD of sets not containing any elements from the given list.
// Assumes not every variable is on the list.
void contains_none(inta_t a) {
  zdd_push();
  int i = 1;
  int v1 = inta_is_empty(a) ? -1 : inta_first(a);
  for(int v = 1; v <= vmax; v++) {
    if (v1 == v) {
      if (i < inta_count(a)) {
	v1 = inta_at(a, i++);
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
  zdd_set_vmax(vmax);
  inta_t a;
  inta_init(a);

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
	    inta_append(a, getv(i, j));
	    return -2;
	    break;
	  case '0' ... '4':
	    inta_append(a, getv(i, j));
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
  // Instead of constructing a ZDD excluding particular elements, we could
  // reduce the number of variables, but then we need to record which variable
  // represents which square.
  contains_none(a);

  for (uint32_t i = 0; i < rcount; i++) {
    inta_remove_all(a);
    zdd_contains_at_most_1(inta_raw(a), inta_count(a));
    for (uint32_t j = 0; j < ccount; j++) {
      switch(board[i][j]) {
	case -1:
	  // There must be at least one light bulb in this square or a
	  // square reachable in one rook move.
	  inta_remove_all(a);
	  int r0, c0;
	  for(r0 = i; r0 > 0 && board[r0 - 1][j] == -1; r0--);
	  for(int k = r0; k != i; k++) {
	    inta_append(a, getv(k, j));
	  }
	  for(c0 = j; c0 > 0 && board[i][c0 - 1] == -1; c0--);
	  for(int k = c0; k != j; k++) {
	    inta_append(a, getv(i, k));
	  }
	  inta_append(a, getv(i, j));
	  for(int k = j + 1; k < ccount && board[i][k] == -1; k++) {
	    inta_append(a, getv(i, k));
	  }
	  for(int k = i + 1; k < rcount && board[k][j] == -1; k++) {
	    inta_append(a, getv(k, j));
	  }
	  zdd_push();
	  contains_at_least_one(a);
	  // There is at most one light bulb in this row. We record this when
	  // we first enter the row.
	  if (r0 == i) {
	    inta_remove_all(a);
	    for(int k = i; k < rcount && board[k][j] == -1; k++) {
	      inta_append(a, getv(k, j));
	    }
	    zdd_contains_at_most_1(inta_raw(a), inta_count(a));
	    zdd_intersection();
	  }
	  // Similarly for columns.
	  if (c0 == j) {
	    inta_remove_all(a);
	    for(int k = j; k < ccount && board[i][k] == -1; k++) {
	      inta_append(a, getv(i, k));
	    }
	    zdd_contains_at_most_1(inta_raw(a), inta_count(a));
	    zdd_intersection();
	  }
	  zdd_intersection();
	  break;
	case 0 ... 4:
	  inta_remove_all(a);
	  void check(int r, int c) {
	    if (r >= 0 && r < rcount && c >= 0 && c < ccount
		&& board[r][c] == -1) {
	      inta_append(a, getv(r, c));
	    }
	  }
	  check(i - 1, j);
	  check(i, j - 1);
	  check(i, j + 1);
	  check(i + 1, j);
          contains_exactly_n(a, board[i][j]);
	  zdd_intersection();
      }
    }
    zdd_intersection();
  }

  void printsol(int *v, int vcount) {
    char pic[rcount][ccount];
    memset(pic, '.', rcount * ccount);
    for(int i = 0; i < vcount; i++) {
      int r = v[i] - 1;
      int c = r % ccount;
      r /= ccount;
      pic[r][c] = '*';
    }

    for (int i = 0; i < rcount; i++) {
      for (int j = 0; j < ccount; j++) {
	switch(board[i][j]) {
	  case -2:
	    putchar('x');
	    break;
	  case 0 ... 4:
	    putchar('0' + board[i][j]);
	    break;
	  default:
	    putchar(pic[i][j]);
	}
      }
      putchar('\n');
    }
    putchar('\n');
  }

  zdd_forall(printsol);
  return 0;
}
