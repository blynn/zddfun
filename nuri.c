// Solve a Nurikabe puzzle.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>
#include "darray.h"
#include "io.h"
#include "zdd.h"

int main() {
  zdd_init();
  int rcount, ccount;

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
  zdd_set_vmax(rcount * ccount);

  // Label squares according to this scheme:
  //   1 2 4 ...
  //   3 5 ...
  //   6 ...
  int vtab[rcount][ccount];
  int rtab[zdd_vmax() + 1], ctab[zdd_vmax() + 1];
  int v = 1;
  int i = 0, j = 0;
  for(;;) {
    rtab[v] = i;
    ctab[v] = j;
    vtab[i][j] = v++;
    if (v > zdd_vmax()) break;
    // If we're on the left or bottom edge:
    if (i == rcount - 1 || !j) {
      // Try advancing to the top of the column.
      j = i + j + 1;
      i = 0;
      if (j >= ccount) {
	// If it's out of bounds, move diagonally bottom-left.
	i = j - (ccount - 1);
	j = ccount - 1;
      }
    } else {
      i++, j--;
    }
  }

  // The state represents which squares are connected.
  // state[i] = -2: i is not selected.
  // state[i] = -1: i is unconnected to anything else in the state.
  // state[i] = i - j: i is connected to j and j is the largest square less
  // than i with this property.
  void recurse(int v, char* state, int start, int size) {
    int r = rtab[v];
    int c = ctab[v];

    int newsize = size;
    if (r - 1 < 0 || c - 1 < 0) newsize++;
    char newstate[newsize];
    for (int i = 0; i < size; i++) newstate[i] = state[i];
    // Omit v:
    newstate[v] = -1;
    // Include v:
    //newstate[v] =
  }
  recurse(1, NULL, 0, 0);

  return 0;
}
