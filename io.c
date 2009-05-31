#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "io.h"

void die(const char *err, ...) {
  va_list params;

  va_start(params, err);
  vfprintf(stderr, err, params);
  fputc('\n', stderr);
  exit(1);
  va_end(params);
}
