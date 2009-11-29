#include <stdint.h>
#include <gmp.h>

// Usage:
// 1. Call zdd_init() first.
// 2. Call zdd_set_vmax() with the number of variables (number of elements).
//    Following Knuth, the variables are 1-indexed.
// 3. Construct ZDDs by using functions such as zdd_contains_exactly_1, or
//    manually with the getters and setters. If done manually, call zdd_push()
//    between trees.
// 4. Call zdd_intersection() to take the last two trees off the stack and
//    replace them with their intersection.
// 5. Now it depends on the application. For a puzzle solver, there is
//    typically a unique solution which can be read by traversing the HI edges:
//      for(i = zdd_root; i != 1; i = zdd_hi(i)) {
//        printf("%d\n", zdd_v(i)); 
//      }
//    Or compute statistics on the family of sets with zdd_count() and friends.

void zdd_init();
void zdd_check();
uint16_t zdd_vmax();
uint16_t zdd_set_vmax(int i);
// Call before computing a new ZDD on the stack.
void zdd_push();
void zdd_pop();
// Print all nodes.
void zdd_dump();
// Getters and setters.
uint32_t zdd_v(uint32_t n);
uint32_t zdd_lo(uint32_t n);
uint32_t zdd_hi(uint32_t n);
uint32_t zdd_root();
uint32_t zdd_set_hi(uint32_t n, uint32_t hi);
uint32_t zdd_set_lo(uint32_t n, uint32_t lo);
uint32_t zdd_set_hilo(uint32_t n, uint32_t hilo);
uint32_t zdd_set_root(uint32_t root);
uint32_t zdd_add_node(uint32_t v, int offlo, int offhi);
uint32_t zdd_abs_node(uint32_t v, uint32_t lo, uint32_t hi);
uint32_t zdd_last_node();
uint32_t zdd_next_node();
// Count number of sets in ZDD.
void zdd_count(mpz_ptr);
// Count number of sets in ZDD, as well as sum of sizes of all sets.
// (The 0- and 1- power sums.)
void zdd_count_1(mpz_ptr z0, mpz_ptr z1);
// Count number of sets in ZDD, the sum of sizes of all sets, and the sum
// of their squares. (The 0-, 1- and 2- power sums.)
void zdd_count_2(mpz_ptr z0, mpz_ptr z1, mpz_ptr z2);
// Returns number of nodes.
uint32_t zdd_size();

// Need to have set vmax to call these:

// Constructs ZDD of all sets.
uint32_t zdd_powerset();

// Runs callback on every set in ZDD.
void zdd_forall(void (*fn)(int *, int));

// Runs callback on largest set in ZDD. If there are several candidates,
// runs on lexicographically smallest.
void zdd_forlargest(void (*fn)(int *, int));

// Construct ZDD of sets containing exactly 1 of the elements in the given list.
void zdd_contains_exactly_1(const int *a, int count);

// Construct ZDD of sets containing at most 1 of the elements in the given
// list.
void zdd_contains_at_most_1(const int *a, int count);

// Construct ZDD of sets containing at least 1 of the elements in the given
// list.
void zdd_contains_at_least_1(const int *a, int count);

// Construct ZDD of sets not containing any elements from the given list.
void zdd_contains_0(const int *a, int count);

// Construct ZDD of sets containing exactly n of the elements in the
// given list.
void zdd_contains_exactly_n(int n, const int *a, int count);

// Replace top two ZDDs on the stack with their intersection.
uint32_t zdd_intersection();
