include ../Make.defines

all:	${LIB_OBJS} ${LIBP2P_OBJS}
		ar rv ${LIBUNP_NAME} $?
		${RANLIB} ${LIBUNP_NAME}
	
clean:
		rm -f ${PROGS} ${CLEANFILES}
