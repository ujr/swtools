
Software Tools in C
===================

This is a glimpse at the classic *Software Tools in Pascal*
by Brian W. Kernighan and P. J. Plauger (Addison-Wesley, 1981).
It can still be bought at [Amazon][amazon] or may be found
in your local library.

Going through the lessons may seem odd from a 2019 perspective,
but I consider it a rite of passage. Here a selection of the
Pascal from the book will be recast in working C program.
Neither completeness nor fidelity is a goal.

My progress through the book and notes about the individual tools
and their restatement in C can be found in [doc/Tools.md](doc/Tools.md).


Structure
---------

While the book presents many separate tools, the implementation
here builds a single binary that contains all the tools.
To invoke a particular tool, pass the tool's name as the first
argument. Alternatively, create a link to the one binary and
name it for any of the tools; if the binary is invoked through
this link, the tool name need not be specified as the first argument.
(This is a common Unix pattern, employed e.g. by [BusyBox][busybox]).


Usage
-----

For lack of a better idea the binary is called `quux` (see
[quux][jargon] in the Jargon File). You may want to rename it.
Usage example:

>
    $ ./quux echo Hello
    Hello
    $ ln -s quux echo
    $ ./echo Hello
    Hello


Author and License
------------------

Written by UJR and dedicated to the public domain (see the file
*UNLICENSE* and [unlicense.org][unlicense]).


[amazon]: https://www.amazon.com/dp/0201103427
[busybox]: https://busybox.net/
[jargon]: http://catb.org/esr/jargon/html/Q/quux.html
[unlicense]: https://unlicense.org/

