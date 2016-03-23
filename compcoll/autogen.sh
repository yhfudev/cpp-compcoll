#!/bin/sh

./autoclean.sh

rm -f configure

rm -f Makefile.in

rm -f config.guess
rm -f config.sub
rm -f install-sh
rm -f missing
rm -f depcomp

if [ 0 = 1 ]; then
autoscan
else

touch NEWS
touch README
touch AUTHORS
touch ChangeLog
touch config.h.in

libtoolize --force --copy --install --automake
aclocal
automake -a -c
autoconf
autoreconf -f -i -Wall,no-obsolete
autoreconf -f -i -Wall,no-obsolete

LIBCHSETDET_CFLAGS=-I../../cpp-libucd/include/ \
LIBCHSETDET_LIBS="-L../../cpp-libucd/src/ -lucd" \
    ./configure --enable-static --enable-shared --enable-debug --with-iconv

make clean
make

fi
