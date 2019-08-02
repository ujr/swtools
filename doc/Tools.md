
Software Tools
==============

Inspired and largely taken from: 
Brian W. Kernighan and P. J. Plauger, 
*Software Tools in Pascal*, Addison-Wesley, 1981.


Tools in the book but not implemented here:
*overstrike*, *compress*, *expand*


Good Programs
-------------

Good quality programs require [p.2]:

  *  good design -- easy to maintain and modify
  *  human engineering -- convenient to use
  *  reliability -- you get the right answers
  *  efficiency -- you can afford to use them

Readability is the best criterion of program quality: if it is
hard to read, then it is hardly good [p.28].

**Testing:** Aim at a small selection of critical tests directed
at boundaries (where bugs usually hide). You must know in advance
the answer each test is supposed to produce: if you don't, you're
*experimenting*, *not testing* [p.19].


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

The *overstrike* tool I skip.


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

  1.  transliteration (src and dest of same length)
  2.  squash and delete (dest shorter or absent)
  3.  complement of src set (the -c option)
  4.  escapes and character ranges in the sets

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
[buf]: https://github.com/skeeto/growable-buf

