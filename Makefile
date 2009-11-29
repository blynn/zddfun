.PHONY: target test clean public

CFLAGS := -O2 -Wall -std=gnu99

# I recommend appending -ltcmalloc for a slight boost.
LDFLAGS := -lgmp

zddcore := memo darray zdd io inta

ready := dom fill light loop nono

tests := cycle_test tiling_test

misc := sud nuri

binaries := $(ready) $(tests) $(misc)

target: $(ready)

test: $(tests)

define rule_fn
  $(1) : $(addsuffix .o,$(1) $(zddcore))
endef

$(foreach x,$(binaries),$(eval $(call rule_fn,$(x))))

clean:
	-rm $(addsuffix .o,$(binaries)) $(addsuffix .o,$(zddcore))

public:
	git push git@github.com:blynn/zddfun.git
	git push git+ssh://repo.or.cz/srv/git/zddfun.git
