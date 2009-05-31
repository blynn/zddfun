// Solve a Fillomino puzzle.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "darray.h"
#include "inta.h"
#include "zdd.h"
#include <stdarg.h>
#include "io.h"

int main() {
  zdd_init();

  uint32_t rcount, ccount;
  if (!scanf("%d %d\n", &rcount, &ccount)) die("input error");
  int board[rcount][ccount];
  inta_t white[rcount][ccount];
  inta_t black[rcount][ccount];
  inta_t must[rcount * ccount];

  for (int i = 0; i < rcount; i++) {
    int c;
    for (int j = 0; j < ccount; j++) {
      inta_init(white[i][j]);
      inta_init(black[i][j]);
      inta_init(must[i * ccount + j]);
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
  inta_t r, c, blackr, blackc, growth, dupe, psize;
  darray_t adj;
  inta_init(r);
  inta_init(c);
  inta_init(blackr);
  inta_init(blackc);
  inta_init(growth);
  inta_init(dupe);
  inta_init(psize);
  darray_init(adj);
  // Sacrifice a void * so we can index psize from 1.
  inta_append(psize, 0);

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
	  // Even when m = n we look for sites to grow the polyomino, because
	  // in this case, we blacklist these squares; no other n-omino
	  // may intersect these squares.
	  void check(int x, int y) {
	    // See if square is suitable for growing polyomino.
	    if (onboard(x,y) && !scratch[x][y] &&
		(board[x][y] == n || board[x][y] == 0)) {
	      inta_append(r, x);
	      inta_append(c, y);
	      scratch[x][y] = inta_count(r);
	      // Have we seen this polyomino before?
	      if (board[x][y] == n) {
		if ((x < i || (x == i && y < j))) {
		  // We already handled it last time we encountered it.
		  // Mark it as already searched.
		  scratch[x][y] = -3;
		} else {
		  inta_append(dupe, x * ccount + y);
		}
	      }
	    }
	  }
	  int old = inta_count(r);
	  int olddupecount = inta_count(dupe);
	  check(x - 1, y);
	  check(x, y + 1);
	  check(x + 1, y);
	  check(x, y - 1);

	  if (m == n) {
	    // It has grown to the right size.
	    // Check squares adjacent to polyomino. We need only check
	    // the unsearched growth sites and squares already searched because
	    // these are the only potential trouble spots.
	    for(int k = 0; k < inta_count(r); k++) {
	      int x = inta_at(r, k);
	      int y = inta_at(c, k);
	      if (scratch[x][y] != -1) {
		if (board[x][y] == n) {
		  // An n-polyomino cannot border a square clued with n.
		  inta_remove_all(blackr);
		  inta_remove_all(blackc);
		  goto abort;
		} else {
		  inta_append(blackr, x);
		  inta_append(blackc, y);
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

	    inta_append(psize, n);
	    void checkconflict(int w) {
	      // If the other polyomino covers our home square then don't
	      // bother reporting the conflict, because we handle this case
	      // elsewhere.
	      if (inta_index_of(must[i * ccount + j], w) >= 0) return;
	      if (inta_at(psize, w) == n) {
		inta_ptr a = (inta_ptr) malloc(sizeof(inta_t));
		inta_init(a);
		darray_append(adj, (void *) a);
		inta_append(a, w);
		inta_append(a, v);
		//printf("conflict %d %d\n", w, v);
	      }
	    }
	    for(int k = 0; k < inta_count(blackr); k++) {
	      int x = inta_at(blackr, k);
	      int y = inta_at(blackc, k);
	      inta_append(black[x][y], v);
	      inta_forall(white[x][y], checkconflict);
	    }
	    inta_remove_all(blackr);
	    inta_remove_all(blackc);
	    inta_append(white[i][j], v);
	    void addv(int k) {
	      int x = inta_at(r, k);
	      int y = inta_at(c, k);
	      inta_append(white[x][y], v);
	    }
	    inta_forall(growth, addv);
	    inta_append(must[i * ccount + j], v);
	    void addmust(int k) { inta_append(must[k], v); }
	    inta_forall(dupe, addmust);
	    v++;
	  } else {
	    // Recurse through each growing site.
	    for(int k = lower; k < inta_count(r); k++) {
	      int x = inta_at(r, k);
	      int y = inta_at(c, k);
	      if (scratch[x][y] != -3) {
		inta_append(growth, k);
		scratch[x][y] = -1;
		recurse(x, y, m + 1, k + 1);
		scratch[x][y] = -2;
		inta_remove_last(growth);
	      }
	    }
	  }
abort:
	  for(int k = old; k < inta_count(r); k++) {
	    scratch[inta_at(r, k)][inta_at(c, k)] = 0;
	  }
	  inta_set_count(r, old);
	  inta_set_count(c, old);
	  inta_set_count(dupe, olddupecount);
	}
	recurse(i, j, 1, 0);
	inta_remove_all(r);
	inta_remove_all(c);
      }
    }
  }
  zdd_set_vmax(v - 1);
  // Each clue n must be covered by exactly one n-polyomino.
  for (int i = 0; i < rcount * ccount; i++) {
    if (inta_count(must[i]) > 0) {
      zdd_contains_exactly_1(inta_raw(must[i]), inta_count(must[i]));
      zdd_intersection();
    }
  }

  // Othewise, each square can be covered by at most one of the polyominoes we
  // enumerated.
  for (int i = 0; i < rcount; i++) {
    int first = 1;
    for (int j = 0; j < ccount; j++) {
      if (board[i][j] != 0) continue;
      if (inta_count(white[i][j]) > 1) {
	zdd_contains_at_most_1(inta_raw(white[i][j]), inta_count(white[i][j]));
	if (first) first = 0; else zdd_intersection();
      }
    }
    zdd_intersection();
  }

  // Adjacent polyominoes must differ in size.
  void handleadj(void *data) {
    inta_ptr list = (inta_ptr) data;
    zdd_contains_at_most_1(inta_raw(list), inta_count(list));
    zdd_intersection();
  }
  darray_forall(adj, handleadj);

  int solcount = 0;
  void printsol(int *v, int count) {
    char pic[rcount][ccount];
    memset(pic, '.', rcount * ccount);
    for(int k = 0; k < count; k++) {
      for (int i = 0; i < rcount; i++) {
	for (int j = 0; j < ccount; j++) {
	  if (inta_index_of(white[i][j], v[k]) >= 0) {
	    int n = inta_at(psize, v[k]);
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
	if ('.' == pic[i][j]) {
	  int count = 0;
	  void paint(int i, int j) {
	    scratch[i][j] = -1;
	    inta_append(r, i);
	    inta_append(c, j);
	    count++;
	    void bleed(int i, int j) {
	      if (onboard(i, j) &&
	       	  pic[i][j] == '.' && !scratch[i][j]) paint(i, j);
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
	      inta_remove_all(r);
	      inta_remove_all(c);
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
	    int x = inta_at(r, 0);
	    int y = inta_at(c, 0);
	    neighbour_run(count1, x, y);
	    x = inta_at(r, 1);
	    y = inta_at(c, 1);
	    neighbour_run(count1, x, y);
	    if (n1) {
	      inta_remove_all(r);
	      inta_remove_all(c);
	      return;
	    }
	    x = inta_at(r, 0);
	    y = inta_at(c, 0);
	    pic[x][y] = '2';
	    x = inta_at(r, 1);
	    y = inta_at(c, 1);
	    pic[x][y] = '2';
	  }
	  inta_remove_all(r);
	  inta_remove_all(c);
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
