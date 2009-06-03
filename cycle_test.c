// Confirm that an 2x2 grid graph has 2 simple cycles.
// Confirm that an 3x3 grid graph has 14 simple cycles (easily checked by hand).
// Confirm that an 8x8 grid graph has 603841648932 simple cycles (see Knuth).
// Other results, not verified:
//   4x4: 214 
//   5x5: 9350 
//   6x6: 1222364
//   7x7: 487150372
//   9x9: 2318527339461266
// 10x10: 27359264067916806102
// 11x11: 988808811046283595068100
// 12x12: 109331355810135629946698361372109331355810135629946698361372
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memo.h"
#include "zdd.h"
#include <stdarg.h>
#include "io.h"

void compute_grid_graph(int max, int want_print) {
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
      if (!(r << 15)) printf("node #%x\n", r);
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
  //
  // We keep our state in a char *, so this cannot work on 256x256 grids.
  // Our crit-bit trie uses NULL-terminated strings as keys, so 0 cannot appear
  // in our state. Thus, in the state:
  //   -1 means we've already picked two edges involving this vertex
  //    n means the other end is n + au[e] - 1
  memo_t cache[zdd_vmax() + 1];
  for(int i = 0; i <= zdd_vmax(); i++) memo_init(cache[i]);
  uint32_t recurse(int e, char *state, int start, int count) {
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
      int just_created = memo_it_insert(&it, cache[e], state);
      if (!just_created) return (uint32_t) memo_it_data(it);
      // Examine part of state that cannot be affected by future choices,
      // including whether we include the current edge.
      // Return false node if it's impossible to continue, otherwise
      // remove them from the state and continue.
      int j = au[e] - start;
      if (j > count - 1) die("bad vertex or edge numbering");
      for (int i = 0; i < j; i++) {
	int otherend = state[i];
	if (otherend != -1 && otherend - 1 != i) {
	  // Vertex start - 1 + i is dangling, and we can no longer connect it
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
	return memoize(1);
      } else {
	// ...or must need the last edge to finish the
	// loop; we want !V ? FALSE : TRUE (an elementary family)
	return memoize(unique(e, 0, 1));
      }
    }

    // Recurse the case where we don't pick the current edge.
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
    } else if (u + au[e] - 1 == av[e]) {
      // We have a closed a loop. We're good as long as nothing is dangling.
      for (int i = 1; i < newcount - 1; i++) {
	if (newstate[i] != -1 && newstate[i] != i + 1) {
	  // Dangling link starting at i + au[e] - 1.
	  hi = 0;
	  break;
	}
      }
    } else {
      // Recurse the case when we do pick the current edge. Modify the
      // state accordingly for the call.
      newstate[0] = -1;
      newstate[newcount - 1] = -1;
      newstate[v - 1] = u;
      newstate[u - 1] = v;
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
    putchar('\n');
  }
  if (want_print) zdd_forall(printsol);
}

int main() {
  zdd_init();

  mpz_t z;
  mpz_init(z);
  printf(" n, simple cycles in nxn grid graph\n");
  for(int n = 2; n <= 12; n++) {
    compute_grid_graph(n, n <= 3);
    zdd_count(z);
    gmp_printf("%2d, %Zd\n", n, z);
    switch(n) {
      case 3:
	EXPECT(!mpz_cmp_ui(z, 14));
	break;
      case 8:
      {
	mpz_t answer;
	mpz_init(answer);
	mpz_set_str(answer, "603841648932", 0);
	EXPECT(!mpz_cmp(z, answer));
	mpz_clear(answer);
	break;
      }
    }
    zdd_pop();
    fflush(stdout);
  }
  mpz_clear(z);
}
