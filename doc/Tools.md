# Software Tools

My path through the book *Software Tools in Pascal* by
Brian W. Kernighan and P. J. Plauger, Addison-Wesley, 1981.

## Good Programs

Good quality programs require [p.2]:

- good design — easy to maintain and modify
- human engineering — convenient to use
- reliability — you get the right answers
- efficiency — you can afford to use them

Readability is the best criterion of program quality: if it is
hard to read, then it is hardly good [p.28].

## Controlling Complexity

... is the essence of computer programming. [p.311]

A selection of **guiding principles** and **best practices** from
the book (some have modern names, given here in parentheses):

- “there is great temptation to add more and more features” [p.80]
  (*creaping featurism*)
- when an option is left unspecified, this is never an error
  (options are optional); instead choose some default value and
  “try not to surprise your users” [p.80]
  (*principle of least surprise*)
- “people cost a great deal more than machines” [p.82]
- “proper separation of function” [p.131]
  (*separation of concerns*, which is an old design principle)
- “start testing as soon as possible” (*test driven development*)
  and “top-down testing is a natural extension of top-down
  design and top-down coding” [p.146]
- if you build something, “make sure it has some conceptual
  integrity” and “build it in increments” [p.263] (be *agile*)
- “information hiding is critical to program design” [p.275]
  (*information hiding*)
- “rewriting will always remain an important part of programming”
  [p.311] (*refactoring*, that is, reading and revising)

The book concludes with three guidelines for attacking
a programming task:

1. keep it simple
2. build it in stages
3. let someone else do the hard part

## Testing

Aim at a small selection of critical tests directed at
boundaries (where bugs usually hide). You must know in
advance the answer each test is supposed to produce:
if you don't, you're *experimenting, not testing*. [p.19]

## Manual Pages

All tools come with a user documentation in the form of
a *manual page*. A manual page is *comprehensive* but *succinct*.
They have a standard structure, including the tool's name, its
usage and description, ideally an example, and sometimes notes
about bugs.

The example allows users (a) to reinforce their understanding,
and (b) to check that the program works properly.
The section on bugs is not for programming problems, but to
warn about shortcomings in the *design* that could be improved
but are not worth the effort, at least not for now.

For the section headings I follow the Linux conventions,
not the ones used in the book.

## Primitives

Functions that interface to the 'outside world' are called
*primitives*. They allow a high level task to be expressed
in a small and well-defined set of basic operations and thus
insulate a program from its operating environment.

When programming in C, the Standard Library is such a set
of primitives. The authors of *Software Tools in Pascal*
had no such standard library available and started their
own set of primitives with the functions *getc* and *putc*
to read and write characters from and to some 'standard'
input and output file. Notes on the book primitives and
their mapping to the C standard library is in the separate
[Primitives.md](Primitives.md) document.

## File Copying

The first tool in the book is **copy** for copying standard
input to standard output (pp. 7–12). The translation to C
is straightforward, especially because the concept of
standard input and output is native to C.

The *copy* tool was amended to accept optional arguments
that specify an input file (must exist) and on output file
(will be created or truncated); it therefore subsumes
*makecopy* from the book (pp. 83–85).

## Counting Bytes, Words, Lines

The three tools *charcount*, *wordcount*, and *linecount*
I combine into one tool **count**, much in the spirit of the
standard Unix *wc* tool, but at the price of having to deal
with command line options. An implementation of *wc* comes
as an example with the [CWEB][cweb] package (you may see
the generated [wc.pdf][wc.pdf] online).

[cweb]: https://ctan.org/tex-archive/web/c_cpp/cweb
[wc.pdf]: http://tex.loria.fr/litte/wc.pdf

## Converting Tabs and Blanks

The two tools **detab** and **entab** convert tabs to blanks
and vice versa. (Unix comes with similar tools called
*expand* and *unexpand*.) My implementation follows
the variation suggested in Exercise 1-7(b) and my
*tabpos* function returns the *next* tab stop *after*
the given column.

Both *detab* and *entab* are in the same file
[detab.c](../src/detab.c) because they share a lot of code.

## Compress and Expand

The *compress* and *expand* (uncompress) tools implement a simple
run-length encoding, where runs of *n* times a character *x* are
encoded as *~Nx* where the tilde ~ is arbitrarily chosen as a
rarely-used character and *N* is 'A'+*n*.
A literal `~` would be encoded as `~A~` and is thus three times
longer. Interesting is **exercise 2-14:**

> Prove that any compression scheme that is reversible, accepts any
> input, and makes some files smaller must also make some files longer.

Well, there are more longer strings than shorter strings. Any reversible
(that is, lossless) compression scheme therefore must map some strings
to longer strings, simply because there are not enough shorter strings.

## Command Arguments and Echo

Other than with Pascal, in C the command line arguments are passed
to the `main` method and therefore readily available.
My C version of **echo** always outputs a newline, even if no arguments
are given. As in Unix versions of echo, the newline can be omitted
with the `-n` option, and interpretation of a few escape sequences can
be turned on with the `-e` option (though not the `\c` escape to
produce no further output). You may want to read the little story
about [The Unix and the Echo](./UnixEcho.md) by Doug McIlroy.

## Transliteration

The tool **translit** is analogous to the standard Unix *tr* command.
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
*looks like* a regular C string (see, e.g., [growable-buf][growable-buf]
and [sds][sds]). The downside is that such schemes potentially
return a new buffer pointer on each append operation, and
that such strings are easily confused with plain C strings.
My implementation, [strbuf.c](../src/strbuf.c), is an explicit
string buffer with separate housekeepking.

By the way, both the book and the man page speak of *character*
transliteration, but it really is *byte by byte* transliteration.
With ASCII (and other one-byte-per-character encodings) this makes
no difference. But with multi-byte encodings (e.g. UTF-8)
this does make a difference, but I ignore it here.

Examples of transliteration:

- `translit a-z A-Z` — to upper case
- `translit a-z b-za` — a Caesar shift
- `translit a-zA-Z a | translit 0-9 n`
- `translit -c a-z -` — replace all non-letters with a dash
- `translit -c a-z` — delete all but lower-case letters
- `translit ' \t\n' '\n'` — leave one world per line
- `translit '\\'` — remove backslashes

[sds]: https://github.com/antirez/sds
[growable-buf]: https://github.com/skeeto/growable-buf

## Line Input and Compare

The next tool is **compare** for line-by-line comparison of two files.
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
read, including the delimiter, `0` on end-of-file, `-1` on error.

Unlike the standard *diff* tool, the *compare* here is a plain
line-by-line comparison and therefore may find lots of differing
lines if only one line has been added or deleted.

**Exercise 3-5.** What happens if *compare* is asked
to compare a file with itself (`compare f f`)?
If *f* is a named file, this should and does work: *f* is
opened twice, each open file has its own file pointer, and
its own current offset. If *f* refers to the standard input
(can be achieved with `compare - -` or `foo | compare -`),
an already opened file, then there is only one file pointer
and only one current offset, so that *compare* would compare
odd numbered lines against even numbered lines.
This special case is recognized and handled specially
by defining standard input to be identical to itself.

## File Inclusion

The **include** tool copies input to standard output, replacing
any line beginning `#include "filename"` with the contents of
*filename*, which can itself include other files. This is like
the `#include` mechanism of the C preprocessor and can be used
to stitch files together from smaller files.

As an extension over the book, we qualify relative include file
names against the current file. For example, when the file `foo/bar`
includes the file `baz`, the file `foo/baz` will be read. This is
what users would expect. (For this purpose, standard input is
considered to be a file in the current working directory).

## File Concatenation

The **concat** tool concatenates each file argument, in order,
to standard output. It is similar to the **copy** tool we started
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

## File Printing

The book's **print** tool is intended to “print” some files with
top and bottom margins, a heading line, and filling the last page
with blank lines before beginning the next file.

Here instead I write a tool (with the same name) that prints
arbitrary files to standard output, replacing non-printing
characters by escape sequences, and folding long lines.
Optionally, it can mark the end-of-line (useful if lines end
with white space), prefix line numbers or byte offsets.
It is somewhat similar to the standard *od* file dump tool.

## Sorting

Chapter 4 develops a tool to sort the lines in text files,
much like the standard Unix **sort** tool. We need a sorting
algorithm, a definition for ordering lines and characters,
input and output routines, and a way to cope with *big* files
(larger than fit into memory).

**Bubble sort** [p.109] is simple, well-known, but slow (running
time grows as *n* squared when *n* is the input size).
**Shell sort** [p.110] is more complex, faster, and like Bubble
sort, it requires no auxiliary memory; its details are intricate
and for a real-world application a good library routine
should be preferred over a home-grown implementation.
**Quicksort** [p.117] runs on average in *n* log *n* time and
is remarkably simple when described recursively; other than
Shell sort, it requires a small amount of auxiliary memory,
either implicitly in the call stack, or with an explicit stack.

In [sorting.c](../src/sorting.c) the algorithms are restated
in C, and *make check* will run a spead comparison. On a very
small array, Bubble sort is good because it has little overhead,
but its quadratic time complexity soon becomes a pain.
For a real-world application, consider using the C library's
*qsort* routine or a specialised sorting library.

> The book on *The AWK Programming Language* has an excellent
> section on Insertion Sort, Quicksort, and Heapsort, along
> with hints on testing sort algorithms. Be sure to test:
>
> - the empty input (length 0)
> - a single item (length 1)
> - *n* random items
> - *n* sorted items
> - *n* reverse-sorted items
> - *n* identical items

The first approach at the *sort* tool assumes that the whole
input fits into memory and thus an internal sort can be used.

- Input lines are read into one character array *linebuf*,
  separated by NUL bytes.
- A separate array *linepos* contains indices to the beginning
  of each line in *linebuf*.
- Only *linepos* needs to be sorted, not the variable-length lines.
- The size of the two arrays is not known until after the input
  has been read; the public domain [buf.h][growable-buf] header-only
  library will be used to implement growable arrays.

This design precludes NUL in the input. To allow NUL, we could
explicitly record starting position *and* length of each line
in *linebuf*.

**Exercise 4-6:** reverse sorting (option `-r`) is best implemented
in *compare* (all order-defining logic in one place) or
in *writelines* (very simple and efficient) (in the book
those two methods are called *compare* and *ptext*).

**External sorting** is required when there is more data than
fits into memory. The approach taken here is to sort chunks
(runs) of the data and store them to temporary files. Then the
first *m* of these files are merged into a new temporary file
and then removed. This is repeated with the next *m* temporary
files until only one file is left, which is the sorted output.
The merger order *m* is a parameter, typically between 3 and 7.
Existing functionality can be used to sort the individual runs,
but merging is new and we also need to cope with those temporary
files.

The temporary files will be */tmp/sortxxxx.tmp* or *$TMPDIR/sortxxx.tmp*,
which is a bit too simple for real life: we risk overwriting existing
data, which could also be a link to an essential file! For mitigation,
create a directory writable only to the sorting routine and set the
`TMPDIR` environment variable to this directory; sort will then create
its temporary files in this directory instead of in */tmp*.

**Merging** uses a heap and works like this:

```text
read one line from each file
form a heap from those lines
while heap not empty:
  output heap[1], which is always the smallest line
  read the next line from the same file into heap[1]
  restore the heap property (“reheap”)
```

A **heap** is a complete binary tree such that each node is
less than or equal to its children (the heap property). It
can be represented in an array: index 0 is left unused, the
two children of the node at index `k` are at index `2k` and
`2k+1`. The initial heap will be created by in-memory sorting
(a sorted array always has the heap property).

Reverse sorting (exercise 4-6) must be revised: implementing
it in *writelines* is no longer feasible, it has to go into
a central comparison routine, which is to be used for sorting
the individual runs and for the merging of the runs.

Further options allow case folding (`-f`), “dictionary sort”
(`-d`, exercise 4-19) where runs of non-alpha-numeric characters
are considered a single blank, and an initial numeric string
(`-n`, exercise 4-20) that will sort numerically, while the
remainder of the line still sorts lexicographically.

Beware that on some systems `char` is signed and on some it's
unsigned. We always cast to `unsigned char` before passing
a character to any of the `<ctype.h>` functions (because they
implicitly convert to `int` and we want no sign extension)
and before computing character differences. When having a
character string, we can convert the string pointer to
`unsigned char*` (via `void*` to avoid a warning about
differing signedness).

## Shuffling Lines

Shuffling is not in the book, but a nice complement to sorting:
once we can order things, we might like some chaos... Shuffling
could be implemented as an option to **sort** because we already
have the whole line reading/writing machinery ready, but from a
user perspective shuffling is sufficiently different from sorting
that it merits its own tool called **shuffle**. The line reading
and writing code is factored out into its own file that is
referenced by both sort and shuffle.

The shuffle algorithm is known as “Fisher-Yates”: swap the
last item with an item from the array chosen at random, then
repeat this same step with the first `n-1` items until there
is only one item left. It requires a source of random numbers
in the `[0,n)` range (we use `rand()%n`, which is not optimal
but shall do for now).

```text
shuffle(array, n):
  while n > 1:
    let k = random(n)  # 0 <= k < n
    n -= 1
    swap(array, k, n)
```

The shuffle tool loads all lines into memory.
There is no “external shuffling” as there is for sort.

## Adjacent Duplicate Lines

Once a file is sorted, it is not difficult to find duplicate
lines, because they are adjacent. The **unique** tool removes
adjacent duplicate lines (keeping only one copy). If the
option `-n` is given, each line is prefixed with the number of
occurrences of the line in the original input (exercise 4-25;
it would be a bad idea to suffix this number, because then
it is in a variable position).

The occurrence prefix minimally complicates matters because
we only know the number of occurrences after having seen the
first non-duplicate line. Only now can we emit the number
and the line. The overall plan is to use two line buffers
and proceed like this:

```text
read 1st line into buf0; return if eof; num=1
while read next line into buf1:
  if same as buf0: ++num;
  else: emit(num, buf0); swap buf0/buf1; num=1
emit(num, buf0)
```

If buf0 and buf1 are pointers to the actual buffers,
the “swap” does not have to copy a line, it simply
changes pointers. The “emit” after the loop is necessary
because only upon end-of-input do we know the number of
occurrences of the last line.

With the tools so far we can construct a pipeline to
create a word frequency list for a document; for example:

```sh
concat UNLICENSE |
  translit 'A-Z \t\n' 'a-z\n' | translit '.,:"' |
    sort -d -f | unique -n | sort -n -r
```

Options `-d` and `-f` (as with sort) would be very useful.

**Exercise 4-28** proposes a tool **common** that compares
lines in two sorted text files and produces three-column
output: lines only in the 1st file, lines only in the
2nd file, and lines in both files. Options `-1`, `-2`,
`-3` can be used to print only the corresponding column.

Such a tool, in combination with *translit*, *sort*, and
*unique*, allows comparing documents against dictionaries
for indicating spelling problems or glossary terms.
Interesting but not implemented here.

## Find patterns in lines

The next tool, called **find**, looks for input lines that
match a given *pattern*. It corresponds to the well-known
Unix *grep* tool, though for only a limited subset of
*regular expressions* and having fewer options.

Option `-i` ignores case, option `-n` prefixes matched lines
with their line number (exercise 5-20), and option `-v` emits
those lines that do not match the pattern (exercise 5-9). If
the search extends over multiple files (exercise 5-8), matches
will be prefixed by the corresponding file name.

The pattern matching logic is straightforward except for
closures (the `*` operator), which use a backtracking
approach. “Real” implementations of regular expression
matching compile the pattern into an automaton that
can do the matching in a single pass over the input
without backtracking, though I did not investigate.

Support for the “one or more” operator `+` is easy
to implement once “zero or more” `*` closures are
working (exercise 5-10). Note that other than in the
book, I use `.` instead of `?` to match any character.

The C implementation of *find* is very close to the
Pascal code from the book. The only major differences
are that I do not pass indices by reference (`var` in
Pascal) but instead choose appropriate return values,
that I use C's capability to return anywhere from a
function, and the strbuf library for growable strings.

The problem with Pascal `var` arguments is that you
cannot see that arguments are passed by reference,
which makes for hard-to-read code. (C# requires the
`ref` keyword to be present in method signatures
*and* in method invocations, so it is immediately
obvious where arguments are passed by reference.)

Here are some patterns to test: `^` and `$` match
each line, whereas `^$` only matches empty lines,
and `^[ \t]*$` matches blank lines.

## Change matched text

With the pattern matching logic of *find* available,
the next tool, **change**, allows changing the matched
substrings. Change is often classified into three
operations: *insertion*, *deletion*, and *replacement*;
our tool will support all three. Essential is the
possibility to refer to the matched text from the
replacement text, which we do with the `&` operator.

```sh
change mispell misspell  # replace
change " *$"             # delete trailing blanks
change active "in&"      # prefix "in" (insert)
change "a+b" "(&)"       # parenthesize
change very "&, &"       # stronger emphasis
change and "\&"          # escape for a literal ampersand
```

The C implementation is again close to the Pascal code
in the book, with the exception that no special `DITTO`
value is used in the prepared substitution string; instead,
the literal `&` serves the purpose, and escaping is handled
in `putsub`. This way all possible char values are allowed.

With the tools so far it is possible to reverse
the order of the lines in a file:

```sh
find -n $ | sort -r -n | change "^[0-9]*:"
```

The remaining book chapters cover more substantial projects:
a text editor, a text formatter, and a macro processor.

## Text Editing

The book's chapter 6 is devoted to **edit**, a text editor
similar to the standard Unix *ed* editor. The editor's design
and use is explained, in a way similar to the description of
the standard editor in Appendix 1 of Kernighan and Pike's 1984
*The Unix Programming Environment*.
The editor is not like today's screen editors. It is driven
by commands that may be entered interactively, or read from
a script file. This makes it a valuable tool, but somewhat
inconvenient for interactive use, at least when compared to
today's editors. See the manual page for details.

The editor is the largest program in the book. It can be
roughly broken into three parts: (1) the buffer, upon
which (2) the commands operate, and (3) command parsing.

Input to the editor is a series of commands, one per line,
each of which looking like

```text
line1,line2 command stuff
```

where *line1*, *line2* and *stuff* are optional.
This entails a main loop of the form

```text
while getline(cmdline, stdin):
  get list of line numbers from cmdline
  if status is OK:
    do command
```

**Exercise 6-9** addresses a subtlety of the *substitute*
command: what if the trailing newline is substituted away
or an additional newline is substituted into existence?
Two approaches seem reasonable: (1) prohibit *substitute*
from matching or substituting a newline, or (2) make insertion
split the line, and removal join two lines. In practice, a
compromise might be best suited: (3) make insertion split
the line, prohibit removal of the trailing newline, and
have a specific *join* command `j` to join lines.
Approach (3) is implemented here.

**Exercise 6-12** asks for a `u` command to **undo** the last
substitute or, more generally, any command. Lines in the buffer
are immutable and old lines are kept at the invisible end of the
buffer (lines beyond `lastln`). Therefore, *undo* can restore
those lines into place.
Indeed, the editor's only low-level operation is moving lines
in the buffer. We keep track of those moves and *undo* will do
the inverse moves. It will also need to restore former values
of `curln` and `lastln`. By using a stack, we can provide any
number of *undo* and *redo*. (Presently implemented that way,
but global commands generate many undo steps, not just one.)

Is unlimited undo/redo really useful in an editor like ours?
Other than with screen editors, it is not easy for the user
to see the effect of an undo. Therefore, having just one level
of undo might be all that is needed.

The **global command** faces the difficulty that it must
process all matching lines, regardless of how much they are
rearranged. The book's approach works in two passes:
first, mark all matching lines; second, iterate over the
line buffer repeatedly until no marked lines remain. This
always terminates (**exercise 6-15**) because each marked
line found is immediately unmarked and introduced lines, if
any, are never marked. Therefore, in `doglob`, eventually
only the `else` branch within the loop will be taken
and `count` incremented beyond `lastln`.

## Text Formatting

Chapter 7 develops a text formatter in the spirit of Unix
*troff* but much simpler. Not implemented here.

## Macro Processing

Macros extend an underlying language by expanding certain
keywords (or other syntactic constructs) with some replacement
text. This process is called *macro expansion* and the tool
that performs it is a *macro processor*.

The book approaches macro processing in two steps: first
is a tool **define** that replaces words by some string,
such that after the definition `define(EOF, -1)` all
occurrences of the word `EOF` will be replaced by `-1`.
Second is a tool **macro** that allows macros to have
arguments, as in `define(putchar(c), fputc(c, stdout))`,
and will provide a number of built-ins for conditional macros,
arithmetics, and the like.

Fundamental design decisions for the book's macro processor:

- The unit of replacement: an identifier in a typical language.
- Evaluation: as late as possible for **define** and as early
  as possible for **macro**.
- Rescanning: a macro's replacement text undergoes again macro expansion.

Rescanning implies that an invocation of `x` after `define(x,x)`
produces an infinite loop, but also that in both cases below,
`y` produces `1` (good because the user does not have to
care about the ordering).

```text
define(x, 1)                define(y, x)
define(y, x)                define(x, 1)
```

The overall structure of the **define** processor is:

```text
while gettok(token) != ENDFILE:
  if token is "define":
    install new token and replacement text
  else if lookup(token) succeeds:
    switch input to token's replacement text
  else
    copy token to output
```

```text
define(d, define)
d(a,b)
a                      // expands to b

define(define, x)
define(a,b)            // expands to x(a,b)

define(empty,)
empty                  // expands to the empty string
define(,oops)          // error: invalid macro name
define( x ,oops)       // error: invalid macro name

define(x,y)
define(y,x)
x                      // infinite loop!

define(x, x x)
x                      // eats all your memory!
```

Macros with arguments: follow book's code but with some
changes: (1) array of struct frame instead of parallel
arrays typestk, plev, callstk; (2) use growable buffers
instead of fixed-size arrays; (3) rename some functions.

How does it work? Copy input to output, token by token.
If a token is defined, push replacement text to evaluation
stack and start argument collection. If a replacement or
argument token is defined, open a new evaluation stack frame.
Once all arguments have been collected (matching closing paren
was read), evaluate current stack frame: push replacement text
back to input, substituting evaluated arguments for each
occurrence of $*n*.
If quoted text appears, unquote and push to evaluation stack
(if in a macro call) or emit to output (otherwise), but do
not push back to input. This way upon each evaluation, one
level of quotes (and only one) is removed.

```text
define(STDIN,0)
define(STDOUT,1)
define(getc,getcf(`STDIN'))
define(putc,putcf($1,`STDOUT'))
putc(x)                        // emits putcf(x,1)
putc(getc())                   // emits putcf(getcf(0),1)
define(incr, $1:=$1+1)
incr(x)                        // emits x:=x+1
define(cat,$1$2$3$4$5$6$7$8$9)
cat(Foo,Bar,Baz)               // emits FooBarBaz
define(op,a$1b)
op, op(), op(+)                // emits ab, ab, a+b
define(a,(b,c))
a                              // emits (b,c) (comma within parens)

define(def, `define($1,$2)')   // must quote replacement
def(foo, bar baz)
foo                            // emits bar baz
define(x, y)
define(x, z)                   // defines y (!) to be z
                               // (x also evals to z, via y)
define(`x',z)                  // redefines x

define(sqr, $1*$1)             // careful about precedence:
sqr(x+1)                       // use parens or get x+1*x+1

define(length,`ifelse($1,,0,`expr(1+length(substr($1,1)))')')
define(reverse,`ifelse($1,,,`reverse(substr($1,1)) substr($1,0,1)')')
```

**Exercise 8-14.** Tracing the macro call `putc(getc())`
assuming the definitions above. The diagram shows the
progression of the evaluation stack and the push backs
in-between.

```text
call:              getc()            STDIN()           putc($1)           STDOUT()
unput:          getcf(STDIN)            0        putcf(getcf(0),STDOUT)       |
output:              |                  |                  |          putcf(getcf(0),1)
  |                  |                  |                  |                  |
  |                  | STDIN            |                  |                  |
e | getc             | 0                |                  |                  |
m | getcf(STDIN)     | ---------------- |                  |                  |
p | ---------------- | getcf(           | getcf(0)         |                  |
t | putc             | putc             | putc             | STDOUT           |
y | putcf($1,STDOUT) | putcf($1,STDOUT) | putcf($1,STDOUT) | 1                | (empty)
= | ================ | ================ | ================ | ================ | =======
```

The **expr** built-in is implemented by a simple
*recursive descent* parser and evaluator; it lives
in its own [evalint.c](../src/evalint.c) file.

**Exercise 8-25.** The built-in `trace(on|off)` turns tracing
on or off; when on, each invocation of a macro or built-in is
dumped to stderr. I've also added a built-in `dumpdefs` that
dumps all definitions to stderr.

**Exercise 8-28.** The macro `rpt(s,n)` shall evaluate to
`s(1)`, `s(2)`, ... `s(n)` — below is a solution. Unless
frequently used, another built-in is hardly worth the effort
and the dreaded feature creap.

```text
define(`rpt', `ifelse($2,0,,`rpt(`$1',expr($2-1))' `$1'($2))')
define(`sqr',`expr(($1)*($1))')
rpt(`sqr',5)           // should emit 1 4 9 16 25
```

**Exercise 8-29.** The new built-in `include(fn)` shall
expand to the contents of the file `fn` and the included
text shall also be scanned for macros. One idea is to read
the entire file `fn` into the push back buffer (must reverse).
More economical on memory is a nested call to `expand()` but
we can only do it after the eval stack has been popped.
The latter approach has been implemented.
There is no guard against a file including itself; two
appraoches that come to mind are: (1) limit the inclusion
depth, as done in the **include** tool, or (2) keeping a
list of active files and refusing to include an active file.

It is instructive to compare with Jon Bentley's M1 macro
processor (written in AWK) and the still common M4 macro
processor, which traces its origins to the earlier Fortran
edition of the *Software Tools* book. See [m1.pdf](m1.pdf)
and [m4.pdf](m4.pdf) for local copies of the relevant papers.

|  |define|macro|m4|m1|
|--:|:---:|:---:|:---:|:---:|
|unit of expansion:|identifier|identifier|identifier|@name@|
|evaluation:|late|early|early|late|
|rescanning:|yes|yes|yes|yes|
|arguments:|no|$1 $2 ...|$1 $2 ...|no|

## The End

The book ends with the macro processor, making for a
remarkably complete toolset. To review by book chapter:

1. Getting Started: *copy*, *count*, *detab*
2. Filters: *entab*, *overstrike*, *compress*, *expand*,
   *echo*, *translit*
3. Files: *compare*, *include*, *concat*, *print*, *makecopy*, *archive*
4. Sorting: *sort*, *unique*, *common*, *kwic*, *unrotate*
5. Text Patterns: *find*, *change*
6. Editing: *edit*
7. Formatting: *format*
8. Macro Processing: *define*, *macro*

Tools in the book but not implemented here:
*overstrike*, *compress*, *expand*, *archive*,
*kwic* and *unrotate*, *format*.
Tools implemented here but not in the book: *shuffle*.

The only thing that is really missing is a simple **shell**.
On the other hand, all tools presented in the book also
exist in any Unix system today, some using different names.
Therefore in real life, you would probably want to stick to
the [GNU tools][coreutils] or use [BusyBox][busybox] or
a similar set of proven tools and use the book to gain
insight into and appreciation of those tools.

[busybox]: https://busybox.net/
[coreutils]: https://www.gnu.org/software/coreutils/

Here are a few ideas for improvements:

- count: an option to count UTF-8 characters
- print: parameters for offset and count
- sort: acccept multiple file arguments
- unique: options -d and -f (as in sort)
- find, change: predefined classes: %u %l %w etc.
  (for upper, lower, alnum, etc.)
- find, change: word boundary zero-width pat elem
- edit: limit size of undo stack
- macro: hold expansion of name in define(name,stuff),
  forget(name), ifdef(name, ...) – would add to usability,
  but that is my personal opinion.

## One Last Remark

There are many routines for scanning text. In the book,
they have signatures like `int scanint(text, int *pindex)`,
that is, they take a pointer to the current index into the
text, they update this index an return the scanned value.
This interface is convenient to use, but the index pointer
is somewhat ugly and there is no straightforward way to
signal a syntax error.

Another scheme that I have been using for a long time is
to return the number of characters scanned like this:
`size_t scanint(text, int *pvalue)`. Now a return of `0`
can easily signal an error, but it tends to be somewhat
less convenient to use: the returned length must be added
to the text pointer for subsequent scans, and the value
cannot be used in compound expressions because it always
goes to a variable.
