include ../Make.defines

PROGS =	lan_chat-v1

all:	${PROGS}

lan_chat-v1:	lan_chat-v1.o
		${CC} ${CFLAGS} -o $@ lan_chat-v1.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}
