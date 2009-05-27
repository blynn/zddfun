// A linked-list implementation using C arrays
// darray = "dynamic array"

#define __DARRAY_H__

struct darray_s {
    void **item;
    int count;
    int max;
};

typedef struct darray_s darray_t[1];
typedef struct darray_s *darray_ptr;

/*@manual darray
Initialize a dynamic array ''a''. Must be called before ''a'' is used.
*/
void darray_init(darray_t a);
darray_ptr darray_new(void);

/*@manual darray
Clears a dynamic array ''a''. Should be called after ''a'' is no longer needed.
*/
void darray_clear(darray_t a);

/*@manual darray
Appends ''p'' to the dynamic array ''a''.
*/
void darray_append(darray_t a, void *p);

/*@manual darray
Returns the pointer at index ''i'' in the dynamic array ''a''.
*/
static inline void *darray_at(darray_t a, int i)
{
    return a->item[i];
}

void *darray_at_test(darray_ptr a, int (*test)(void *));

int darray_index_of(darray_ptr a, void *p);
int darray_index_of_test(darray_ptr a, int (*test)(void *));
void darray_remove(darray_ptr a, void *p);
void* darray_remove_last(darray_ptr a);
void darray_remove_test(darray_ptr a, int (*test)(void *));
void darray_swap(darray_ptr a, int i, int j);

/*@manual darray
Removes the pointer at index ''i'' in the dynamic array ''a''.
*/
void darray_remove_index(darray_ptr a, int n);
void darray_copy(darray_ptr dst, darray_ptr src);
void darray_remove_all(darray_ptr d);
void darray_forall(darray_t a, void (*func)(void *));

/*@manual darray
Returns the number of pointers held in ''a''.
*/
static inline int darray_count(darray_ptr a)
{
    return a->count;
}

static inline int darray_is_empty(darray_ptr a)
{
    return !a->count;
}

static inline void *darray_last(darray_t a)
{
    return a->item[a->count - 1];
}

void darray_qsort(darray_t a, int (*compar)(const void *, const void *));
