.PHONY: target

CFLAGS:=-lgmp -Wall -std=gnu99 -O2 -ltcmalloc

ZDD:=memo.c darray.c zdd.c io.c

target: fill

sud: sud.c $(ZDD)

tri: tri.c $(ZDD)

nono: nono.c $(ZDD)

light: light.c $(ZDD)

fill: fill.c $(ZDD)

dom: dom.c $(ZDD)
