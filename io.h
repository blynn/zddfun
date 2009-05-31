// Requries stdarg, stdio.
extern void die(const char *err, ...)
    __attribute__((__noreturn__))  __attribute__((format (printf, 1, 2)));

#define EXPECT(condition) \
  if (condition); else fprintf(stderr, "%s:%d: FAIL\n", __FILE__, __LINE__)
