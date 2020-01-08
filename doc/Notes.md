
General Notes
=============


Compiler Flags
--------------

  * `-Wall` enable all warnings
  * `-Wextra` enable some extra warnings (not included in `-Wall`)
  * `-g3` include lots of debug information
  * `-Os` optimize for code size (implies `-O2` but
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


C89 vs C99
----------

**K&R C** is the original C, as described in the first edition (1978)
of *The C Programming Language* by Kernighan and Ritchie.
C was standardized by ANSI in 1989, and the second edition (1988)
of *The C Programming Language* describes this revised version
of C, known as **ANSI C** or **C89**. As compilers added features
C was standardized again in 1999, creating the **C99** dialect.
Today (2019) it is probably save to assume that C99 is supported
by your compiler. Some of the new C99 features are:

  * C++ style line comments: `// comment`
  * the Boolean data type (`_Bool`, stdbool.h provides `bool`)
  * mixing declarations and code (not only at start of block)
  * variable-length arrays (length determined at runtime)
  * [flexible array members][fam] (member array without a dimension)
  * variadic macros, the [offsetof][offsetof] macro (from stddef.h)

[fam]: https://en.wikipedia.org/wiki/Flexible_array_member
[offsetof]: https://en.wikipedia.org/wiki/Offsetof


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

  * all (default target)
  * clean (delete all generated files)
  * install (install built artifacts)
  * distclean (delete even more than *clean*)
  * test or check (run the test suite)
  * dist (create a package for distribution)

The *install* target, by convention, should use PREFIX and DESTDIR:

  * PREFIX: should default to */usr/local*
  * DESTDIR: for staged builds (install in a fake root)


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


Operator Precedence
-------------------

>
    call, index, member  ()  []  ->  .
    unary           ! ~ ++ -- + - * & (type) sizeof    (right-to-left)
    multiplicative  *  /  %
    additive        +  -
    bit shift       <<  >>
    relational      <  <=  >  >=
    equality        ==  !=
    bitwise AND     &
    bitwise XOR     ^
    bitwise OR      |
    logical AND     &&
    logical OR      ||
    conditional     ?:                                 (right-to-left)
    assignment      = += -= *= /= %= &= ^= |= <<= >>=  (right-to-left)
    comma           ,

Operators are listed in order from highest precedence (tightest binding)
to least precedence. All operators associate left-to-right unless
indicated otherwise. For example, 4-2+1 is 3 (not 1).


Rounding up to multiples of 2, 4, 8 (powers of two)
---------------------------------------------------

The integer *n* can be rounded up to a multiple of 2, 4, 8 like this:

>
    (n+1)&~1
    (n+3)&~3
    (n+7)&~7

The bitwise AND masks off the low order bits, thereby rounding _down_
to a multiple of a power-of-two. To effect a rounding _up_ instead,
add one less than the desired power-of-two prior to masking.

The mask is created by a bitwise NOT: 3 is the two leftmost bits set,
`~3` is all bits set except the two leftmost.
The expression `x&~3` is independent of word size, and thus preferable
to, e.g., `x&0xFFFC`, which assumes 16-bit words.


Limiting Resources
------------------

It may be useful to limit process resources for testing.
For example, a tool can be run with very limited memory.

Unix provides the getrlimit(2) and setrlimit(2) syscalls,
as well as the now obsolete ulimit(2) syscall for setting
user resource limits. The *bash* exposes this functionality
through the **ulimit** built-in command.

>
    ulimit -a              show all current limits (and units)
    ulimit -d 1024         limit data segment (memory) to 1M
    ulimit -s 32           limit stack size to 32K
    ulimit -u 5            user cannot have more then 5 processes

Note that a non-priviledged process cannot raise limits!  
(To get out: exit your shell/terminal and open a new one.)


Markdown
--------

Markdown is a plain text formatting syntax widely used
for README files and the like. The idea is that you write
plain text following a very few syntax rules, and then
a software tool will convert it to nicely looking HTML
(from markdown to markup). GitHub recognises a few extra
features, known as “GitHub Flavored Markdown” (or GFM).
Online previewers and even editors exist.

  * <https://daringfireball.net/projects/markdown/>
  * <https://github.github.com/gfm/>
  * <https://jbt.github.io/markdown-editor/>


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

The [POSIX standards](http://pubs.opengroup.org/onlinepubs/9699919799/)
(IEEE Std 1003.1-2017), volume about *Shell & Utilities*.

Rob Pike, *Notes on Programming in C*, 1989,
[archived at Lysator](https://www.lysator.liu.se/c/pikestyle.html),
and [local copy](/doc/PikeStyle.md)

Kernighan and Ritchie, *The C Programming Language*, 2nd edition,
Prentice Hall 1988, [ISBN 0131103628](https://www.amazon.com/dp/0131103628).

Aho, Kernighan, Weinberger, *The AWK Programming Language*,
Addison-Wesley 1988, [ISBN 020107981X](https://www.amazon.com/dp/020107981X).

*The Unix Philosophy* at [Wikipedia](https://en.wikipedia.org/wiki/Unix_philosophy).
