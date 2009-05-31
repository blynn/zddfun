// Solve a Light Up puzzle.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inta.h"
#include "zdd.h"
#include <stdarg.h>
#include "io.h"

int main() {
  zdd_init();

  uint32_t rcount, ccount;
  if (!scanf("%d %d\n", &rcount, &ccount)) die("input error");
  int board[rcount][ccount];
  uint32_t getv(uint32_t i, uint32_t j) { return ccount * i + j + 1; }
  zdd_set_vmax(getv(rcount - 1, ccount - 1));
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
  zdd_contains_0(inta_raw(a), inta_count(a));

  int colcount = 0;
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
	  zdd_contains_at_least_1(inta_raw(a), inta_count(a));
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
	    // Defer the intersection.
	    colcount++;
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
          zdd_contains_exactly_n(board[i][j], inta_raw(a), inta_count(a));
	  zdd_intersection();
      }
    }
    zdd_intersection();
  }

  while(colcount) {
    zdd_intersection();
    colcount--;
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
