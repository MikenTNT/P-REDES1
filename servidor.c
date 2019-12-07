/*
 *								S E R V I D O R
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <pthread.h>

#include "servidor.h"


extern int errno;

int FIN = 0; /* Para el cierre ordenado */



/*
 *						M A I N
 *
 * This routine starts the server.
 * It forks, leaving the child to do all the work,
 * so it does not have to be run in the background.
 * It sets up the sockets.
 * It will loop forever, until killed by a signal.
 */
int main(int argc, char *argv[])
{
	int s_TCP, s_UDP;  /* connected socket descriptor */
	int ls_TCP;  /* listen socket descriptor */

	int cc;  /* contains the number of bytes read */

	Lista usuarios;
	Lista canales;

	createEmpty(&usuarios);
	createEmpty(&canales);


	struct sigaction sa = {.sa_handler = SIG_IGN};  /* used to ignore SIGCHLD */

	struct sockaddr_in localaddr_in;  /* for local socket address */
	struct sockaddr_in clientaddr_in;  /* for peer socket address */
	socklen_t addrlen = sizeof(struct sockaddr_in);

	fd_set readmask;
	int numfds, s_mayor;

	char buf[BUFFERSIZE];  /* buf for packets to be read into */

	struct sigaction vec;

	/* Create the listen socket. */
	ls_TCP = socket(AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}

	/* clear out address structures */
	memset((char *)&localaddr_in, 0, addrlen);
	memset((char *)&clientaddr_in, 0, addrlen);

	/* Set up address structure for the listen socket. */
	localaddr_in.sin_family = AF_INET;

	/* The server should listen on the wildcard address,
	 * rather than its own internet address.  This is
	 * generally good practice for servers, because on
	 * systems which are connected to more than one
	 * network at once will be able to have one server
	 * listening on all networks at once.  Even when the
	 * host is connected to only one network, this is good
	 * practice, because it makes the server program more
	 * portable.
	 */
	localaddr_in.sin_addr.s_addr = INADDR_ANY;
	localaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &localaddr_in, addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}

	/* Initiate the listen on the socket so remote users
	 * can connect.  The listen backlog is set to 5, which
	 * is the largest currently supported.
	 */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}

	/* Create the socket UDP. */
	s_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	}

	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *) &localaddr_in, addrlen) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	}

	/* Now, all the initialization of the server is
	 * complete, and any user errors will have already
	 * been detected.  Now we can fork the daemon and
	 * return to the user.  We need to do a setpgrp
	 * so that the daemon will no longer be associated
	 * with the user's control terminal.  This is done
	 * before the fork, so that the child will not be
	 * a process group leader.  Otherwise, if the child
	 * were to open a terminal, it would become associated
	 * with that terminal as its control terminal.  It is
	 * always best for the parent to do the setpgrp.
	 */
	setpgrp();

	switch (fork()) {
		case -1:  /* Unable to fork, for some reason. */
			perror(argv[0]);
			fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
			exit(1);

		case 0:  /* The child process (daemon) comes here. */

			/* Close stdin and stderr so that they will not
			 * be kept open.  Stdout is assumed to have been
			 * redirected to some logging file, or /dev/null.
			 * From now on, the daemon will not report any
			 * error messages.  This daemon will loop forever,
			 * waiting for connections and forking a child
			 * server to handle each one.
			 */
			fclose(stdin);
			fclose(stderr);

			/* Set SIGCLD to SIG_IGN, in order to prevent
			 * the accumulation of zombies as each child
			 * terminates.  This means the daemon does not
			 * have to make wait calls to clean them up.
			 */
			if (sigaction(SIGCHLD, &sa, NULL) == -1) {
				perror("sigaction(SIGCHLD)");
				fprintf(stderr, "%s: unable to register the SIGCHLD signal\n", argv[0]);
				exit(1);
			}

			/* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
			vec.sa_handler = (void *)finalizar;
			vec.sa_flags = 0;
			if (sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
				perror("sigaction(SIGTERM)");
				fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
				exit(1);
			}

			while (!FIN) {
				/* Meter en el conjunto de sockets los sockets UDP y TCP */
				FD_ZERO(&readmask);
				FD_SET(ls_TCP, &readmask);
				FD_SET(s_UDP, &readmask);

				/*
				 * Seleccionar el descriptor del socket que ha cambiado.
				 * Deja una marca en el conjunto de sockets (readmask)
				 */
				if (ls_TCP > s_UDP)
					s_mayor = ls_TCP;
				else
					s_mayor = s_UDP;

				if (0 > (numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL))) {
					if (errno == EINTR) {
						FIN = 1;
						close (ls_TCP);
						close (s_UDP);
						perror("\nFinalizando el servidor. Señal recibida en elect\n ");
					}
				}
				else {
					/* Comprobamos si el socket seleccionado es el socket TCP */
					if (FD_ISSET(ls_TCP, &readmask)) {
						 /* Note that addrlen is passed as a pointer
						 * so that the accept call can return the
						 * size of the returned address.
						 */

						/* This call will block until a new
						 * connection arrives.  Then, it will
						 * return the address of the connecting
						 * peer, and a new socket descriptor, s,
						 * for that connection.
						 */
						s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
						if (s_TCP == -1) exit(1);
						switch (fork()) {
							case -1:  /* Can't fork, just exit. */
								exit(1);
							case 0:  /* Child process comes here. */
								close(ls_TCP); /* Close the listen socket inherited from the daemon. */
								serverTCP(s_TCP, clientaddr_in, &usuarios, &canales);
								exit(0);
							default:  /* Daemon process comes here. */
								/* The daemon needs to remember
								 * to close the new accept socket
								 * after forking the child.  This
								 * prevents the daemon from running
								 * out of file descriptor space.  It
								 * also means that when the server
								 * closes the socket, that it will
								 * allow the socket to be destroyed
								 * since it will be the last close.
								 */
								close(s_TCP);
						}
					} /* De TCP*/

					/* Comprobamos si el socket seleccionado es el socket UDP */
					if (FD_ISSET(s_UDP, &readmask)) {
						/* This call will block until a new
						 * request arrives.  Then, it will
						 * return the address of the client,
						 * and a buf containing its request.
						 * BUFFERSIZE - 1 bytes are read so that
						 * room is left at the end of the buf
						 * for a null character.
						 */
						cc = recvfrom(s_UDP, buf, BUFFERSIZE - 1, 0,
							(struct sockaddr *)&clientaddr_in, &addrlen);
						if (cc == -1) {
							perror(argv[0]);
							printf("%s: recvfrom error\n", argv[0]);
							exit (1);
						}
						/*
						 * Make sure the message received is
						 * null terminated.
						 */
						buf[cc] = '\0';
						serverUDP(s_UDP, buf, clientaddr_in);
					}
				}
			} /* Fin del bucle infinito de atención a clientes */

			/* Cerramos los sockets UDP y TCP */
			close(ls_TCP);
			close(s_UDP);

			printf("\nFin de programa servidor!\n");

		default:  /* Parent process comes here. */
			exit(0);
	}
}


/*
 *						S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int idSoc, struct sockaddr_in clientaddr_in, Lista * usuarios, Lista * canales)
{
	int reqcnt = 0;  /* keeps count of number of requests */
	buffer buf;  /* This example uses TAM_BUFFER byte messages. */
	char hostname[HOSTLEN];  /* remote host's name string */
	char orden[10];
	char arg1[10];
	char arg2[256];
	nick nickName;
	int status;

	/* allow a lingering, graceful close; */
	/* used when setting SO_LINGER */
	struct linger linger;

	/*
	 * Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in),
		hostname, HOSTLEN, NULL, 0, 0);

	/*
	 * The information is unavailable for the remote
	 * host.  Just format its internet address to be
	 * printed out in the logging information.  The
	 * address will be shown in "internet dot format".
	 */
	if (status) {
		/* inet_ntop para interoperatividad con IPv6 */
		if (NULL == inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, HOSTLEN))
			perror(" inet_ntop \n");
	}

	/*
	 * The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	printf("S) Startup from %s port %u at %s",
		hostname, ntohs(clientaddr_in.sin_port), timeString());

	/*
	 * Set the socket for a lingering, graceful close.
	 * This will cause a final close of this socket to wait until all of the
	 * data sent on it has been received by the remote host.
	 */
	linger.l_onoff = 1;
	linger.l_linger = 1;
	if (-1 == setsockopt(idSoc, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger))) {
		printf("S) Connection with %s aborted on error\n", hostname);
		exit(1);
	}


	while (!FIN) {
		if (recv(idSoc, buf, TAM_BUFFER, 0) == -1) {
			printf("S) Connection with %s aborted on error\n", hostname);
			exit(1);
		}

		/* funcion dividir(buf, orden, arg1, arg2) */

		/*
		 * This sleep simulates the processing of the
		 * request that a real server might do.
		 */
		sleep(1);
/*
		if (!strcmp(orden, "NICK")) {
			if(!nickOrd(arg1, usuarios)) {

			}
			strcpy(nickName, arg1);
		}
		else if (!strcmp(orden, "USER"))
			userOrd(arg1, arg2, usuarios);
		else if (!strcmp(orden, "PRIVMSG"))
			mensajesOrd(arg1, arg2, usuarios, canales);
		else if (!strcmp(orden, "JOIN"))
			joinOrd(nickName, arg1, usuarios, canales);
		else if (!strcmp(orden, "PART"))
			partOrd(nickName, arg1, arg2, canales);
		else if (!strcmp(orden, "QUIT"))
			quitOrd(arg1, usuarios, canales);
		else
			fprintf(stderr, "Funcion no existente\n");

*/


		/* Send a response back to the client. */
		if (send(idSoc, buf, TAM_BUFFER, 0) != TAM_BUFFER) {
			printf("S) Connection with %s aborted on error\n", hostname);
			exit(1);
		}
	}

	/*
	 * The loop has terminated, because there are no
	 * more requests to be serviced.  As mentioned above,
	 * this close will block until all of the sent replies
	 * have been received by the remote host.  The reason
	 * for lingering on the close is so that the server will
	 * have a better idea of when the remote has picked up
	 * all of the data.  This will allow the start and finish
	 * times printed in the log file to reflect more accurately
	 * the length of time this connection was used.
	 */
	close(idSoc);


	/*
	 * The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	printf("S) Completed %s port %u, %d requests, at %s\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, timeString());
}


/*
 *						S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverUDP(int idSoc, char * buf, struct sockaddr_in clientaddr_in)
{
	struct in_addr reqaddr;  /* for requested host's address */
	int nc;

	struct addrinfo hints, *res;

	int addrlen = sizeof(struct sockaddr_in);

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;

	/*
	 * Esta función es la recomendada para la compatibilidad con IPv6
	 * gethostbyname queda obsoleta.
	 */
	if (getaddrinfo(buf, NULL, &hints, &res) != 0) {
		reqaddr.s_addr = ADDRNOTFOUND;
	}
	else {
		/* Copy address of host into the return buf. */
		reqaddr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}

	freeaddrinfo(res);

	nc = sendto(idSoc, &reqaddr, sizeof(struct in_addr), 0,
		(struct sockaddr *)&clientaddr_in, addrlen);

	if (nc == -1) {
		perror("serverUDP");
		printf("%s: sendto error\n", "serverUDP");
		return;
	}
}


void finalizar()
{
	FIN = 1;
}


int nickOrd(nick nickName, Lista * usuarios)
{
	tipoPosicion celda;
	datosUsuario * datosBD;
	datosUsuario * datosInsertar;

	if ((datosInsertar = (datosUsuario *)malloc(sizeof(datosUsuario))) == NULL) {
		perror("memoriaUsuario");
		exit(151);
	}

	strcpy(datosInsertar->nickName, nickName);
	strcpy(datosInsertar->nombreReal, "");

	celda = firstPosition(usuarios);
	while (celda != NULL) {
		if ((datosBD = (datosUsuario *)celda->elemento) != NULL) {
			if (!strcmp(datosInsertar->nickName, datosBD->nickName)) {
				fprintf(stderr, ":%s %u %s:Nickname is already in use.", "h", ERR_NICKNAME, nickName);
				return ERR_NICKNAME;
			}
		}
		celda = nextPosition(usuarios, celda);
	}

	insertAt(usuarios, datosInsertar, lastPosition(usuarios));

	return 0;
}


int userOrd(nick nickName, nombre nombreReal, Lista * usuarios)
{
	tipoPosicion celda;
	datosUsuario * datosBD;
	int existe = 0;

	celda = firstPosition(usuarios);
	while (celda != NULL) {
		datosBD = (datosUsuario *)celda->elemento;
		if (!strcmp(nickName, datosBD->nickName)) {
			strcpy(datosBD->nombreReal, nombreReal);
			existe = 1;
			break;
		}
		celda = nextPosition(usuarios, celda);
	}

	if (!existe) {
		fprintf(stderr, ":%s %u %s:You may not reregister.", "h", ERR_ALREADYREGISTRED, nickName);
		return ERR_ALREADYREGISTRED;
	}

	return 0;
}


int mensajesOrd(char * receptor, char * mensaje, Lista * usuarios, Lista * canales)
{
	fprintf(stderr, ":%s %u %s:No such nick/channel.", "h", ERR_NOSUCHNICK , receptor);
	return ERR_NOSUCHNICK;
}


int joinOrd(nick nickName, nombre canal, Lista * usuarios, Lista * canales)
{
	tipoPosicion celda;
	datosCanal * datosBD;
	datosCanal * datosInsertar;
	nick * idUsuario;
	int existe = 0;



	celda = firstPosition(canales);
	while (celda != NULL) {
		datosBD = (datosCanal *)celda->elemento;
		if (!strcmp(canal, datosBD->nombreCanal)) {
			if ((idUsuario = (nick *)malloc(sizeof(nick))) == NULL) {
				perror("memoriaNick");
				exit(151);
			}
			strcpy(*idUsuario, nickName);
			insertAt(datosBD->nicks, idUsuario, lastPosition(datosBD->nicks));
			existe = 1;
			break;
		}
		celda = nextPosition(canales, celda);
	}

	if (!existe) {
		if ((datosInsertar = (datosCanal *)malloc(sizeof(datosCanal))) == NULL) {
			perror("memoriaCanal");
			exit(151);
		}

		if ((idUsuario = (nick *)malloc(sizeof(nick))) == NULL) {
			perror("memoriaNick");
			exit(151);
		}
		strcpy(*idUsuario, nickName);
		insertAt(datosInsertar->nicks, idUsuario, lastPosition(datosInsertar->nicks));

		insertAt(canales, datosInsertar, lastPosition(canales));
	}

	return 0;
}

int partOrd(nick nickName, nombre canal, char * mensaje, Lista * canales)
{
	tipoPosicion celda;
	tipoPosicion celdaNicks;
	datosCanal * datosBD;
	nick * datosNicks;
	int existeNick = 0;
	int existeCanal = 0;


	celda = firstPosition(canales);
	while (celda != NULL) {
		datosBD = (datosCanal *)celda->elemento;
		if (!strcmp(canal, datosBD->nombreCanal)) {
			existeCanal = 1;
			celdaNicks = firstPosition(datosBD->nicks);
			while (celdaNicks != NULL) {
				datosNicks = (nick *)celdaNicks->elemento;
				if (!strcmp(nickName, *datosNicks)) {
					free(datosNicks);
					removeAt(datosBD->nicks, celdaNicks);
					existeNick = 1;
					break;
				}
				celdaNicks = nextPosition(datosBD->nicks, celdaNicks);
			}

			if (!existeNick) {
				fprintf(stderr, ":%s %u %s:The user isn't registered at this chanel.", "h", ERR_ALREADYREGISTRED, nickName);
				return ERR_NOREGISTEREDINCHANEL;
			}

			if (isEmpty(datosBD->nicks)) {
				destroy(datosBD->nicks);
				free(datosBD);
				removeAt(canales, celda);
			}
		}
		celda = nextPosition(canales, celda);
	}

	if (!existeCanal) {
		fprintf(stderr, ":%s %u %s:No such channel.", "h", ERR_NOSUCHCHANNEL, canal);
		return ERR_NOSUCHCHANNEL;
	}

	return 0;
}


int quitOrd(char * mensaje, Lista * usuarios, Lista * canales)
{
	return 0;
}


/* @TODO */
/*
	-funcion dividir candena
	-(opcional) imprimir datos de listas
	-funcion mensajes
	-funcion quit
	-UDP
 */
