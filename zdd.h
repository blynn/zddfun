/* Requires:
#include <stdint.h>
*/
void zdd_init();
void zdd_check();
void zdd_push();
void zdd_pop();
uint32_t zdd_v(uint32_t n);
uint32_t zdd_lo(uint32_t n);
uint32_t zdd_hi(uint32_t n);
uint32_t zdd_root();
uint32_t zdd_set_hi(uint32_t n, uint16_t hi);
uint32_t zdd_set_lo(uint32_t n, uint16_t lo);
uint32_t zdd_set_hilo(uint32_t n, uint16_t hilo);
uint32_t zdd_set_root(uint16_t root);
uint32_t zdd_add_node(uint32_t v, int offlo, int offhi);
uint32_t zdd_last_node();
uint32_t zdd_next_node();
void get_count(uint32_t n);
uint32_t zdd_intersection();
