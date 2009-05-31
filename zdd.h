#include <stdint.h>
#include <gmp.h>

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
void zdd_count(mpz_ptr);
// Replace top two ZDDs on the stack with their intersection.
uint32_t zdd_intersection();

// Returns number of nodes.
uint32_t zdd_size();

// Need to have set vmax to call these:

// Constructs ZDD of all sets.
uint32_t zdd_powerset();

// Runs callback on every set in ZDD.
void zdd_forall(void (*fn)(int *, int));

// Construct ZDD of sets containing exactly 1 of the elements in the given list.
void zdd_contains_exactly_1(int *a, int count);

// Construct ZDD of sets containing at most 1 of the elements in the given
// list.
void zdd_contains_at_most_1(int *a, int count);
