target  ?= uninst
objects := $(patsubst %.c,%.o,$(wildcard *.c)) idblex.o

#libs:=sqlite3

#EXTRAS += -fsanitize=bounds -fsanitize=undefined -fsanitize=null -fcf-protection=full -fstack-protector-all -fstack-check -Wimplicit-fallthrough -fanalyzer -Wall -Wtrampolines -flto

ifdef libs
LDLIBS  := ${LDLIBS} $(shell pkg-config --libs   ${libs})
CFLAGS  := ${CFLAGS} $(shell pkg-config --cflags ${libs})
endif

LDFLAGS += ${EXTRAS}
CFLAGS  += -std=gnu23 -Og -ggdb ${EXTRAS}

.PHONY: all
all:	$(target) README

.PHONY: clean
clean:
	rm -f $(target) $(objects) README

.PHONY: install
install: ${target} ${target}.1
	install -m 755 ${target} /usr/local/bin
	install -m 755 -d /usr/local/share/man/man1
	install -m 644 ${target}.1 /usr/local/share/man/man1

.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/${target} /usr/local/share/man/man1/${target}.1

README: ${target}.1
	MANWIDTH=77 man --nh --nj ./${target}.1 | col -b > $@

$(target): $(objects)
