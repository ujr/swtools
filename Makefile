
.POSIX:

CC = cc -std=c99
CFLAGS = -Wall -Wextra -g3 -O0
LDFLAGS =
LDLIBS = # -lm
PREFIX = /usr/local

all: tools tests

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	cp -f quux $(DESTDIR)$(PREFIX)/bin
	gzip < quux.1 > $(DESTDIR)$(PREFIX)/share/man/man1/quux.1.gz

tools: bin/quux
tests: bin/runtests

TOOLS = obj/copy.o obj/count.o obj/echo.o obj/detab.o obj/translit.o \
  obj/compare.o obj/include.o obj/concat.o obj/print.o obj/sort.o \
  obj/unique.o obj/shuffle.o obj/find.o obj/change.o obj/edit.o
bin/quux: obj/main.o $(TOOLS) obj/strbuf.o obj/sorting.o obj/lines.o obj/regex.o obj/utils.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

symlinks: bin/quux
	ln -sf quux bin/change
	ln -sf quux bin/compare
	ln -sf quux bin/concat
	ln -sf quux bin/copy
	ln -sf quux bin/count
	ln -sf quux bin/echo
	ln -sf quux bin/edit
	ln -sf quux bin/find
	ln -sf quux bin/include
	ln -sf quux bin/print
	ln -sf quux bin/shuffle
	ln -sf quux bin/sort
	ln -sf quux bin/translit
	ln -sf quux bin/unique
	ln -sf quux bin/oops

DEPS = src/common.h src/strbuf.h src/test.h
obj/%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

TESTS = obj/buf_test.o \
        obj/strbuf_test.o obj/strbuf.o \
        obj/sorting_test.o obj/sorting.o \
		obj/regex_test.o obj/regex.o \
        obj/utils_test.o obj/utils.o
bin/runtests: obj/runtests.o obj/utils.o $(TESTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

dist: clean
	@echo "Package for distribution... TODO"

check: tests
	@echo "Running Test Suite..."
	bin/runtests

clean:
	rm -f bin/* obj/*.o

.PHONY: all install check clean dist tools tests
