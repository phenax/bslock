# bslock - simple screen locker
# See LICENSE file for copyright and license details.

include config.mk

SRC = slock.c ${COMPATSRC}
OBJ = ${SRC:.c=.o}

all: clean options bslock

options:
	@echo bslock build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk arg.h util.h

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

bslock: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f bslock ${OBJ} slock-${VERSION}.tar.gz
	@rm -f config.h

dist: clean
	@echo creating dist tarball
	@mkdir -p slock-${VERSION}
	@cp -R LICENSE Makefile README slock.1 config.mk \
		${SRC} explicit_bzero.c config.def.h arg.h util.h slock-${VERSION}
	@tar -cf slock-${VERSION}.tar slock-${VERSION}
	@gzip slock-${VERSION}.tar
	@rm -rf slock-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f bslock ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/bslock
	@chmod u+s ${DESTDIR}${PREFIX}/bin/bslock
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" <bslock.1 >${DESTDIR}${MANPREFIX}/man1/bslock.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/bslock.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/bslock
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/bslock.1

.PHONY: all options clean dist install uninstall
