// How many ways can you tile a chessboard with 1-, 2- and 3-polyonominos?
// Using the obvious approach with ZDDs, we should end up with a 468-variable
// 512227-node ZDD describing 92109458286284989468604 solutions.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "darray.h"
#include "zdd.h"
#include "io.h"

uint32_t vmax;

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

int main() {
  zdd_init();
  int v = 1;
  darray_t board[8][8];
  // Initialize board.
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      darray_init(board[i][j]);
    }
  }
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      // Monominoes.
      darray_append(board[i][j], (void *) v++);
      // Dominoes.
      if (j != 8 - 1) {
	darray_append(board[i][j], (void *) v);
	darray_append(board[i][j + 1], (void *) v++);
      }
      if (i != 8 - 1) {
	darray_append(board[i][j], (void *) v);
	darray_append(board[i + 1][j], (void *) v++);
      }
      // Trionimoes.
      // 3x1 trionimoes.
      if (i < 8 - 2) {
	darray_append(board[i][j], (void *) v);
	darray_append(board[i + 1][j], (void *) v);
	darray_append(board[i + 2][j], (void *) v++);
      }
      if (j < 8 - 2) {
	darray_append(board[i][j], (void *) v);
	darray_append(board[i][j + 1], (void *) v);
	darray_append(board[i][j + 2], (void *) v++);
      }
      // 2x2 - 1 tile trionimoes.
      if (i != 8 - 1 && j != 8 - 1) {
	darray_append(board[i][j], (void *) v);
	darray_append(board[i + 1][j], (void *) v);
	darray_append(board[i][j + 1], (void *) v++);

	darray_append(board[i][j], (void *) v);
	darray_append(board[i + 1][j], (void *) v);
	darray_append(board[i + 1][j + 1], (void *) v++);

	darray_append(board[i][j], (void *) v);
	darray_append(board[i][j + 1], (void *) v);
	darray_append(board[i + 1][j + 1], (void *) v++);

	darray_append(board[i + 1][j], (void *) v);
	darray_append(board[i][j + 1], (void *) v);
	darray_append(board[i + 1][j + 1], (void *) v++);
      }
    }
  }

  vmax = v - 1;
  printf("variables: %d\n", vmax);
  EXPECT(468 == vmax);

  /*
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      void print_it(void *data) {
	int i = (int) data;
	printf(" %d", i);
      }
      darray_forall(board[i][j], print_it);
      printf("\t\t");
    }
    printf("\n");
  }
  */
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      contains_exactly_one(board[i][j]);
      if (j) zdd_intersection();
    }
  }
  for (int i = 6; i >= 0; i--) {
    printf("%d\n", i), fflush(stdout);
    zdd_intersection();
  }

  printf("nodes: %d\n", zdd_size());
  EXPECT(zdd_size() == 512227);
  mpz_t z, answer;
  mpz_init(z);
  mpz_init(answer);
  mpz_set_str(answer, "92109458286284989468604", 0);

  zdd_count(z);
  gmp_printf("%Zd\n", z);
  EXPECT(!mpz_cmp(z, answer));

  mpz_clear(z);
  mpz_clear(answer);
  return 0;
}
