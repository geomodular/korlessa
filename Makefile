CFLAGS  = -O2 -s -std=c99 -pedantic -Wall
CFLAGSD = -g -std=c99 -pedantic -Wall
LDFLAGS = -lasound
PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin

korlessa: main.c parser.c parser.h list.c list.h listing.c listing.h translator.c translator.h scheduler.c scheduler.h
	$(CC) $(CFLAGS) main.c lib/mpc.c parser.c list.c listing.c translator.c scheduler.c -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f korlessa

.PHONY: indent
indent:
	indent *.c *.h
	rm *.c~ *.h~

.PHONY: install
install: korlessa
	install -m 0755 korlessa $(BINDIR)

