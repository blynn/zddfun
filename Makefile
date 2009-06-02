.PHONY: target

CFLAGS:=-lgmp -Wall -std=gnu99 -O2 -ltcmalloc -lprofiler

ZDD:=memo.c darray.c zdd.c io.c inta.c

target: loop

sud: sud.c $(ZDD)

tri: tri.c $(ZDD)

nono: nono.c $(ZDD)

light: light.c $(ZDD)

fill: fill.c $(ZDD)

dom: dom.c $(ZDD)

tiling_test: tiling_test.c $(ZDD)

loop: loop.c $(ZDD)

cycle_test: cycle_test.c $(ZDD)
