# [PROGRAM/LIBRARY] - [BRIEF DESCRIPTION]
# See [header file here] for copyright and licence details.

include config.mk

SRC = src/cfalloc.c src/cferror.c src/cfreq.c src/cfstate.c
OBJ = ${SRC:.c=.o}
BIN = cfreq

all: options ${BIN}

options:
	@echo ${BIN} build options:
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "LDFLAGS = ${LDFLAGS}"
	@echo "CC      = ${CC}"

src/%.o : src/%.c
	${CC} -c ${CFLAGS} $< -o $@

${OBJ}: config.mk

${BIN}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f ${BIN} ${OBJ} ${BIN}-${VERSION}.tar.gz

dist: clean
	mkdir -p ${BIN}-${VERSION}
	cp -r COPYING Makefile README config.mk \
		${BIN}.1 ${SRC} ${BIN}-${VERSION}
	tar -cf ${BIN}-${VERSION}.tar ${BIN}-${VERSION}
	gzip ${BIN}-${VERSION}.tar
	rm -rf ${BIN}-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${BIN} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${BIN}
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < ${BIN}.1 | gzip > ${DESTDIR}${MANPREFIX}/man1/${BIN}.1.gz
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/${BIN}.1.gz

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${BIN}\
		${DESTDIR}${MANPREFIX}/man1/${BIN}.1.gz

.PHONY: all options clean dist install unistall
