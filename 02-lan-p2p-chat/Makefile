include ../Make.defines

PROGS = lan_chat-v1 lan_chat-v2 lan_chat-v3 lan_chat-v4

all:    ${PROGS}

lan_chat-v1: lan_chat-v1.o
	${CC} ${CFLAGS} -o $@ lan_chat-v1.o ${LIBS}

lan_chat-v2: lan_chat-v2.o
	${CC} ${CFLAGS} -o $@ lan_chat-v2.o ${LIBS}

lan_chat-v3: lan_chat-v3.o udp-iterate.o
	${CC} ${CFLAGS} -o $@ lan_chat-v3.o udp-iterate.o ${LIBS}

lan_chat-v4: lan_chat-v4.o udp-broadcast.o
	${CC} ${CFLAGS} -o $@ lan_chat-v4.o udp-broadcast.o ${LIBS}

lan_chat-v5: lan_chat-v5.o udp-multicast.o
	${CC} ${CFLAGS} -o $@ lan_chat-v5.o udp-multicast.o ${LIBS}


clean:
	rm -f ${PROGS} ${CLEANFILES}
