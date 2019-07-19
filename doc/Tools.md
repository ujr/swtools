
Software Tools
==============

Inspired and largely taken from: 
Brian W. Kernighan and P. J. Plauger, 
*Software Tools in Pascal*, Addison-Wesley, 1981.


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

