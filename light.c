// Solve a Light Up puzzle.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
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

int main() {
  zdd_init();

  if (!scanf("%d %d\n", &rcount, &ccount)) die("input error");
  int board[rcount][ccount];
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
	    return -2;
	    break;
	  case '0' ... '4':
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
  darray_t a;
  darray_init(a);
  int flag = 0;
  uint32_t getv(uint32_t i, uint32_t j) { return ccount * i + j + 1; }
  vmax = getv(rcount - 1, ccount - 1);

  for (uint32_t i = 0; i < rcount; i++) {
    for (uint32_t j = 0; j < ccount; j++) {
      switch(board[i][j]) {
	//case -2:
	case -1:
      printf("%d %d\n", i, j);
	  darray_remove_all(a);
	  int k;
	  for(k = i - 1; k >= 0 && board[k][j] == -1; k--);
	  for(k++; k != i; k++) {
	    darray_append(a, (void *) getv(k, j));
	  }
	  for(k = j - 1; k >= 0 && board[i][k] == -1; k--);
	  for(k++; k != j; k++) {
	    darray_append(a, (void *) getv(i, k));
	  }
	  darray_append(a, (void *) getv(i, j));
	  for(k = j + 1; k < ccount && board[i][k] == -1; k++) {
	    darray_append(a, (void *) getv(i, k));
	  }
	  for(k = i + 1; k < rcount && board[k][j] == -1; k++) {
	    darray_append(a, (void *) getv(k, j));
	  }
void outwithit(void *data) {
  printf(" %d", (int) data);
}
darray_forall(a, outwithit); printf("\n");
	  zdd_push();
	  contains_at_least_one(a);
	  if (flag) zdd_intersection(); else flag = 1;
	  break;
      }
    }
  }

  zdd_dump();
  zdd_count();
  return 0;
}
