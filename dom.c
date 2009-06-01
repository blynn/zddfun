// Solve a Dominosa puzzle.
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

  int n;
  if (!scanf("%d\n", &n)) die("input error");
  uint32_t rcount = n + 1, ccount = n + 2;
  int board[rcount][ccount];
  inta_t list[rcount][ccount];

  for(int i = 0; i < rcount; i++) {
    int c;
    for(int j = 0; j < ccount; j++) {
      inta_init(list[i][j]);
      c = getchar();
      if (c == EOF || c == '\n') die("input error");
      int encode(char c) {
	switch(c) {
	  case '0' ... '9':
	    return c - '0';
	    break;
	  case 'a' ... 'z':
	    return c - 'a' + 10;
	    break;
	  case 'A' ... 'Z':
	    return c - 'A' + 10;
	    break;
	}
	die("input error");
      }
      board[i][j] = encode(c);
    }
    c = getchar();
    if (c != '\n') die("input error");
  }

  // n is useless as the highest number on a domino. Reuse it as
  // the total number of dominoes.
  n = rcount * ccount / 2;
  inta_t tally[n];
  for(int i = 0; i < n; i++) inta_init(tally[i]);

  int geti(int a, int b) {
    // Return canonical numbering for a given domino.
    // Assumes a >= b.
    return a * (a + 1) / 2 + b;
  }

  int v = 1;
  for(int i = 0; i < rcount; i++) {
    for(int j = 0; j < ccount; j++) {
      int a, b, c;
      if (j < ccount - 1) {
	// Horizontal tiling constraint.
	inta_append(list[i][j], v);
	inta_append(list[i][j + 1], v);
	a = board[i][j];
	b = board[i][j + 1];
	if (a < b) c = a, a = b, b = c;
	// One domino per board constraint.
	inta_append(tally[geti(a, b)], v);
	v++;
      }

      if (i < rcount - 1) {
	// Vertical tiling constraint.
	inta_append(list[i][j], v);
	inta_append(list[i + 1][j], v);
	a = board[i][j];
	b = board[i + 1][j];
	if (a < b) c = a, a = b, b = c;
	// One domino per board constraint.
	inta_append(tally[geti(a, b)], v);
	v++;
      }
    }
  }
  zdd_set_vmax(v - 1);

  int todo = rcount - 1;
  for(int i = 0; i < rcount; i++) {
    for(int j = 0; j < ccount; j++) {
      zdd_contains_exactly_1(inta_raw(list[i][j]), inta_count(list[i][j]));
      if (j) zdd_intersection();
    }

    int n = i + 1;
    while (!(n & 1)) {
      n >>= 1;
      zdd_intersection();
      todo--;
    }
  }
  while(todo--) zdd_intersection();

  int compar(const void *a, const void *b) {
    return inta_count((const inta_ptr) a) > inta_count((const inta_ptr) b);
  }
  qsort(tally, n, sizeof(inta_t), compar);
  for(int i = 0; i < n; i++) {
    //printf("%d/%d\n", i + 1, n);
    zdd_contains_exactly_1(inta_raw(tally[i]), inta_count(tally[i]));
    zdd_intersection();
  }

  // Print lexicographically largest solution, assuming it exists.
  char pic[2 * rcount + 1][2 * ccount + 1];
  for(int i = 0; i < rcount; i++) {
    for(int j = 0; j < ccount; j++) {
      pic[2 * i][2 * j] = board[i][j] + '0';
      pic[2 * i][2 * j + 1] = '|';
      pic[2 * i + 1][2 * j] = '-';
      pic[2 * i + 1][2 * j + 1] = '+';
    }
    pic[2 * i][2 * ccount] = pic[2 * i + 1][2 * ccount] = '\0';
  }
  v = zdd_root();
  while(v != 1) {
    int i = zdd_v(v) / (rcount + ccount);
    int j = zdd_v(v) % (rcount + ccount);
    if (!j) {
      pic[2 * i - 1][2 * (ccount - 1)] = ' ';
    } else if (rcount - 1 == i) {
      pic[2 * i][2 * j - 1] = ' ';
    } else {
      if (j & 1) {
	pic[2 * i][j] = ' ';
      } else {
	pic[2 * i + 1][j - 2] = ' ';
      }
    }
    v = zdd_hi(v);
  }
  for(int i = 0; i < 2 * rcount; i++) {
    puts(pic[i]);
  }
  return 0;
}
