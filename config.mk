# cfreq - multi-threaded character frequency counter
VERSION = 1.0.0

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# includes and libraries
INCS =
LIBS = -lpthread

# optimizations 
OPTS = -O2

# debug defines and 
#DDEFS = -DCF_ASSERT -DCF_LOG
#DCFLAGS = -fsanitize=address -fsanitize=undefined -g
#DLDFLAGS = -fsanitize=address -fsanitize=undefined

# flags
CPPFLAGS =
CFLAGS   = -std=c99 -Wpedantic -Wall -Wextra ${DDEFS} ${DCFLAGS} ${OPTS} ${INCS} ${CPPFLAGS}
LDFLAGS  = ${LIBS} ${DLDFLAGS}

# compiler and linker
CC = gcc
