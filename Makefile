target  ?= idbscan
objects := $(patsubst %.c,%.o,$(wildcard *.c)) idblex.o

#libs:=sqlite3

#EXTRAS += -fsanitize=bounds -fsanitize=undefined -fsanitize=null -fcf-protection=full -fstack-protector-all -fstack-check -Wimplicit-fallthrough -fanalyzer -Wall -Wtrampolines

ifdef libs
LDLIBS  := ${LDLIBS} $(shell pkg-config --libs   ${libs})
CFLAGS  := ${CFLAGS} $(shell pkg-config --cflags ${libs})
endif

LDFLAGS += ${EXTRAS}
CFLAGS  += -std=gnu2x -Og -ggdb ${EXTRAS}

.PHONY: all
all:	$(target)

.PHONY: clean
clean:
	rm -f $(target) $(objects)

.PHONY: install
install: ${target}
	install -m 755 ${target} /usr/local/bin

.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/${target}

$(target): $(objects)
