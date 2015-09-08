include ../Make.defines

PROGS =	lan_chat

all:	${PROGS}

lan_chat:	lan_chat.o
		${CC} ${CFLAGS} -o $@ lan_chat.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}
