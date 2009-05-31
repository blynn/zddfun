// Requries stdarg, stdio.
extern void die(const char *err, ...)
    __attribute__((__noreturn__))  __attribute__((format (printf, 1, 2)));
