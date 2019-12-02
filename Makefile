CC = gcc-8
CFLAGS =
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS =

PROGS = run/servidor run/cliente

all: ${PROGS}


run/servidor: servidor.o
	${CC} ${CFLAGS} -o $@ servidor.o ${LIBS}

run/cliente: cliente.o
	${CC} ${CFLAGS} -o $@ cliente.o ${LIBS}


clean:
	rm *.o ${PROGS}
