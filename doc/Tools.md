
Software Tools
==============

Inspired and largely taken from:
Brian W. Kernighan and P. J. Plauger,
*Software Tools in Pascal*, Addison-Wesley, 1981.

Tools in the book but not implemented here:
*overstrike*, *compress*, *expand*, *archive*


Good Programs
-------------

Good quality programs require [p.2]:

  * good design -- easy to maintain and modify
  * human engineering -- convenient to use
  * reliability -- you get the right answers
  * efficiency -- you can afford to use them

Readability is the best criterion of program quality: if it is
hard to read, then it is hardly good [p.28].

**Testing:** Aim at a small selection of critical tests directed
at boundaries (where bugs usually hide). You must know in advance
the answer each test is supposed to produce: if you don't, you're
*experimenting*, *not testing* [p.19].

Further **guiding principles** and **best practices** from the book
(some have modern names, given in parentheses):

  * “there is great temptation to add more and more features” [p.80]
    (also known as _creaping featurism_)
  * whe n an option is left unspecified, this is never an error;
    (options are optional) instead choose some default value and
    “try not to surprise your users” [p.80]
    (the _principle of least surprise_)
  * “people cost a great deal more than machines” [p.82]


Manual Pages
------------

All tools come with a user documentation in the form of
a *manual page*. A manual page is *comprehensive* but *succinct*.
They have a standard structure, including the tool's name, its
usage and description, ideally an example, and sometimes notes
about bugs.

The example allows users (a) to reinforce their understanding,
and (b) to check that the program works properly.
The section on bugs is not for programming problems, but to
warn about shortcomings in the _design_ that could be improved
but are not worth the effort, at least not for now.

For the section headings I follow the Linux conventions,
not the ones used in the book.


Primitives
----------

Functions that interface to the 'outside world' are called
*primitives*. They allow a high level task to be expressed
in a small and well-defined set of basic operations and thus
insulate a program from its operating environment.

When programming in C, the Standard Library is such a set
of primitives. The authors of *Software Tools in Pascal*
had no such standard library available and started their
own set of primitives with the functions *getc* and *putc*
to read and write characters from and to some 'standard'
input and output file. Because the names *getc* and *putc*
are part of the C Standard Library, my implementation will
call them *getch* and *putch* and define them as macros
that translate to the standard C functions.


File Copying
------------

The first tool in the book is *copy* for copying standard
input to standard output (pp. 7-12). The translation to C
is straightforward, especially because the concept of
standard input and output is native to C.

The *copy* tool was amended to accept optional arguments
that specify an input file (must exist) and on output file
(will be created or truncated); it therefore subsumes
*makecopy* from the book (pp. 83-85).


Counting Bytes, Words, Lines
----------------------------

The three tools *charcount*, *wordcount*, and *linecount*
I combine into one tool *count*, much in the spirit of the
standard Unix *wc* tool, but at the price of having to deal
with command line options. An implementation of *wc* comes
as an example with the [CWEB][cweb] package (you may see
the generated [wc.pdf][wc.pdf] online).

[cweb]: https://ctan.org/tex-archive/web/c_cpp/cweb
[wc.pdf]: http://tex.loria.fr/litte/wc.pdf


Converting Tabs and Blanks
--------------------------

The two tools *detab* and *entab* convert tabs to blanks
and vice versa. (Unix comes with similar tools called
*expand* and *unexpand*.) My implementation follows
the variation suggested in Exercise 1-7(b) and my
*tabpos* function returns the *next* tab stop *after*
the given column.

Both *detab* and *entab* are in the same file *detab.c*
because they share a lot of code.


Compress and Expand
-------------------

The *compress* and *expand* (uncompress) tools implement a simple
run-length encoding, where runs of _n_ times a character _x_ are
encoded as *~Nx* where the tilde ~ is arbitrarily chosen as a
rarely-used character and _N_ is 'A'+_n_.
A literal `~` would be encoded as `~A~` and is thus three times
longer. Interesting is **exercise 2-14:**

> Prove that any compression scheme that is reversible, accepts any
> input, and makes some files smaller must also make some files longer.

Well, there are more longer strings than shorter strings. Any reversible
(that is, lossless) compression scheme therefore must map some strings
to longer strings, simply because there are not enough shorter strings.


Command Arguments and Echo
--------------------------

Other than with Pascal, in C the command line arguments are passed
to the `main` method and therefore readily available.
My C version of *echo* always outputs a newline, even if no arguments
are given. As in Unix versions of echo, the newline can be omitted
with the `-n` option, and interpretation of a few escape sequences can
be turned on with the `-e` option (though not the `\c` escape to
produce no further output). You may want to search the Internet
for the little story about *The Unix and the Echo* by Doug McIlroy.


Transliteration
---------------

The tool *translit* is analogous to the standard Unix *tr* command.
It is the first substantial program in the book and will be built
in four steps:

  1. transliteration (src and dest of same length)
  2. squash and delete (dest shorter or absent)
  3. complement of src (the -c option)
  4. escapes and character ranges in src and dest

For the 4th step we want some "string buffer" for expanding the
character ranges. C has no such facility, so it must be written.
Different approaches exist, but all must maintain three pieces
of information: a character buffer, its size, and the length of
the string within this buffer. It might be tempting to join the
buffer and the housekeeping (size and length) into a single
allocation, and to hide the housekeeping part so that the buffer
_looks like_ a regular C string (see, e.g., [growable-buf][growable-buf]
and [sds][sds]). The downside is that such schemes potentially
return a new buffer pointer on each append operation, and
that such strings are easily confused with plain C strings.
My implementation, *strbuf*, is an explicit string buffer
with separate housekeepking.

By the way, both the book and the man page speak of _character_
transliteration, but it really is _byte by byte_ transliteration.
With ASCII (and other one-byte-per-character encodings) this makes
no difference. But with multi-byte encodings (e.g. UTF-8)
this does make a difference, but I ignore it here.

[sds]: https://github.com/antirez/sds
[growable-buf]: https://github.com/skeeto/growable-buf


Line Input and Compare
----------------------

The next tool is *compare* for line-by-line comparison of two files.
A function to read a line of input will be needed. There are several
options:

  1. using a fixed size buffer: `getline(char *buf, size_t len, FILE *fp)`
  2. using a growable buffer (e.g., strbuf): `getline(strbuf *sp, FILE *fp)`
  3. using `getline(3)` from the standard library (but beware:
     this is a GNU extension that became part of POSIX around
     2008 and may not be available everywhere):
     `getline(char **buf, size_t **len, FILE *fp)`

The first option is simple to write but harder to use.
Since we already have a growable string buffer implementation
(strbuf), the second option is simple to write and simple to use.
The third option is certainly more efficient, because it has access
to the innards of the standard IO library.
Will go for the second option. Return value is number of characters
read, including the delimiter, 0 on end-of-file, -1 on error.

Unlike the standard *diff* tool, the *compare* here is a plain
line-by-line comparison and therefore may find lots of differing
lines if only one line has been added or deleted.

**Exercise 3-5** asks what happens if *compare* is asked to
compare a file with itself, that is: **compare _f_ _f_**  
If _f_ is a named file, this should and will work: _f_ is
opened twice, each open file has its own file pointer, and
its own current offset. If _f_ refers to the standard input
(can be achieved with **compare - -** or **foo | compare -**),
an already opened file, then there is only one file pointer
and only one current offset, so that *compare* would compare
odd numbered lines against even numbered lines.
This special case is recognized and handled specially
by defining standard input to be identical to itself.


File Inclusion
--------------

The *include* tool copies input to standard output, replacing
any line beginning `#include "filename"` with the contents of
_filename_, which can itself include other files. This is exactly
like the `#include` mechanism of the C preprocessor. It can be
used to stitch files together from smaller files.


File Concatenation
------------------

The *concat* tool concatenates each file argument, in order,
to standard output. It is similar to the *copy* tool we started
with, except that here we have to open and close the files listed
as arguments. If no arguments are specified, copy standard input
to standard output, as does the *copy* tool. Resist temptation
to merge *concat* and *copy* into one tool, because the latter
might be generalized into another way (namely to copy to a newly
created file instead of standard output).

An implementation of the standard *cat* tool (here called *concat*)
serves as an example in K&amp;R (Section 7.6 in the 2nd edition).
It is similar to the Pascal code in *Software Tools*.
And since there's not too much room for variation, my implementation
is similar too.

(The GNU version of *cat* has a number of options to show line
ends, non-printing characters, squeeze runs of empty lines, etc.
These are not part of POSIX, however.)


File Printing
-------------

The book's *print* tool is intended to "print" some files with top
and bottom margins, a heading line, and filling the last page with
blank lines before beginning the next file.

Here instead I write a tool (with the same name) that prints
arbitrary files to standard output, replacing non-printing
characters by escape sequences, and folding long lines.
Optionally, it can mark the end-of-line (useful if lines end
with white space), prefix line numbers or byte offsets.
It is somewhat similar to the standard *od* file dump tool.


Sorting
-------

Chapter 4 develops a tool to sort the lines in text files,
much like the standard Unix *sort* tool. We need a sorting
algorithm, a definition for ordering lines and characters,
input and output routines, and a way to cope with *big* files
(larger than fit into memory).

**Bubble sort** [p.109] is simple, well-known, but slow (running
time grows as _n_ squared when _n_ is the input size).
**Shell sort** [p.110] is more complex, faster, and like Bubble sort,
it requires no auxiliary memory; its details are intricate
and for a real-world application a good library routine
should be preferred over a home-grown implementation.
**Quicksort** [p.117] runs on average in _n_ log _n_ time and
is remarkably simple when described recursively; other than
Shell sort, it requires a small amount of auxiliary memory,
either implicitly in the call stack, or with an explicit stack.

In *src/sorting.c* the algorithms are restated in C, and
a *make check* will run a speed comparison. On a very small
array, Bubble sort is good because it has little overhead,
but its quadratic time complexity soon becomes a pain.
For a real-world application, consider using the C library's
*qsort* routine or a specialised sorting library.

> The book on *The AWK Programming Language* has an excellent
> section on Insertion Sort, Quicksort, and Heapsort, along
> with hints on testing sort algorithms:
>
> * the empty input (length 0)
> * a single item (length 1)
> * _n_ random items
> * _n_ sorted items
> * _n_ reverse-sorted items
> * _n_ identical items

The first approach at the *sort* tool assumes that the whole
input fits into memory and thus an internal sort can be used.

  * Input lines are read into one character array *linebuf*,
    separated by NUL bytes.
  * A separate array *linepos* contains indices to the beginning
    of each line in *linebuf*.
  * Only *linepos* needs to be sorted, not the variable-length lines.
  * The size of the two arrays is not known until after the input
    has been read; the public domain [buf.h][growable-buf] header-only
    library will be used to implement growable arrays.

This design precludes NUL in the input. To allow null, we could
explicitly record starting position *and* length of each line
in *linebuf*.

**Exercise 4-6:** reverse sorting (option `-r`) is best implemented
in *compare* (all order-defining logic in one place) or
in *writelines* (very simple and efficient) (in the book
those two methods are called *compare* and *ptext*).

