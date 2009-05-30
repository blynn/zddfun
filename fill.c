// Solve a Fillomino puzzle.
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

// Construct ZDD of sets containing exactly 1 element for each interval
// [a_k, a_{k+1}) in given list. List must start with a_0 = 1, while there is an
// implied vmax + 1 at end of list, so the last interval is [a_n, vmax + 1).
//
// The ZDD begins:
//   1 ... 2
//   1 --- a_1
//   2 ... 3
//   2 --- a_1
//   ...
//   a_1 - 1 ... F
//   a_1 - 1 --- a_1
//
// and so on:
//   a_k ... a_k + 1
//   a_k --- a_{k+1}
// and so on until vmax --- F, vmax ... T.
void one_per_interval(const int* list, int count) {
  zdd_push();
  // Check list[0] is 1.
  int i = 0;
  uint32_t n = zdd_last_node();
  int get() {
    i++;
    //return i < darray_count(a) ? (int) darray_at(a, i) : -1;
    return i < count ? list[i] : -1;
  }
  int target = get();
  for (int v = 1; v <= vmax; v++) {
    zdd_abs_node(v, n + v + 1, target > 0 ? n + target : 1);
    if (v == target - 1 || v == vmax) {
      zdd_set_lo(zdd_last_node(), 0);
      target = get();
    }
  }
}

int main() {
  zdd_init();

  if (!scanf("%d %d\n", &rcount, &ccount)) die("input error");
  int board[rcount][ccount];
  darray_t list[rcount][ccount];
  darray_t must[rcount * ccount];

  for (int i = 0; i < rcount; i++) {
    int c;
    for (int j = 0; j < ccount; j++) {
      darray_init(list[i][j]);
      darray_init(must[i * ccount + j]);
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
  darray_t r, c, growth, dupe, psize;
  darray_init(r);
  darray_init(c);
  darray_init(growth);
  darray_init(dupe);
  darray_init(psize);
  for (int i = 0; i < rcount; i++) {
    for (int j = 0; j < ccount; j++) {
      int n = board[i][j];
      if (n) {
	// We store more information than we need in scratch for debugging.
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
	    // Grown to the right size.
	    darray_append(list[i][j], (void *) v);
	    void addv(void *data) {
	      int k = (int) data;
	      darray_append(list[(int) darray_at(r, k)][(int) darray_at(c, k)], (void *) v);
	    }
	    darray_forall(growth, addv);
	    darray_append(must[i * ccount + j], (void *) v);
	    void addmust(void *data) {
	      darray_append(must[(int) data], (void *) v);
	    }
	    darray_forall(dupe, addmust);
	    darray_append(psize, (void *) n);
	    v++;
	    return;
	  }
	  void check(int i, int j) {
	    // See if square is suitable for growing polyomino.
	    if (i >= 0 && i < rcount && j >= 0 && j < ccount &&
		!scratch[i][j] &&
		(board[i][j] == n || board[i][j] == 0)) {
	      // Have we seen this polyomino before?
	      if (board[i][j] == n) {
		if ((i < x || (i == x && j < y))) {
		  // We already handled it last time we encountered it.
		  return;
		} else {
		  darray_append(dupe, (void *) (i * ccount + j));
		}
	      }
	      darray_append(r, (void *) i);
	      darray_append(c, (void *) j);
	      scratch[i][j] = darray_count(r);
	    }
	  }
	  int old = darray_count(r);
	  int olddupecount = darray_count(dupe);
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
	  darray_set_count(dupe, olddupecount);
	}
	recurse(i, j, 1, 0);
	darray_remove_all(r);
	darray_remove_all(c);
      }
    }
  }
  vmax = v - 1;
  printf("vmax: %d\n", vmax);
  for (int i = 0; i < rcount * ccount; i++) {
    void pit(void *data) { printf(" %d", (int) data); }
    printf("%d:", i);
    darray_forall(must[i], pit);
    printf("\n");
    contains_exactly_one(must[i]);
    zdd_intersection();
  }

  for (int i = 0; i < rcount; i++) {
    int first = 1;
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
    zdd_intersection();
  }

  zdd_dump();
  zdd_count();

  // Print lexicographically largest solution, assuming it exists.
  uint32_t hi = zdd_root();
  while(hi != 1) {
    int n = zdd_v(hi);
    for (int i = 0; i < rcount; i++) {
      for (int j = 0; j < ccount; j++) {
	if (darray_index_of(list[i][j], (void *) n) >= 0) {
	  board[i][j] = (int) darray_at(psize, n - 1);
	}
      }
    }
    hi = zdd_hi(hi);
  }
  for (int i = 0; i < rcount; i++) {
    for (int j = 0; j < ccount; j++) {
      putchar(board[i][j] < 10 ? '0' + board[i][j] : 'A' + board[i][j] - 10);
    }
    putchar('\n');
  }
  return 0;
}
