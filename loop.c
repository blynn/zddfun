// Solve a Slither Link puzzle.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memo.h"
#include "zdd.h"
#include <stdarg.h>
#include "io.h"

int main() {
  zdd_init();

  int max;
  if (!scanf("%d\n", &max)) die("input error");
  int board[max][max];
  for(int i = 0; i < max; i++) {
    int c;
    for(int j = 0; j < max; j++) {
      c = getchar();
      if (c == EOF || c == '\n') die("input error");
      switch(c) {
	case '.':
	  board[i][j] = -1;
	  break;
	case '0' ... '4':
	  board[i][j] = c - '0';
	  break;
	default:
	  die("input error");
      }
    }
    c = getchar();
    if (c != '\n') die("input error");
  }

  // The size of a puzzle is the squares, but we really care about the edges
  // between their corners. If there are n^2 squares, then there are (n + 1)^2
  // corners.
  max++;

  // Label nodes and edges of grid graph. For example, when max = 3 we have
  //   1 2 4
  //   3 5 7
  //   6 8 9
  int vtab[max][max];
  int rtab[max * max + 1], ctab[max * max + 1];
  int v = 1;
  int i = 0, j = 0;
  for(;;) {
    rtab[v] = i;
    ctab[v] = j;
    vtab[i][j] = v++;
    if (i == max - 1) {
      if (j == max - 1) break;
      i = j + 1;
      j = max - 1;
    } else if (!j) {
      j = i + 1;
      i = 0;
    } else {
      i++, j--;
    }
  }

  zdd_set_vmax(max * (max - 1) * 2);
  // Arcs go from u to v.
  int au[zdd_vmax() + 1], av[zdd_vmax() + 1];
  i = 1;
  for(v = 1; v <= max * max; v++) {
    if (ctab[v] != max - 1) {
      au[i] = v;
      av[i] = vtab[rtab[v]][ctab[v] + 1];
      i++;
    }
    if (rtab[v] != max - 1) {
      au[i] = v;
      av[i] = vtab[rtab[v] + 1][ctab[v]];
      i++;
    }
  }

  // Add in clues.
  int done = 0, todo = -1;
  for(int i = 0; i < max - 1; i++) for(int j = 0; j < max - 1; j++) {
    int n = board[i][j];
    if (n != -1) {
      int a[4];
      int e = 1;
      // Top left corner should have two outedges: one right, one down.
      while(au[e] != vtab[i][j]) e++;
      a[0] = e;
      a[1] = e + 1;
      EXPECT(au[e + 1] == vtab[i][j]);
      // One more from the top right corner going down.
      while(au[e] != vtab[i][j + 1]) e++;
      if (av[e] != vtab[i + 1][j + 1]) e++;
      a[2] = e;
      // One more from the bottom left corner going right.
      while(au[e] != vtab[i + 1][j]) e++;
      a[3] = e;
      zdd_contains_exactly_n(n, a, 4);

      todo++;
      int n = todo + 1;
      while (!(n & 1)) {
	n >>= 1;
	zdd_intersection();
	done++;
      }
    }
  }
  while (done < todo) {
    zdd_intersection();
    done++;
  }

  uint32_t p = zdd_root();
  uint32_t clue_size = zdd_last_node();
  // Construct ZDD of all simple loops constrained by the clues.
  memo_t node_tab[zdd_vmax() + 1];
  for(uint16_t v = 1; v <= zdd_vmax(); v++) memo_init(node_tab[v]);

  uint32_t unique(uint16_t v, uint32_t lo, uint32_t hi) {
    // Create or return existing node representing !v ? lo : hi.
    uint32_t key[2] = { lo, hi };
    memo_it it;
    int just_created = memo_it_insert_u(&it, node_tab[v], (void *) key, 8);
    if (just_created) {
      uint32_t r;
      memo_it_put(it, (void *) (r = zdd_abs_node(v, lo, hi)));
      if (!(r << 15)) printf("node #%x\n", r);
      return r;
    }
    return (uint32_t) memo_it_data(it);
  }

  // Similar to the routine in cycle_test.c, but at the same time we respect
  // the clues. The node p in the clue ZDD therefore is part of the state.
  memo_t cache[clue_size + 1];
  for(int i = 0; i <= clue_size; i++) memo_init(cache[i]);
  uint32_t recurse(uint32_t p, char *state, int start, int count) {
    if (p <= 1) return p;
    int e = zdd_v(p);
    char newstate[max + 1];
    int newcount = 0;
    memo_it it = NULL;
    uint32_t memoize(uint32_t n) {
      if (it) return (uint32_t) memo_it_put(it, (void *) n);
      return n;
    }
    // state == NULL is a special case that we use during the first call.
    if (!state) {
      for(int j = au[e]; j <= av[e]; j++) {
	newstate[j - au[e]] = j - au[e] + 1;
      }
      newcount = av[e] - au[e] + 1;
    } else {
      // The crit-bit tree uses NULL to terminate keys, so we must
      // do a sort of variable-length encoding.
      int hack = start, hacki = 0;
      state[count] = hack;
      while(hack & ~0x7f) {
	state[count + hacki] |= 0x80;
	hack >>= 7;
	hacki++;
	state[count + hacki] = hack;
      }
      state[count + ++hacki] = 0;
      int just_created = memo_it_insert(&it, cache[p], state);
      if (!just_created && p <= 29) return (uint32_t) memo_it_data(it);
      // Examine part of state that cannot be affected by future choices,
      // including whether we include the current edge.
      // Return false node if it's impossible to continue, otherwise
      // remove them from the state and continue.
      int j = au[e] - start;
      if (j > count - 1) {
	// Future choices cannot change any dangling edges.
	for (int i = 0; i < j; i++) {
	  int otherend = state[i];
	  if (otherend - 1 != i) {
	    // Vertex start + i is dangling, and we can no longer connect it
	    // to our loop.
	    return memoize(0);
	  }
	}
	return memoize(1);
      }
      for (int i = 0; i < j; i++) {
	int otherend = state[i];
	if (otherend != -1 && otherend - 1 != i) {
	  // Vertex start + i is dangling, and we can no longer connect it
	  // to our loop.
	  return memoize(0);
	}
      }
      // Copy over the part of the state that is still relevant.
      while(j < count) {
	int n = state[j];
	newstate[newcount++] = n < 0 ? -1 : n + start - au[e];
	j++;
      }
      // Add vertices that now matter to our state.
      j += start;
      while(j <= av[e]) {
	newstate[newcount++] = j++ - au[e] + 1;
      }
      // By now newcount == av[e] - au[e].
    }

    // If we've come to the last edge...
    if (e == zdd_vmax()) {
      if (newstate[0] == 1) {
	// ...we have the empty graph:
	if (zdd_lo(p)) return memoize(1);
      } else {
	// ...or must need the last edge to finish the
	// loop: we want !V ? FALSE : TRUE. (An elementary family.)
	return memoize(unique(e, 0, 1));
      }
    }

    // First, the case where we don't pick the current edge.
    // If the clues force us to leave every other element out then we cannot
    // complete a loop (since we have not completed a loop yet). Similarly
    // we must use the current edge, we cannot complete a loop. Otherwise
    // recurse.
    uint32_t lo = 1 >= zdd_lo(p) ? 0 :
        recurse(zdd_lo(p), newstate, au[e], newcount);

    // Before we recurse the other case, we must check a couple of things.
    // Let's initially assume we are done if we pick the current edge!
    uint32_t hi = 1;
    // Examine the other ends of au[e] and av[e], conveniently located at
    // the ends of newstate[].
    int u = newstate[0];
    int v = newstate[newcount - 1];
    if (u == -1 || v == -1) {
      // At least one of the endpoints of the current edge is already busy.
      hi = 0;
    } else if (u + au[e] - 1 == av[e]) {
      // We have a closed a loop. We're good as long as nothing is dangling...
      for (int i = 1; i < newcount - 1; i++) {
	if (newstate[i] != -1 && newstate[i] != i + 1) {
	  // Dangling link starting at i + au[e] - 1.
	  hi = 0;
	  break;
	}
      }
      // ...and the clues allow us to pick no higher edges.
      if (1 == hi) {
	uint32_t q = zdd_hi(p);
	while(q > 1) q = zdd_lo(q);
	hi = q;
      }
    } else if (1 == zdd_hi(p)) {
      // The clues allow no future edges, so we cannot complete our loop.
      hi = 0;
    } else {
      // Recurse the case when we do pick the current edge. Modify the
      // state accordingly for the call.
      newstate[0] = -1;
      newstate[newcount - 1] = -1;
      newstate[v - 1] = u;
      newstate[u - 1] = v;
      hi = recurse(zdd_hi(p), newstate, au[e], newcount);
    }
    // Compress HI -> FALSE nodes.
    if (!hi) return memoize(lo);
    return memoize(unique(e, lo, hi));
  }
  zdd_push();
  zdd_set_root(recurse(p, NULL, 0, 0));
  for(int i = 0; i <= clue_size; i++) memo_clear(cache[i]);
  for(uint16_t v = 1; v <= zdd_vmax(); v++) memo_clear(node_tab[v]);
  zdd_intersection();

  void printsol(int *v, int vcount) {
    char pic[2 * max][2 * max];
    for(int i = 0; i < max; i++) {
      for(int j = 0; j < max; j++) {
	pic[2 * i][2 * j] = '.';
	pic[2 * i][2 * j + 1] = ' ';
	pic[2 * i + 1][2 * j] = ' ';
	pic[2 * i + 1][2 * j + 1] = ' ';
      }
      pic[2 * i][2 * max - 1] = '\0';
      pic[2 * i + 1][2 * max - 1] = '\0';
    }
    for(int i = 0; i < max - 1; i++) {
      for(int j = 0; j < max - 1; j++) {
	if (board[i][j] != -1) {
	  pic[2 * i + 1][2 * j + 1] = '0' + board[i][j];
	}
      }
    }

    for(int i = 0; i < vcount; i++) {
      int e = v[i];
      int r = rtab[au[e]] + rtab[av[e]];
      int c = ctab[au[e]] + ctab[av[e]];
      pic[r][c] = r & 1 ? '|' : '-';
    }

    /* Plain ASCII output:
    for(int i = 0; i < 2 * max; i++) puts(pic[i]);
    */

    for(int i = 0; i < 2 * max - 1; i++) {
      for(int j = 0; j < 2 * max; j++) {
	int analyse() {
	  int n = 0;
	  int filled(int x, int y) {
	    if (x >= 0 && x < 2 * max - 1 && y >= 0 && y < 2 * max - 1 &&
	        pic[x][y] != ' ') return 1;
	    return 0;
	  }
	  if (filled(i - 1, j)) n += 1;
	  if (filled(i, j - 1)) n += 2;
	  if (filled(i + 1, j)) n += 4;
	  if (filled(i, j + 1)) n += 8;
	  return n;
	}
	switch(pic[i][j]) {
	  case '|':
	    printf("\u2502");
	    break;
	  case '-':
	    printf("\u2500");
	    break;
	  case '.':
	    switch(analyse()) {
	      case 0:
		printf("\u00b7");
		break;
	      case 3:
		printf("\u2518");
		break;
	      case 5:
		printf("\u2502");
		break;
	      case 6:
		printf("\u2510");
		break;
	      case 9:
		printf("\u2514");
		break;
	      case 10:
		printf("\u2500");
		break;
	      case 12:
		printf("\u250c");
		break;
	      default:
		printf("*");
		break;
	    }
	    break;
	  case '\0':
	    putchar('\n');
	    break;
	  default:
	    putchar(pic[i][j]);
	    break;
	}
      }
    }
    putchar('\n');
  }
  zdd_forall(printsol);
}
