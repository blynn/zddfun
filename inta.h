// A linked-list of ints using C arrays
// inta = "int array"

struct inta_s {
  int *item;
  int count;
  int max;
};

typedef struct inta_s inta_t[1];
typedef struct inta_s *inta_ptr;

/*@manual inta
Initialize a dynamic array ''a''. Must be called before ''a'' is used.
*/
void inta_init(inta_t a);
inta_ptr inta_new();

/*@manual inta
Clears a dynamic array ''a''. Should be called after ''a'' is no longer needed.
*/
void inta_clear(inta_t a);

/*@manual inta
Appends ''p'' to the dynamic array ''a''.
*/
void inta_append(inta_t a, int p);

/*@manual inta
Returns the pointer at index ''i'' in the dynamic array ''a''.
*/
static inline int inta_at(inta_t a, int i)
{
  return a->item[i];
}

static inline int inta_put(inta_t a, int n, int i) {
  return a->item[i] = n;
}

int inta_at_test(inta_ptr a, int (*test)(int));
int inta_index_of(inta_ptr a, int p);
int inta_index_of_test(inta_ptr a, int (*test)(int));
void inta_remove(inta_ptr a, int p);
int inta_remove_last(inta_ptr a);
void inta_remove_test(inta_ptr a, int (*test)(int));
void inta_swap(inta_ptr a, int i, int j);

/*@manual inta
Removes the pointer at index ''i'' in the dynamic array ''a''.
*/
void inta_remove_index(inta_ptr a, int n);
void inta_copy(inta_ptr dst, inta_ptr src);
void inta_remove_all(inta_ptr d);
void inta_forall(inta_t a, void (*func)(int));

/*@manual inta
Returns the number of pointers held in ''a''.
*/
static inline int inta_count(const inta_ptr a) { return a->count; }

static inline int *inta_raw(const inta_ptr a) { return a->item; }

static inline int inta_set_count(inta_ptr a, int n) { return a->count = n; }

static inline int inta_is_empty(const inta_ptr a) { return !a->count; }

static inline int inta_first(inta_t a) { return a->item[0]; }

static inline int inta_last(inta_t a) { return a->item[a->count - 1]; }

void inta_qsort(inta_t a, int (*compar)(const void *, const void *));

void inta_init_n(inta_t a, int n);
