
.POSIX:

CC = cc -std=c99
CFLAGS = -Wall -Wextra -Os -g3
LDFLAGS =
LDLIBS = # -lm
PREFIX = /usr/local

all: tools

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	cp -f quux $(DESTDIR)$(PREFIX)/bin
	gzip < quux.1 > $(DESTDIR)$(PREFIX)/share/man/man1/quux.1.gz

tools: bin/quux

TOOLS = obj/copy.o obj/count.o obj/echo.o obj/detab.o
bin/quux: obj/main.o $(TOOLS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

DEPS = src/common.h
obj/%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

dist: clean
	@echo "Package for distribution... TODO"

check: all
	@echo "Run the Test Suite... TODO"

clean:
	rm -f bin/quux obj/*.o

.PHONY: all install check clean dist tools
