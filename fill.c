// Solve a Fillomino puzzle.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "darray.h"
#include "zdd.h"
#include <stdarg.h>
#include "io.h"

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
  darray_t white[rcount][ccount];
  darray_t black[rcount][ccount];
  darray_t must[rcount * ccount];

  for (int i = 0; i < rcount; i++) {
    int c;
    for (int j = 0; j < ccount; j++) {
      darray_init(white[i][j]);
      darray_init(black[i][j]);
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
  darray_t r, c, blackr, blackc, growth, dupe, psize, adj;
  darray_init(r);
  darray_init(c);
  darray_init(blackr);
  darray_init(blackc);
  darray_init(growth);
  darray_init(dupe);
  darray_init(psize);
  darray_init(adj);
  // Sacrifice a void * so we can index psize from 1.
  darray_append(psize, NULL);

  int onboard(int x, int y) {
    return x >= 0 && x < rcount && y >= 0 && y < ccount;
  }

  for (int i = 0; i < rcount; i++) {
    for (int j = 0; j < ccount; j++) {
      int n = board[i][j];
      if (n) {
	// We store more information than we need in scratch for debugging.
	// The entry [i][j] is:
	//  -1 when being considered as part of the current polyomino
	//  -2 when already searched from the same home square
	//  -3 when already searched from a different home square
	//   0 when not involved yet
	//  >0 when a potential site to grow the current polyomino
	int scratch[rcount][ccount];
	memset(scratch, 0, rcount * ccount * sizeof(int));
	scratch[i][j] = -1;

	void recurse(int x, int y, int m, int lower) {
	  int intat(darray_t a, int i) { return (int) darray_at(a, i); }
	  // Even when m = n we look for sites to grow the polyomino, because
	  // in this case, we blacklist these squares; no other n-omino
	  // may intersect these squares.
	  void check(int x, int y) {
	    // See if square is suitable for growing polyomino.
	    if (onboard(x,y) && !scratch[x][y] &&
		(board[x][y] == n || board[x][y] == 0)) {
	      darray_append(r, (void *) x);
	      darray_append(c, (void *) y);
	      scratch[x][y] = darray_count(r);
	      // Have we seen this polyomino before?
	      if (board[x][y] == n) {
		if ((x < i || (x == i && y < j))) {
		  // We already handled it last time we encountered it.
		  // Mark it as already searched.
		  scratch[x][y] = -3;
		} else {
		  darray_append(dupe, (void *) (x * ccount + y));
		}
	      }
	    }
	  }
	  int old = darray_count(r);
	  int olddupecount = darray_count(dupe);
	  check(x - 1, y);
	  check(x, y + 1);
	  check(x + 1, y);
	  check(x, y - 1);

	  if (m == n) {
	    // It has grown to the right size.
	    // Check squares adjacent to polyomino. We need only check
	    // the unsearched growth sites and squares already searched because
	    // these are the only potential trouble spots.
	    for(int k = 0; k < darray_count(r); k++) {
	      int x = intat(r, k);
	      int y = intat(c, k);
	      if (scratch[x][y] != -1) {
		if (board[x][y] == n) {
		  // An n-polyomino cannot border a square clued with n.
		  darray_remove_all(blackr);
		  darray_remove_all(blackc);
		  goto abort;
		} else {
		  darray_append(blackr, (void *) x);
		  darray_append(blackc, (void *) y);
		}
	      }
	    }
    /*
    printf("v %d\n", v);
    for(int a = 0; a < rcount; a++) {
      for (int b = 0; b < ccount; b++) {
	putchar(scratch[a][b] + '0');
      }
      putchar('\n');
    }
    putchar('\n');
    */

	    darray_append(psize, (void *) n);
	    void checkconflict(void *data) {
	      // If the other polyomino covers our home square then don't
	      // bother reporting the conflict, because we handle this case
	      // elsewhere.
	      if (darray_index_of(must[i * ccount + j], data) >= 0) return;
	      int w = (int) data;
	      if ((int) darray_at(psize, w) == n) {
		darray_ptr a = (darray_ptr) malloc(sizeof(darray_t));
		darray_init(a);
		darray_append(adj, (void *) a);
		darray_append(a, (void *) w);
		darray_append(a, (void *) v);
		//printf("conflict %d %d\n", w, v);
	      }
	    }
	    for(int k = 0; k < darray_count(blackr); k++) {
	      int x = intat(blackr, k);
	      int y = intat(blackc, k);
	      darray_append(black[x][y], (void *) v);
	      darray_forall(white[x][y], checkconflict);
	    }
	    darray_remove_all(blackr);
	    darray_remove_all(blackc);
	    darray_append(white[i][j], (void *) v);
	    void addv(void *data) {
	      int k = (int) data;
	      int x = intat(r, k);
	      int y = intat(c, k);
	      darray_append(white[x][y], (void *) v);
	    }
	    darray_forall(growth, addv);
	    darray_append(must[i * ccount + j], (void *) v);
	    void addmust(void *data) {
	      darray_append(must[(int) data], (void *) v);
	    }
	    darray_forall(dupe, addmust);
	    v++;
	  } else {
	    // Recurse through each growing site.
	    for(int k = lower; k < darray_count(r); k++) {
	      int x = intat(r, k);
	      int y = intat(c, k);
	      if (scratch[x][y] != -3) {
		darray_append(growth, (void *) k);
		scratch[x][y] = -1;
		recurse(x, y, m + 1, k + 1);
		scratch[x][y] = -2;
		darray_remove_last(growth);
	      }
	    }
	  }
abort:
	  for(int k = old; k < darray_count(r); k++) {
	    scratch[intat(r, k)][intat(c, k)] = 0;
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
  zdd_set_vmax(v - 1);
  vmax = v - 1;
  // Each clue n must be covered by exactly one n-polyomino.
  for (int i = 0; i < rcount * ccount; i++) {
    if (darray_count(must[i]) > 0) {
      contains_exactly_one(must[i]);
      zdd_intersection();
    }
  }

  // Othewise, each square can be covered by at most one of the polyominoes we
  // enumerated.
  for (int i = 0; i < rcount; i++) {
    int first = 1;
    for (int j = 0; j < ccount; j++) {
      if (board[i][j] != 0) continue;
      //printf("%d %d:", i, j);
      //void dump(void *data) {
	//printf(" %d", (int) data);
      //}
      //darray_forall(white[i][j], dump);
      //printf("\n");
      if (darray_count(white[i][j]) > 1) {
	contains_at_most_one(white[i][j]);
	if (first) first = 0; else zdd_intersection();
      }
    }
    zdd_intersection();
  }

  // Adjacent polyominoes must differ in size.
  void handleadj(void *data) {
    darray_ptr list = (darray_ptr) data;
    contains_at_most_one(list);
    zdd_intersection();
  }
  darray_forall(adj, handleadj);

  int solcount = 0;
  void printsol(int *v, int count) {
    char pic[rcount][ccount];
    memset(pic, '?', rcount * ccount);
    for(int k = 0; k < count; k++) {
      for (int i = 0; i < rcount; i++) {
	for (int j = 0; j < ccount; j++) {
	  if (darray_index_of(white[i][j], (void *) v[k]) >= 0) {
	    int n = (int) darray_at(psize, v[k]);
	    pic[i][j] = n < 10 ? '0' + n : 'A' + n - 10;
	  }
	}
      }
    }
    // Now we have a new problem: tile the remaining squares with polyominoes
    // of arbitrary size to satisfy the puzzle conditions.
    // TODO: Brute force search to finish the puzzle. For now, we only check
    // the simplest cases.
    for (int i = 0; i < rcount; i++) {
      for (int j = 0; j < ccount; j++) {
	int scratch[rcount][ccount];
	memset(scratch, 0, rcount * ccount * sizeof(int));

	void neighbour_run(void (*fn)(int, int), int i, int j) {
	  fn(i - 1, j);
	  fn(i + 1, j);
	  fn(i, j - 1);
	  fn(i, j + 1);
	}
	if ('?' == pic[i][j]) {
	  int count = 0;
	  void paint(int i, int j) {
	    scratch[i][j] = -1;
	    darray_append(r, (void *) i);
	    darray_append(c, (void *) j);
	    count++;
	    void bleed(int i, int j) {
	      if (onboard(i, j) &&
	       	  pic[i][j] == '?' && !scratch[i][j]) paint(i, j);
	    }
	    neighbour_run(bleed, i, j);
	  }
	  paint(i, j);
	  if (count == 1) {
	    int n1 = 0;
	    void count1(int i, int j) {
	      if (onboard(i, j) && pic[i][j] == '1') n1++;
	    }
	    neighbour_run(count1, i, j);
	    if (n1) {
	      darray_remove_all(r);
	      darray_remove_all(c);
	      return;
	    }
	    pic[i][j] = '1';
	  } else if (count == 2) {
	    int n1 = 0;
	    void count1(int i, int j) {
	      if (onboard(i, j) && pic[i][j] == '2') {
		n1++;
	      }
	    }
	    int x = (int) darray_at(r, 0);
	    int y = (int) darray_at(c, 0);
	    neighbour_run(count1, x, y);
	    x = (int) darray_at(r, 1);
	    y = (int) darray_at(c, 1);
	    neighbour_run(count1, x, y);
	    if (n1) {
	      darray_remove_all(r);
	      darray_remove_all(c);
	      return;
	    }
	    x = (int) darray_at(r, 0);
	    y = (int) darray_at(c, 0);
	    pic[x][y] = '2';
	    x = (int) darray_at(r, 1);
	    y = (int) darray_at(c, 1);
	    pic[x][y] = '2';
	  }
	  darray_remove_all(r);
	  darray_remove_all(c);
	}
      }
    }

    printf("Solution #%d:\n", ++solcount);
    for (int i = 0; i < rcount; i++) {
      for (int j = 0; j < ccount; j++) {
	putchar(pic[i][j]);
      }
      putchar('\n');
    }
    putchar('\n');
  }
  zdd_forall(printsol);
  return 0;
}
