// Solve a Slither Link puzzle
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
  int max = 4;
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

  /*
  for(int i = 1; i <= zdd_vmax(); i++) {
    printf("%d -> %d\n", au[i], av[i]);
  }
  for(int i = 0; i < max; i++) {
    for(int j = 0; j < max; j++) {
      printf(" %d", vtab[i][j]);
    }
    printf("\n");
  }
  printf("\n");
  */

  // Construct ZDD of all simple loops. See Knuth.
  
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
      return r;
    }
    return (uint32_t) memo_it_data(it);
  }

  // By arc e, we have already considered all arcs with sources less than
  // the current source, au[e], thus nothing we do from now on can affect their
  // state. Also, av[e] is as least as large as all previous targets we
  // have considered, so our choices so far cannot possibly influence targets
  // strictly larger than the current target.
  //
  // Thus rather than consider our choice's effects on every vertex, we can
  // restrict our focus to au[e], ..., av[e].
  // 
  // We associate an int with each vertex in our state. -1 means we've already
  // chosen two edges containing this vertex, so can choose no more. Otherwise
  // we store the vertex representing the other end: if we haven't yet picked
  // edges from this vertex, the vertex is its own other end.
  //
  // When picking an edge that closes a loop, we see if we have picked edges
  // outside the loop; if so, the set of edges is not a simple loop.
  memo_t cache[zdd_vmax() + 1];
  for(int i = 0; i <= zdd_vmax(); i++) memo_init(cache[i]);
  uint32_t recurse(int e, char *state, char start, int count) {
    char newstate[max + 1];
    int newcount = 0;
    memo_it it = NULL;
    uint32_t memoize(uint32_t n) {
      if (it) return (uint32_t) memo_it_put(it, (void *) n);
      return n;
    }
    // state == NULL is a special case that we use during the first call.
    if (!state) {
      // I really should be using au[] and av[].
      newstate[0] = 1;
      newstate[1] = 2;
      newcount = 2;
    } else {
      state[count] = 0;
      int just_created = memo_it_insert(&it, cache[e], (void *) state);
      if (!just_created) return (uint32_t) memo_it_data(it);
      // Examine part of state that cannot be affected by future choices,
      // including whether we include the current edge.
      // Return false node if it's impossible to continue, otherwise
      // remove them from the state and continue.
      int j = au[e] - start;
      if (j > count - 1) die("bad vertex or edge numbering");
      for (int i = 0; i < j; i++) {
	int otherend = state[i];
	if (otherend != -1 && otherend != start + i) {
	  // Vertex start + i is dangling, and we can no longer connect it to
	  // our loop.
	  return memoize(0);
	}
      }
      // Copy over the part of the state that is still relevant.
      while(j < count) {
	newstate[newcount++] = state[j++];
      }
      // Add vertices that now matter to our state.
      j += start;
      while(j <= av[e]) {
	newstate[newcount++] = j++;
      }
      // By now newcount == av[e] - au[e].
    }

    if (e == zdd_vmax()) {
      // We've come to the last edge, so our choices so far must have either...
      if (newstate[0] == au[e]) {
	// ... produced a complete loop already:
	// We want !V ? TRUE : FALSE. In a ZDD, this gets compressed to
	// simply TRUE.
	return memoize(1);
      } else {
	// ... or we need the last edge to finish the loop:
	// We want !V ? FALSE : TRUE. (An elementary family.)
	return memoize(unique(e, 0, 1));
      }
    }

    // Recurse the case when we don't pick the current edge.
    uint32_t lo = recurse(e + 1, newstate, au[e], newcount);

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
    } else if (u == av[e]) {
      // We have a closed a loop. We're good as long as nothing is dangling.
      for (int i = 1; i < newcount - 1; i++) {
	// Dangling link starting at i + au[e].
	if (newstate[i] != i + au[e]) {
	  hi = 0;
	  break;
	}
      }
    } else {
      // Recurse the case when we do pick the current edge. Modify the
      // state accordingly for the call.
      newstate[0] = -1;
      newstate[newcount - 1] = -1;
      newstate[v - au[e]] = u;
      newstate[u - au[e]] = v;
      hi = recurse(e + 1, newstate, au[e], newcount);
    }
    // Compress HI -> FALSE nodes.
    if (!hi) return memoize(lo);
    return memoize(unique(e, lo, hi));
  }
  zdd_push();
  zdd_set_root(recurse(1, NULL, 0, 0));
  for(int i = 0; i <= zdd_vmax(); i++) memo_clear(cache[i]);
  for(uint16_t v = 1; v <= zdd_vmax(); v++) memo_clear(node_tab[v]);
  zdd_dump();
  mpz_t z;
  mpz_init(z);
  zdd_count(z);
  gmp_printf("%Zd\n", z);
  mpz_clear(z);
  zdd_check();

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

    for(int i = 0; i < vcount; i++) {
      int e = v[i];
      int r = rtab[au[e]] + rtab[av[e]];
      int c = ctab[au[e]] + ctab[av[e]];
      pic[r][c] = r & 1 ? '|' : '-';
    }

    for(int i = 0; i < 2 * max; i++) puts(pic[i]);
    putchar('\n');
  }
  zdd_forall(printsol);

  return 0;
}
