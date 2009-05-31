// Solve a sudoku with ZDDs.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "inta.h"
#include "zdd.h"
#include <stdarg.h>
#include "io.h"

// Construct ZDD of sets containing exactly 1 digit at forall boxes
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
  zdd_push();
  int next = 9;
  uint32_t n = zdd_next_node();
  for(int i = 1; i <= 729; i++) {
    zdd_add_node(i, (i % 9) ? 1 : 0, -1);
    if (next < 729) {
      zdd_set_hi(zdd_last_node(), n + next);
    }
    if (!(i % 9)) next += 9;
  }
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
// 9*8^8*9^72 members. (Pick 1 of 9 positions ford, leaving 8 possible choices
// forthe remaining 8 boxes in that row. The other 81 - 9 boxes can contain
// any single digitS.)
// The intersection forall d and a fixed r has 9!*9^72 members.
// The intersection forall r and a fixed d has 9^9*8^72 members.
void unique_digit_per_row(int d, int r) {
  zdd_push();
  // The order is determined by sorting by number, then by letter.
  int next = 81 * r + d;  // The next node involving the digit d.
  int v = 1;
  int state = 0;
  while (v <= 729) {
    if (v == next) {
      next += 9;
      state++;
      if (state == 1) {
	// The first split in the ZDD.
	zdd_add_node(v, 1, 2);
      } else if (state < 9) {
	// Fix previous node. We must not have a second occurrence of d.
	uint32_t n = zdd_last_node();
	zdd_set_hilo(n, n + 3);
	// If this is the first occurrence of d, we're on notice.
	zdd_add_node(v, 1, 2);
      } else {
	// If we never saw d, then branch to FALSE.
	// Otherwise reunite the branches.
	zdd_add_node(v, 0, 1);
	next = -1;
      }
    } else if (state == 0 || state == 9) {
      zdd_add_node(v, 1, 1);
    } else {
      zdd_add_node(v, 2, 2);
      zdd_add_node(v, 2, 2);
    }
    v++;
  }
  // Fix last nodes.
  uint32_t n = zdd_last_node();
  if (zdd_lo(n - 1) > n) zdd_set_lo(n - 1, 1);
  if (zdd_hi(n - 1) > n) zdd_set_hi(n - 1, 1);
  if (zdd_lo(n) > n) zdd_set_lo(n, 1);
  if (zdd_hi(n) > n) zdd_set_hi(n, 1);
}

// Construct ZDD of sets containing all elements in the given list.
// The list is terminated by -1.
void contains_all(int *list) {
  zdd_push();
  int v = 1;
  int *next = list;
  while (v <= 729) {
    if (v == *next) {
      next++;
      zdd_add_node(v, 0, 1);
    } else {
      zdd_add_node(v, 1, 1);
    }
    v++;
  }
  // Fix 729.
  uint32_t n = zdd_last_node();
  if (zdd_lo(n) > n) zdd_set_lo(n, 1);
  if (zdd_hi(n) > n) zdd_set_hi(n, 1);
}

void unique_digit_per_col(int d, int col) {
  int list[9];
  for(int i = 0; i < 9; i++) {
    list[i] = 81 * i + 9 * col + d;
  }
  zdd_contains_exactly_1(list, 9);
}

void unique_digit_per_3x3(int d, int row, int col) {
  int list[9];
  for(int i = 0; i < 3; i++) {
    for(int j = 0; j < 3; j++) {
      list[i * 3 + j] = 81 * (i + 3 * row) + 9 * (j + 3 * col) + d;
    }
  }
  zdd_contains_exactly_1(list, 9);
}

int main() {
  zdd_init();
  // The universe is {1, ..., 9^3 = 729}.
  zdd_set_vmax(729);
  // Number rows and columns from 0. Digits are integers [1..9].
  // The digit d at (r, c) is represented by element 81 r + 9 c + d.
  inta_t list;
  inta_init(list);
  for(int i = 0; i < 9; i++) {
    for(int j = 0; j < 9; j++) {
      int c = getchar();
      if (EOF == c) die("unexpected EOF");
      if ('\n' == c) die("unexpected newline");
      if (c >= '1' && c <= '9') {
	inta_append(list, 81 * i + 9 * j + c - '0');
      }
    }
    int c = getchar();
    if (EOF == c) die("unexpected EOF");
    if ('\n' != c) die("expected newline");
  }
  inta_append(list, -1);
  contains_all(inta_raw(list));
  inta_clear(list);

  global_one_digit_per_box();
  zdd_intersection();

  // Number of ways you can put nine 1s into a sudoku is
  //   9*6*3*6*3*4*2*2.
  printf("rows\n");
  fflush(stdout);
  for(int i = 1; i <= 9; i++) {
    for(int r = 0; r < 9; r++) {
      unique_digit_per_row(i, r);
      if (r) zdd_intersection();
    }
    zdd_intersection();
  }
  for(int i = 1; i <= 9; i++) {
    for(int c = 0; c < 3; c++) {
      for(int r = 0; r < 3; r++) {
	printf("3x3 %d: %d, %d\n", i, r, c);
	fflush(stdout);
	unique_digit_per_3x3(i, r, c);
	if (r) zdd_intersection();
      }
      if (c) zdd_intersection();
    }
    zdd_intersection();
  }
  for(int i = 1; i <= 9; i++) {
    for(int c = 0; c < 9; c++) {
      printf("cols %d: %d\n", i, c);
      fflush(stdout);
      unique_digit_per_col(i, c);
      if (c) zdd_intersection();
    }
    zdd_intersection();
  }

  void printsol(int *v, int vcount) {
    for(int i = 0; i < vcount; i++) {
      putchar(((v[i] - 1) % 9) + '1');
      if (8 == (i % 9)) putchar('\n');
    }
    putchar('\n');
  }
  zdd_forall(printsol);
  return 0;
}
