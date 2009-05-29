.PHONY: target

CFLAGS:=-lgmp -Wall -std=gnu99 -O2 -ltcmalloc

ZDD:=cbt.c darray.c zdd.c

target: fill

sud: sud.c $(ZDD)

tri: tri.c $(ZDD)

nono: nono.c $(ZDD)

light: light.c $(ZDD)

fill: fill.c $(ZDD)
