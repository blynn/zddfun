// How many ways can you tile a chessboard with 1-, 2- and 3-polyonominos?
// Using the obvious approach with ZDDs, we should end up with a 468-variable
// 512227-node ZDD describing 92109458286284989468604 solutions.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "inta.h"
#include "zdd.h"
#include "io.h"

int main() {
  zdd_init();
  int v = 1;
  inta_t board[8][8];
  // Initialize board.
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      inta_init(board[i][j]);
    }
  }
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      // Monominoes.
      inta_append(board[i][j], v++);
      // Dominoes.
      if (j != 8 - 1) {
	inta_append(board[i][j], v);
	inta_append(board[i][j + 1], v++);
      }
      if (i != 8 - 1) {
	inta_append(board[i][j], v);
	inta_append(board[i + 1][j], v++);
      }
      // Trionimoes.
      // 3x1 trionimoes.
      if (i < 8 - 2) {
	inta_append(board[i][j], v);
	inta_append(board[i + 1][j], v);
	inta_append(board[i + 2][j], v++);
      }
      if (j < 8 - 2) {
	inta_append(board[i][j], v);
	inta_append(board[i][j + 1], v);
	inta_append(board[i][j + 2], v++);
      }
      // 2x2 - 1 tile trionimoes.
      if (i != 8 - 1 && j != 8 - 1) {
	inta_append(board[i][j], v);
	inta_append(board[i + 1][j], v);
	inta_append(board[i][j + 1], v++);

	inta_append(board[i][j], v);
	inta_append(board[i + 1][j], v);
	inta_append(board[i + 1][j + 1], v++);

	inta_append(board[i][j], v);
	inta_append(board[i][j + 1], v);
	inta_append(board[i + 1][j + 1], v++);

	inta_append(board[i + 1][j], v);
	inta_append(board[i][j + 1], v);
	inta_append(board[i + 1][j + 1], v++);
      }
    }
  }

  zdd_set_vmax(v - 1);
  printf("variables: %d\n", zdd_vmax());
  EXPECT(468 == zdd_vmax());

  /*
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      void print_it(int data) {
	int i = (int) data;
	printf(" %d", i);
      }
      inta_forall(board[i][j], print_it);
      printf("\t\t");
    }
    printf("\n");
  }
  */
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      zdd_contains_exactly_1(inta_raw(board[i][j]), inta_count(board[i][j]));
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
