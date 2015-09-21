include ../Make.defines

PROGS = lan_chat-v1 lan_chat-v2 lan_chat-v3 lan_chat-v4

all:    ${PROGS} ${TESTS}

lan_chat-v1: lan_chat-v1.o
	${CC} ${CFLAGS} -o $@ lan_chat-v1.o ${LIBS}

lan_chat-v2: lan_chat-v2.o
	${CC} ${CFLAGS} -o $@ lan_chat-v2.o ${LIBS}

lan_chat-v3: lan_chat-v3.o
	${CC} ${CFLAGS} -o $@ lan_chat-v3.o ${LIBS}

lan_chat-v4: lan_chat-v4.o
	${CC} ${CFLAGS} -o $@ lan_chat-v4.o ${LIBS}


clean:
	rm -f ${PROGS} ${CLEANFILES}
