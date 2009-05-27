.PHONY: target

CFLAGS:=-lgmp -Wall -std=gnu99 -O2 -ltcmalloc

target: tri

sud: sud.c cbt.c darray.c zdd.c

tri: tri.c cbt.c darray.c zdd.c

nono: nono.c cbt.c darray.c zdd.c
