#!/bin/sh

libtoolize --copy &&
aclocal &&
autoconf &&
automake --add-missing --copy &&
rm -rf autom4te.cache
