// Solve a Slither Link puzzle
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inta.h"
#include "zdd.h"
#include <stdarg.h>
#include "io.h"

int main() {
  zdd_init();
  int max = 5;
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
  return 0;
} 
