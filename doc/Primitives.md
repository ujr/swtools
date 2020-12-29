# Primitives

The book's tools are built on top of the primitives presented
below. My C implementation remaps them to (mostly obvious)
counterparts in the C standard library.

## Book Primitives

These are essentially a transcription from Pascal to C.

`bool getarg(int n, char buf[], int maxbuf);`

Write up to *maxbuf* chars (including `ENDSTR`) of the *n*th
command line argument into the array *buf*. First argument
on the command line is number one. No error is reported if
the argument string is truncated. Return `true` if the
argument is present, otherwise `false`.

`int nargs(void);`

Get the number of command line arguments that invoked
the program and thus are available to `getarg()`. Return
an integer greater than or equal to zero.

`int create(const char *name, int mode);`

Associate a file descriptor with the named file. If the file
already exists, truncate it to length zero, otherwise create
it as a new zero-length file. The only sensible value for
*mode* is `IOWRITE`. The file remains under control of the
returned file descriptor until it is closed or the program
terminates.

Return a file descriptor suitable for use with subsequent
calls to `putcf`, `putstr`, `seek`, or `close`; or return
`IOERROR` if the file cannot be accessed as desired, for
any reason.

`int open(const char *name, int mode);`

Associate a file descriptor with the named file for the
type of access specified by *mode*. Legitimate modes are
`IOREAD` for read access and `IOWRITE` for write access.
File access after `open` starts at the first character
of the file. The file remains under control of the returned
file descriptor until it is closed or the program terminates.

Return a file descriptor for use with subsequent calls to
`getcf`, `getline`, `putcf`, `putstr`, `seek`, or `close`;
or return `IOERROR` if the file cannot be accessed as desired,
for any reason.

`void close(int fd);`

Release the file descriptor and any associated resources
for a file opened by `open` or `create`.

`int getch(int *c);`

Read at most one character from standard input; if there are
no more characters available, return `ENDFILE`; if the input
is at end-of-line, return `NEWLINE` and advance to the beginning
of the next line; otherwise return the next input character.
The return value is also written to `*c`.

**Note:** this is `getc` in the book but was renamed to avoid
conflict with *stdio.h*; putting a function's return value also
into an out argument is more natural in Pascal than in C.

`int getcf(int *c, int fd);`

This is the same as `getch` but reading from the file descriptor
*fd* instead of from standard input.

`bool getline(char *buf, int bufsize, int fd);`

Read at most one line of text from the specified file
descriptor *fd*. The characters are written into *buf*
up to and including the terminating `NEWLINE`, and
`ENDSTR` is then appended to terminate the string.
No more than *bufsize*-1 characters are returned,
so a line of length *bufsize*-1 that does not end
with `NEWLINE` has been truncated.

Return `true` if a line is successfully obtained;
`false` implied end of file.

`void putch(int c);`

Write the character *c* to the standard output `STDOUT`.
If *c* is `NEWLINE` an appropriate end-of-line condition
is generated.

**Note:** this is `putc` in the book but was renamed
to avoid conflict with *stdio.h*.

`void putcf(int c, int fd);`

This is the same as `putch` but writing to the given
file descriptor *fd* instead of to the standard output.

`void putstr(const char *s, int fd);`

Write the characters in *s* up to but excluding the
terminating `ENDSTR` to the file specified by *fd*.
An unsuccessful write may or may not cause a warning
message or early termination of the program.

`void error(const char *msg);`

Write the given string *msg* to a highly visible place,
such as the user's terminal, then perform an abnormal exit.

`void message(const char *msg);`

Write the given string *msg* to a highly visible place,
such as the user's terminal, then continue execution.

`void remove(const char *name);`

Discard the named file so that a subsequent call to
`open` with the same name will fail and a subsequent
call to `create` with the same name will create a
new instance of the file.

`void seek(int pos, int fd);`

Position the file associated with *fd* so that a
subsequent read or write access will be from the
given position.

## Remapping

All of the primitives and the constants can be readily
expressed in terms of the C standard library:

Constants: `'\0'` for `ENDSTR`, `'\n'` for `NEWLINE`,
`EOF` for `ENDFILE`, `-1` for `IOERROR`, `"r"` for `IOREAD`,
`"w"` for `IOWRITE`.

Note that we use *stdio.h* and thus a file pointer `fp`
instead of a file descriptor `fd`.

|Book|C|Notes|
|----|-|-----|
|`nargs()`|`argc - 1`|from `main()`|
|`getarg(n)`|`argv[n]`|from `main()`|
|`create(name)`|`fopen(name,"w")`|using *stdio.h*|
|`open(name,IOREAD)`|`fopen(name,"r")`|
|`open(name,IOWRITE)`|`fopen(name,"w")`|
|`close(fd)`|`fclose(fp)`|
|`getch(&c)`|`getchar()`|giving up on out arg|
|`getcf(&c,fd)`|`fgetc(fp)`|giving up on out arg|
|`getline(buf,max,fd)`|`getline(sb,fp)`|using *strbuf.h*|
|`putch(c)`|`putchar(c)`|
|`putcf(c,fd)`|`fputc(c,fp)`|
|`putstr(s,fd)`|`fputs(s,fp)`|
|`error(msg)`|`printf` + `longjmp`|could also `abort()`|
|`message(msg)`|`printf`|
|`remove(fn)`|`remove(fn)`|from *stdio.h*|
|`seek(pos,fd)`|`fseek(fp,ofs,whence)`|from *stdio.h*|
