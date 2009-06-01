#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inta.h"

#define NDEBUG
#include <assert.h>

enum {
    max_init = 8
};

void inta_init(inta_ptr a)
{
    a->max = max_init;
    a->count = 0;
    a->item = malloc(sizeof(int) * a->max);
}

void inta_init_n(inta_ptr a, int n)
{
    a->max = n;
    a->count = 0;
    a->item = malloc(sizeof(int) * a->max);
}

inta_ptr inta_new()
{
    inta_ptr res = malloc(sizeof(inta_t));
    inta_init(res);
    return res;
}

void inta_remove_all(inta_ptr a)
{
    a->max = max_init;
    a->count = 0;
    free(a->item);
    a->item = malloc(sizeof(int) * a->max);
}

int inta_remove_last(inta_ptr a)
{
    assert(a->count > 0);
    a->count--;
    return(a->item[a->count]);
}

void inta_realloc(inta_ptr a, int size)
{
    a->max = size;
    a->item = realloc(a->item, sizeof(int) * a->max);
}

void inta_append(inta_ptr a, int p)
{
    if (a->count == a->max) {
	if (!a->max) a->max = max_init;
	else a->max *= 2;
	a->item = realloc(a->item, sizeof(int) * a->max);
    }
    a->item[a->count] = p;
    a->count++;
}

int inta_index_of(inta_ptr a, int p)
{
    int i;
    for (i=0; i<a->count; i++) {
	if (a->item[i] == p) return i;
    }
    return -1;
}

void inta_clear(inta_t a)
{
    free(a->item);
    a->max = 0;
    a->count = 0;
}

void inta_remove_index(inta_ptr a, int n)
{
    assert(a->count >= n-1);
    a->count--;
    memmove(a->item + n, a->item + n + 1, sizeof(int) * (a->count - n));
}

void inta_remove(inta_ptr a, int p)
{
    int i;
    for (i=0; i<a->count; i++) {
	if (a->item[i] == p) {
	    inta_remove_index(a, i);
	    return;
	}
    }
    assert(0);
}

int inta_index_of_test(inta_ptr a, int (*test)(int))
{
    int i;
    for (i = 0; i < a->count; i++) if (test(a->item[i])) return i;
    return -1;
}

int inta_at_test(inta_ptr a, int (*test)(int))
{
    int i;
    for (i = 0; i < a->count; i++) {
	int p = a->item[i];
	if (test(p)) return p;
    }
    return 0;
}

void inta_remove_test(inta_ptr a, int (*test)(int))
{
    int i;
    while ((i = inta_index_of_test(a, test) >= 0)) {
	inta_remove_index(a, i);
    }
}

void inta_swap(inta_ptr a, int i, int j)
{
    int tmp = a->item[j];
    a->item[j] = a->item[i];
    a->item[i] = tmp;
}

void inta_copy(inta_ptr dst, inta_ptr src)
{
    inta_realloc(dst, src->count);
    memcpy(dst->item, src->item, src->count * sizeof(int));
    dst->count = src->count;
}

void inta_forall(inta_t a, void (*func)(int))
{
  int *item = a->item;
  const int *end = item + a->count;
  while (item != end) {
    func(*item);
    item++;
  }
}

void inta_qsort(inta_t a, int (*compar)(const void *, const void *)) {
    qsort(a->item, a->count, sizeof(void *), compar);
}
