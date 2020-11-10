#!/bin/sh
# Testing edit by comparing against expected output

trap 'rm -f "$TMPFILE"' EXIT
TMPFILE=$(mktemp) || exit 1
echo "Using temp file $TMPFILE"

RED="\033[31m"
RESET="\033[0m"
error() { printf "${RED}$* FAILED${RESET}\n"; }

### Testing some boundary behaviour (ex 6-5)
cat << EOT > $TMPFILE
three
three
two
two
three
two
one
0
EOT
bin/quux edit << EOT | cmp $TMPFILE || error "Test 1"
0i
one
two
three
.
m0
1p
1m.-1p
\$p
\$m.-1p
2m\$
1,\$p
1,\$d
\$=
EOT

# Append, Change, Translit, Substitute, Join
cat << EOT > $TMPFILE
4
CC-B1-B2-AA
1
EOT
bin/quux edit << EOT | cmp $TMPFILE || error "Test 2"
H
a
aa
bb
cc
.
g/^/m0
2c
b1
b2
.
\$=
1,\$t/a-z/A-Z/
1,\$-1s/$/-/
1,\$jp
\$=
EOT

# Global/RE/Print (grep)
cat << EOT > $TMPFILE
bar
bar
bazaar
bar
baz
bazaar
EOT
bin/quux edit << EOT | cmp $TMPFILE || error "Test 3"
a
foo
bar
baz
bazaar
quux
.
1,/baz/g/ar/p
1,\$g/ar/p
1,\$g/ba/p
EOT

echo "(silent means no errors)"
