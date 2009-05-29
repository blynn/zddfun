// Solve a Fillomino puzzle
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

// Construct ZDD of sets containing at most 1 of the elements in the given
// list.
void contains_at_most_one(darray_t a) {
  zdd_push();
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

int main() {
  zdd_init();

  if (!scanf("%d %d\n", &rcount, &ccount)) die("input error");
  int board[rcount][ccount];
  darray_t list[rcount][ccount];
  uint32_t getv(uint32_t i, uint32_t j) { return ccount * i + j + 1; }
  vmax = getv(rcount - 1, ccount - 1);
  darray_t a;
  darray_init(a);

  for (int i = 0; i < rcount; i++) {
    int c;
    for (int j = 0; j < ccount; j++) {
      darray_init(list[i][j]);
      c = getchar();
      if (c == EOF || c == '\n') die("input error");
      int encode(char c) {
	switch(c) {
	  case '.':
	    return 0;
	    break;
	  case '1' ... '9':
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
  int v = 1;
  darray_t r, c, growth;
  darray_init(r);
  darray_init(c);
  darray_init(growth);
  void grr() {
  for (int i = 0; i < rcount; i++) {
    for (int j = 0; j < ccount; j++) {
      printf("%d %d:", i, j);
      void dump(void *data) {
	printf(" %d", (int) data);
      }
      darray_forall(list[i][j], dump);
      printf("\n");
    }
  }
  }
  for (int i = 0; i < rcount; i++) {
    for (int j = 0; j < ccount; j++) {
      int n = board[i][j];
      if (n) {
	// Scratch only needs to be a bit field, but it's current form is good
	// for debugging.
	int scratch[rcount][ccount];
	memset(scratch, 0, rcount * ccount * sizeof(int));
	scratch[i][j] = -1;

	void recurse(int x, int y, int m, int lower) {
    /*
    for(int a = 0; a < rcount; a++) {
      for (int b = 0; b < ccount; b++) {
	putchar(scratch[a][b] + '0');
      }
      putchar('\n');
    }
    putchar('\n');
    */
	  if (m == n) {
	    darray_append(list[i][j], (void *) v);
	    void addv(void *data) {
	      int k = (int) data;
	      darray_append(list[(int) darray_at(r, k)][(int) darray_at(c, k)], (void *) v);
	    }
	    darray_forall(growth, addv);
	    v++;
	    return;
	  }
	  void check(int x, int y) {
	    // See if square is suitable for growing polyomino.
	    if (x >= 0 && x < rcount && y >= 0 && y < ccount &&
		(board[x][y] == n || board[x][y] == 0) &&
		!scratch[x][y]) {
	      darray_append(r, (void *) x);
	      darray_append(c, (void *) y);
	      scratch[x][y] = darray_count(r);
	    }
	  }
	  int old = darray_count(r);
	  check(x - 1, y);
	  check(x, y + 1);
	  check(x + 1, y);
	  check(x, y - 1);
	  // Recurse through each growing site.
	  for(int k = lower; k < darray_count(r); k++) {
	    darray_append(growth, (void *) k);
	    scratch[(int) darray_at(r, k)][(int) darray_at(c, k)] = -1;
	    recurse((int) darray_at(r, k), (int) darray_at(c, k), m + 1, k + 1);
	    scratch[(int) darray_at(r, k)][(int) darray_at(c, k)] = -2;
	    darray_remove_last(growth);
	  }
	  for(int k = old; k < darray_count(r); k++) {
	    scratch[(int) darray_at(r, k)][(int) darray_at(c, k)] = 0;
	  }
	  darray_set_count(r, old);
	  darray_set_count(c, old);
	}
	recurse(i, j, 1, 0);
	darray_remove_all(r);
	darray_remove_all(c);
      }
    }
  }
  int vmax = v - 1;
  int first = 1;
  printf("%d\n", vmax);
  for (int i = 0; i < rcount; i++) {
    for (int j = 0; j < ccount; j++) {
      printf("%d %d:", i, j);
      void dump(void *data) {
	printf(" %d", (int) data);
      }
      darray_forall(list[i][j], dump);
      printf("\n");
      if (darray_count(list[i][j]) > 1) {
	contains_at_most_one(list[i][j]);
	if (first) first = 0; else zdd_intersection();
      }
    }
  }
  zdd_dump();
  zdd_count();
  return 0;
}
