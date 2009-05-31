// Test basic ZDD routines by comparing against results in Knuth's book.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "inta.h"
#include "zdd.h"
#include "io.h"

inta_t board[8][8];

// How many ways can you tile a chessboard with monominoes?
// This trivial case serves as a sanity check.
void test_monomino_tilings() {
  int v = 1;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      // Monominoes.
      inta_append(board[i][j], v++);
    }
  }

  zdd_set_vmax(v - 1);
  printf("variables: %d\n", zdd_vmax());
  EXPECT(64 == zdd_vmax());

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      zdd_contains_exactly_1(inta_raw(board[i][j]), inta_count(board[i][j]));
      zdd_intersection();
    }
  }

  printf("nodes: %d\n", zdd_size());
  EXPECT(zdd_size() == 8 * 8 + 2);
  mpz_t z, answer;
  mpz_init(z);
  mpz_init(answer);
  mpz_set_str(answer, "1", 0);

  zdd_count(z);
  gmp_printf("1-onimo tilings: %Zd\n", z);
  EXPECT(!mpz_cmp(z, answer));

  mpz_clear(z);
  mpz_clear(answer);
  zdd_pop();
  // Empty board.
  for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++) {
    inta_remove_all(board[i][j]);
  }
}

// How many ways can you tile a chessboard with dominoes?
// Using the obvious approach with ZDDs, we should end up with a 112-variable
// 2300-node ZDD describing 12988816 solutions.
void test_domino_tilings() {
  int v = 1;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      // Dominoes.
      if (j != 8 - 1) {
	inta_append(board[i][j], v);
	inta_append(board[i][j + 1], v++);
      }
      if (i != 8 - 1) {
	inta_append(board[i][j], v);
	inta_append(board[i + 1][j], v++);
      }
    }
  }

  zdd_set_vmax(v - 1);
  printf("variables: %d\n", zdd_vmax());
  EXPECT(112 == zdd_vmax());

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      zdd_contains_exactly_1(inta_raw(board[i][j]), inta_count(board[i][j]));
      zdd_intersection();
    }
  }

  printf("nodes: %d\n", zdd_size());
  EXPECT(zdd_size() == 2300);
  mpz_t z, answer;
  mpz_init(z);
  mpz_init(answer);
  mpz_set_str(answer, "12988816", 0);

  zdd_count(z);
  gmp_printf("2-omino tilings: %Zd\n", z);
  EXPECT(!mpz_cmp(z, answer));

  mpz_clear(z);
  mpz_clear(answer);
  zdd_pop();
  // Empty board.
  for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++) {
    inta_remove_all(board[i][j]);
  }
}

// How many ways can you tile a chessboard with 1-, 2- and 3-polyonominoes?
// We expect 468 variables, 512227 nodes and 92109458286284989468604 solutions.
void test_123_tilings() {
  int v = 1;
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
      void print_it(int i) {
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
    int n = i + 1;
    while (!(n & 1)) {
      n >>= 1;
      zdd_intersection();
    }
  }

  printf("nodes: %d\n", zdd_size());
  EXPECT(zdd_size() == 512227);
  mpz_t z, answer;
  mpz_init(z);
  mpz_init(answer);
  mpz_set_str(answer, "92109458286284989468604", 0);

  zdd_count(z);
  gmp_printf("1-, 2-, 3-omino tilings: %Zd\n", z);
  EXPECT(!mpz_cmp(z, answer));

  mpz_clear(z);
  mpz_clear(answer);
  zdd_pop();
  // Empty board.
  for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++) {
    inta_remove_all(board[i][j]);
  }
}

int main() {
  zdd_init();
  // Initialize board.
  for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++) {
    inta_init(board[i][j]);
  }

  test_monomino_tilings();
  test_domino_tilings();
  test_123_tilings();

  // Clear board.
  for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++) {
    inta_clear(board[i][j]);
  }
  return 0;
}
