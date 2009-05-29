// Solve a Dominosa puzzle.
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

uint32_t vmax;

// Construct ZDD of sets containing exactly 1 of the elements in the given list.
// Zero suppression means we must treat sequences in the list carefully.
void contains_exactly_one(darray_t a) {
  zdd_push();
  int v = 1;
  int i = 0;
  while(v <= vmax) {
    if (i >= darray_count(a)) {
      // Don't care about the rest of the elements.
      zdd_add_node(v++, 1, 1);
    } else if (v == (int) darray_at(a, i)) {
      // Find length of consecutive sequence.
      int k;
      for(k = 0;
	  i + k < darray_count(a) && v + k == (int) darray_at(a, i + k); k++);
      uint32_t n = zdd_next_node();
      uint32_t h = v + k > vmax ? 1 : n + k + (darray_count(a) != i + k);
      if (i >= 1) {
	// In the middle of the list: must fix previous node; we reach said node
	// if we've seen an element in the list already, in which case the
	// arrows must bypass the entire sequence, i.e. we need the whole
	// sequence to be out of the set.
	//set_node(n - 1, v - 1, h, h);
	zdd_set_hilo(n - 1, h);
      }
      i += k;
      k += v;
      while (v < k) {
	// If we see an element, bypass the rest of the sequence (see above),
	// otherwise we look for the next element in the sequence.
	zdd_add_node(v++, 1, 1);
	zdd_set_hi(zdd_last_node(), h);
	//set_node(n, v++, n + 1, h);
	//n++;
      }
      //v--;
      if (darray_count(a) == i) {
	// If none of the list showed up, then return false, otherwise,
	// onwards! (Through the rest of the elements to the end.)
	//set_node(n - 1, v, 0, h);
	zdd_set_lo(zdd_last_node(), 0);
	zdd_set_hi(zdd_last_node(), h);
      }
    } else if (!i) {
      // We don't care about the membership of elements before the list.
      zdd_add_node(v++, 1, 1);
    } else {
      zdd_add_node(v, 2, 2);
      zdd_add_node(v, 2, 2);
      v++;
    }
  }
  // Fix last node.
  uint32_t last = zdd_last_node();
  if (zdd_lo(last) > last) zdd_set_lo(last, 1);
  if (zdd_hi(last) > last) zdd_set_hi(last, 1);
}

int main() {
  zdd_init();

  int n;
  if (!scanf("%d\n", &n)) die("input error");
  uint32_t rcount = n + 1, ccount = n + 2;
  int board[rcount][ccount];
  darray_t list[rcount][ccount];

  for(int i = 0; i < rcount; i++) {
    int c;
    for(int j = 0; j < ccount; j++) {
      darray_init(list[i][j]);
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
  darray_t tally[n];
  for(int i = 0; i < n; i++) darray_init(tally[i]);

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
	darray_append(list[i][j], (void *) v);
	darray_append(list[i][j + 1], (void *) v);
	a = board[i][j];
	b = board[i][j + 1];
	if (a < b) c = a, a = b, b = c;
	// One domino per board constraint.
	darray_append(tally[geti(a, b)], (void *) v);
	v++;
      }

      if (i < rcount - 1) {
	// Vertical tiling constraint.
	darray_append(list[i][j], (void *) v);
	darray_append(list[i + 1][j], (void *) v);
	a = board[i][j];
	b = board[i + 1][j];
	if (a < b) c = a, a = b, b = c;
	// One domino per board constraint.
	darray_append(tally[geti(a, b)], (void *) v);
	v++;
      }
    }
  }
  vmax = v - 1;
  for(int i = 0; i < rcount; i++) {
    for(int j = 0; j < ccount; j++) {
      contains_exactly_one(list[i][j]);
      if (j) zdd_intersection();
    }
  }
  for(int i = 0; i < rcount - 1; i++) {
    zdd_intersection();
  }

  for(int i = 0; i < n; i++) {
    contains_exactly_one(tally[i]);
    if (i) zdd_intersection();
  }
  zdd_intersection();

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
