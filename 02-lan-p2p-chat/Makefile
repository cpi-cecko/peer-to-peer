include ../Make.defines

PROGS = lan_chat-v1 lan_chat-v2
TESTS = test-is-valid-subnet-address

all:    ${PROGS} ${TESTS}

lan_chat-v1: lan_chat-v1.o
	${CC} ${CFLAGS} -o $@ lan_chat-v1.o ${LIBS}

lan_chat-v2: lan_chat-v2.o
	${CC} ${CFLAGS} -o $@ lan_chat-v2.o ${LIBS}

test-is-valid-subnet-address: test-is-valid-subnet-address.o
	${CC} ${CFLAGS} -o $@ test-is-valid-subnet-address.o ${LIBS}


clean:
	rm -f ${PROGS} ${TESTS} ${CLEANFILES}
