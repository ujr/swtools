

Compiler Flags
--------------

  *  `-Wall` enable all warnings
  *  `-Wextra` enable some extra warnings (not included in `-Wall`)
  *  `-g3` include lots of debug information
  *  `-Os` optimize for code size (implies `-O2` but
     disables optimizations that could increase code size)

Generally, options and arguments can be mixed. Sometimes, however,
parameter ordering is relevant:

The linker searches object files (.o) and libraries (libX.a or -lX)
in the order they are specified. For example, if *foo.c* references
*sin* from the math library, `gcc -lm foo.c` will fail, whereas
`gcc foo.c -lm` will work because gcc will find the unresolved
symbol *sin* from *foo.c* in the following library *libm.a*.
Note that this behaviour may be used to override functionality.

Also, the search paths for libraries and include files are built
in the order specified by the `-L` and `-I` options.


Makefiles
---------

Do not use recursive Makefiles (breaks the dependency tree and results
in fragile builds); instead, have one Makefile at the root of the project.
To refer to files in subdirectories, include the subdirectory in the name,
for example: `obj/main.o: src/main.c`.

Beware that out-of-source builds (putting .o files in a separate
directory from the .c files) is not compatible with inference rules.

The special target `.SUFFIXES:` (without a value) clears the list
of built-in inference rules. Further `.SUFFIXES` (with a value)
will add to the now empty list:

>
    .SUFFIXES:
    .SUFFIXES: .c .o
    .c.o:
        $(CC) $(CFLAGS) -c $<

Conventional phony targets:

  *  all (default target)
  *  clean (delete all generated files)
  *  install (install built artifacts)
  *  distclean (delete even more than *clean*)
  *  test or check (run the test suite)
  *  dist (create a package for distribution)

The *install* target, by convention, should use PREFIX and DESTDIR:

  *  PREFIX: should default to */usr/local*
  *  DESTDIR: for staged builds (install in a fake root)


Standard Options
----------------

From <http://www.tldp.org/LDP/abs/html/standard-options.html>: 

>
    -h  --help       Give usage message and exit
    -v  --version    Show program version and exit

    -a  --all        Show all information / operate on all arguments
    -l  --list       List files or arguments (no other action)
    -o               Output file name
    -q  --quiet      Suppress stdout
    -r  --recursive  Operate recursively (down directory tree)
    -v  --verbose    Additional info to stdout or stderr
    -z               Apply or assume compression (usually gzip)

Note that `-v` is used in different ways; I tend to use `-V` (capital)
to show program version and `-v` to raise verbosity.


References
----------

Besides the book *Software Tools in Pascal*, the following references
have been consulted in the creation of this project:

[A Tutorial on Portable Makefiles](https://nullprogram.com/blog/2017/08/20/)
from Chris Wellons's blog (20 Aug 2017); it refers to the [specification for
make](http://pubs.opengroup.org/onlinepubs/9699919799/utilities/make.html)
at the Open Group (the POSIX standard).

[The Unlicense (unlicense.org)](https://unlicense.org/) is a simple way
to dedicate some work to the public domain.


