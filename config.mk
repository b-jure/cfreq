# cfreq - multi-threaded character frequency counter
VERSION = 1.0.0

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# includes and libraries
INCS =
LIBS =

# optimizations 
OPTS = -O2

# flags
CPPFLAGS =
CFLAGS   = -std=c99 -Wpedantic -Wall -Wextra ${OPTS} ${INCS} ${CPPFLAGS}
LDFLAGS  = ${LIBS}

# compiler and linker
CC = gcc
