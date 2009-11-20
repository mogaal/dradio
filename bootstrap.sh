#!/bin/sh

rm -f \
aclocal.m4 \
config.h \
config.h.in \
config.log \
config.status \
config.h.in~ \
configure \
missing \
install-sh \
stamp-h1 \
Makefile \
Makefile.in \
src/Makefile \
src/Makefile.in \
doc/Makefile \
doc/Makefile.in

rm -rf autom4te.cache

autoreconf -fi
