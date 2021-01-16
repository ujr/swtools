# Software Tools in C

This is a look at the classic *Software Tools in Pascal*
by Brian W. Kernighan and P. J. Plauger (Addison-Wesley, 1981).
It can still be bought at [Amazon][amazon] or may be found
in your local library. See the book's [blurb](doc/Blurb.md).

Going through the lessons may seem odd from a 2019 perspective,
but I consider it a rite of passage. In this project most of
the Pascal tools from the book will be recast as working C
programs. Neither completeness nor fidelity is a goal.

My progress through the book, along with notes about the
individual tools and their restatement in C can be found
in [doc/Tools.md](doc/Tools.md). Notes not directly related
to the book or the tools are in [doc/Notes.md](doc/Notes.md).

## Getting Started

Clone the repository and run `make` in the checkout directory.
This will build the executable file in the *bin/* directory.
Run `make check` to run the unit tests.
For all else, read the source.

## Structure

While the book presents many separate tools, the implementation
here builds a single binary that contains all the tools.
To invoke a particular tool, pass the tool's name as the first
argument. Alternatively, create a link to the one binary and
name it for any of the tools; if the binary is invoked through
this link, the tool name need not be specified as the first argument.
(This is a common Unix pattern, employed e.g. by [BusyBox][busybox]).

## Usage

For lack of a better idea the binary is called `quux` (see
[quux][quux] in the Jargon File). You may want to rename it.
Usage example:

    $ ./quux echo Hello
    Hello
    $ ln -s quux echo
    $ ./echo Hello
    Hello

## Author and License

Written by UJR and dedicated to the public domain (read the
[UNLICENSE](./UNLICENSE) file or consult [unlicense.org][unlicense]).

[amazon]: https://www.amazon.com/dp/0201103427
[busybox]: https://busybox.net/
[quux]: http://catb.org/esr/jargon/html/Q/quux.html
[unlicense]: https://unlicense.org/
