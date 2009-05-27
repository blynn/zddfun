#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "darray.h"

#define NDEBUG
#include <assert.h>

enum {
    max_init = 8
};

void darray_init(darray_ptr a)
{
    a->max = max_init;
    a->count = 0;
    a->item = malloc(sizeof(void *) * a->max);
}

darray_ptr darray_new(void)
{
    darray_ptr res = malloc(sizeof(darray_t));
    darray_init(res);
    return res;
}

void darray_remove_all(darray_ptr a)
{
    a->max = max_init;
    a->count = 0;
    free(a->item);
    a->item = malloc(sizeof(void *) * a->max);
}

void* darray_remove_last(darray_ptr a)
{
    assert(a->count > 0);
    a->count--;
    return(a->item[a->count]);
}

void darray_realloc(darray_ptr a, int size)
{
    a->max = size;
    a->item = realloc(a->item, sizeof(void *) * a->max);
}

void darray_append(darray_ptr a, void *p)
{
    if (a->count == a->max) {
	if (!a->max) a->max = max_init;
	else a->max *= 2;
	a->item = realloc(a->item, sizeof(void *) * a->max);
    }
    a->item[a->count] = p;
    a->count++;
}

int darray_index_of(darray_ptr a, void *p)
{
    int i;
    for (i=0; i<a->count; i++) {
	if (a->item[i] == p) return i;
    }
    return -1;
}

void darray_clear(darray_t a)
{
    free(a->item);
    a->max = 0;
    a->count = 0;
}

void darray_show(darray_ptr a)
{
    int i;
    for (i=0;i<a->count;i++) {
	printf("%d: %p\n", i, a->item[i]);
    }
}

void darray_remove_index(darray_ptr a, int n)
{
    assert(a->count >= n-1);
    a->count--;
    memmove(a->item + n, a->item + n + 1, sizeof(void *) * (a->count - n));
}

void darray_remove(darray_ptr a, void *p)
{
    int i;
    for (i=0; i<a->count; i++) {
	if (a->item[i] == p) {
	    darray_remove_index(a, i);
	    return;
	}
    }
    assert(0);
}

int darray_index_of_test(darray_ptr a, int (*test)(void *))
{
    int i;
    for (i = 0; i < a->count; i++) if (test(a->item[i])) return i;
    return -1;
}

void *darray_at_test(darray_ptr a, int (*test)(void *))
{
    int i;
    for (i = 0; i < a->count; i++) {
	void *p = a->item[i];
	if (test(p)) return p;
    }
    return NULL;
}

void darray_remove_test(darray_ptr a, int (*test)(void *))
{
    int i;
    while ((i = darray_index_of_test(a, test) >= 0)) {
	darray_remove_index(a, i);
    }
}

void darray_swap(darray_ptr a, int i, int j)
{
    void *tmp = a->item[j];
    a->item[j] = a->item[i];
    a->item[i] = tmp;
}

void darray_copy(darray_ptr dst, darray_ptr src)
{
    darray_realloc(dst, src->count);
    memcpy(dst->item, src->item, src->count * sizeof(void *));
    dst->count = src->count;
}

void darray_forall(darray_t a, void (*func)(void *))
{
  void **item = a->item;
  void **const end = item + a->count;
  while (item != end) {
    func(*item);
    item++;
  }
}

void darray_qsort(darray_t a, int (*compar)(const void *, const void *)) {
    qsort(a->item, a->count, sizeof(void *), compar);
}
