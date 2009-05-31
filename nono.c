// Nonogram solver.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "zdd.h"
#include <stdarg.h>
#include "io.h"

static int max;

// Build ZDD for given row clues, and the root of the ZDD for all later rows.
// Call this from the bottom row to the top row to produce the ZDD of all
// sets satisfying the row clues.
uint32_t add_row_clue(int row, int *a, int size, int root) {
  uint32_t v = row * max + 1;
  uint32_t table[max][max + 1];
  uint32_t partial_row(uint32_t i, int *a, int count, int sum) {
    // Return old result if already computed.
    if (table[i][count] != 0) return table[i][count];
    // Handle the case when the first coloured segment appears immediately.
    uint32_t first, last;
    first = last = table[i][count] = zdd_add_node(v + i, 0, 1);
    for(int j = 1; j < *a; j++) {
      last = zdd_add_node(v + i + j, 0, 1);
    }
    if (1 == count) {
      // The last number in the clue for this row.
      zdd_set_hi(last, root);
    } else {
      // Hop over the next node; it must be false, as it represents a gap
      // between two coloured segments.
      zdd_set_hi(last, partial_row(i + *a + 1, a + 1, count - 1, sum - *a));
    }
    // Recurse for all other cases, if applicable.
    if (max - count - sum + 1 - i > 0) {
      zdd_set_lo(first, partial_row(i + 1, a, count, sum));
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
  memset(table, 0, sizeof(uint32_t) * (max + 1) * (max + 1));
  // Variables outside this column can be in our out, so until we reach
  // the last node we send both edges to the next node.
  uint32_t tail(uint32_t v, uint32_t i) {
    if (table[i][0] != 0) return table[i][0];
    table[i][0] = zdd_next_node();
    if (max == i) {
      while(v < max * max) zdd_add_node(v++, 1, 1);
      zdd_add_node(v, -1, -1);
    } else {
      // Keep adding nodes until we reach this column again. Hop over the
      // next node because it is part of the blank space after the last
      // coloured segment in this column.
      while(v < max * i + col + 1) zdd_add_node(v++, 1, 1);
      v++;
      uint32_t last = zdd_last_node();
      zdd_set_hilo(last, tail(v, i + 1));
    }
    return table[i][0];
  }
  uint32_t partial_col(uint32_t v, uint32_t i, int *a, int count, int sum) {
    if (table[i][count] != 0) return table[i][count];
    uint32_t first, last;
    table[i][count] = zdd_next_node();
    // Variables outside this column can be in our out.
    while(v < col + 1 + i * max) zdd_add_node(v++, 1, 1);
    first = last = zdd_add_node(v++, 0, 1);
    uint32_t oldv = v;
    for(int j = 1; j < *a; j++) {
      while(v < col + 1 + (i + j) * max) zdd_add_node(v++, 1, 1);
      last = zdd_add_node(v++, 0, 1);
    }
    if (1 == count) {
      zdd_set_hi(last, tail(v, i + *a));
    } else {
      while(v < col + 1 + (i + *a) * max) zdd_add_node(v++, 1, 1);
      v++;
      last = zdd_last_node();
      zdd_set_hilo(last,
	  partial_col(v, i + *a + 1, a + 1, count - 1, sum - *a));
    }
    if (max - count - sum + 1 - i > 0) {
      zdd_set_lo(first, partial_col(oldv, i + 1, a, count, sum));
    }
    return table[i][count];
  }

  int sum = 0;
  for (int i = 0; i <= max; i++) for (int j = 0; j <= max; j++) table[i][j] = 0;
  for (int i = 0; i < size; i++) sum += a[i];
  if (!size) return tail(1, 0);
  return partial_col(1, 0, a, size, sum);
}

int main() {
  zdd_init();
  if (!scanf("%d\n", &max)) die("input error");
  // Read max row clues, then max column clues.
  int clue[max * 2][max + 1];
  for(int i = 0; i < max * 2; i++) {
    char s[80];
    if (!fgets(s, 80, stdin)) {
      die("input error");
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
  zdd_push();
  uint32_t root = 1;
  for(int i = max - 1; i >= 0; i--) {
    if (clue[i][0]) {
      root = add_row_clue(i, &clue[i][1], clue[i][0], root);
    }
  }
  zdd_set_root(root);

  //printf("all rows: %d\n", zdd_next_node());
  // Intersect each column clue into the ZDD
  for(int i = 0; i < max; i++) {
    zdd_push();
    compute_col_clue(i, &clue[i + max][1], clue[i + max][0]);
    //printf("column %d: %d\n", i, zdd_next_node());
    zdd_intersection();
    //printf("intersected: %d\n", zdd_next_node());
  }

  zdd_check();

  // Print lexicographically largest solution, assuming it exists.
  int board[max][max];
  memset(board, 0, sizeof(int) * max * max);
  uint32_t v = zdd_root();
  while(v != 1) {
    int r = zdd_v(v) - 1;
    int c = r % max;
    r /= max;
    board[r][c] = 1;
    v = zdd_hi(v);
  }
  for (int i = 0; i < max; i++) {
    for (int j = 0; j < max; j++) {
      putchar(board[i][j] ? 'X' : '.');
    }
    putchar('\n');
  }

  return 0;
}
