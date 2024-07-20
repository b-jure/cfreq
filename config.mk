# cfreq - multi-threaded character frequency counter
VERSION = 1.0.0

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# includes and libraries
LIBS = -lpthread

# optimizations 
OPTS = -O2

# debug defines and 
#DDEFS = -DCF_ASSERT -DCF_LOG
#ASANFLAGS = -fsanitize=address -fsanitize=undefined
#DBGFLAGS = ${ASANFLAGS} -g

# flags
CFLAGS   = -std=c99 -Wpedantic -Wall -Wextra ${DDEFS} ${DBGFLAGS} ${OPTS}
LDFLAGS  = ${LIBS} ${ASANFLAGS}

# compiler and linker
CC = gcc

# archiver
AR = ar
ARARGS = rcs
