.PHONY: target

CFLAGS:=-lgmp -Wall -std=gnu99 -O2 -ltcmalloc

target: paint

sud: sud.c cbt.c darray.c

tri: tri.c cbt.c darray.c

paint: paint.c cbt.c darray.c
