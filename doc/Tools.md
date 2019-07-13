
Software Tools
==============

Inspired and largely taken from: 
Brian W. Kernighan and P. J. Plauger, 
*Software Tools in Pascal*, Addison-Wesley, 1981.


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


Manual Pages
------------

All tools come with a user documentation in the form of
a *manual page*. A manual page is *comprehensive* but *succinct*.
They have a standard structure, including the tool's name, its
usage and description, ideally an example, and sometimes notes
about bugs. For the section headings I use the Linux conventions,
not the ones used in the book.


File Copying
------------

The first tool in the book is *copy* for copying standard
input to standard output (pp. 7-12). The translation to C
is straightforward, especially because the concept of
standard input and output is native to C.

