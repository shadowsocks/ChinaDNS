#! /bin/sh

aclocal && \
    automake --add-missing --force-missing --include-deps && \
    autoconf
