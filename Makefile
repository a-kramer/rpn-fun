CC = gcc
CFLAGS = -O2 -march=native
PREFIX = /usr/local/bin
MANPREFIX = /usr/local/man/man1

.PHONY: all install manual uninstall test


all: rpnc


rpnc: main.c
	$(CC) $(CFLAGS) -Wall -o $@ $^ -lm

install: rpnc
	install rpnc $(PREFIX)

uninstall:
	rm $(PREFIX)/rpnc && rm $(MANPREFIX)/rpnc.1.gz

manual: man/rpnc.1
	[ -d $(MANPREFIX) ] || mkdir $(MANPREFIX) && install $^ $(MANPREFIX) && gzip -f $(MANPREFIX)/rpnc.1

test: tests/rpnc.sh
	$^

prove: tests/rpnc.sh
	@prove $^
