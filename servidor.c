/*
** Fichero: servidor.c
** Autores:
** Carlos Manjón García DNI 70908545M
** Miguel Sánchez González DNI 70921138V
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>

#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>

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
	int ls_UDP;  /* connected socket descriptor */
	int ls_TCP;  /* listen socket descriptor */

	int cc;  /* contains the number of bytes read */
	int s_mayor;

	/* Listas enlazadas de la base de datos. */
	List usuarios;
	List canales;
	createEmpty(&usuarios);
	createEmpty(&canales);

	/* Datos para el uso de los hilos. */
	int nHilos = 0;
	pthread_t hilos[MAXCLIENTS];
	DatosHiloServer *datosHilo;

	/* Handlers for program fidelity. */
	struct sigaction sa = {.sa_handler = SIG_IGN};  /* used to ignore SIGCHLD */
	struct sigaction vec;

	fd_set readmask;

	/* Socket descriptors. */
	struct sockaddr_in localaddr_in;  /* for local socket address */
	socklen_t addrlen = sizeof(struct sockaddr_in);
	/* clear out address structures */
	memset((char *)&localaddr_in, 0, addrlen);

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
	/* Set up address structure for the listen socket. */
	localaddr_in.sin_family = AF_INET;


	/* Create the listen socket. */
	ls_TCP = socket(AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (struct sockaddr *)&localaddr_in, addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}

	/*
	 * Initiate the listen on the socket so remote users
	 * can connect.  The listen backlog is set to 5, which
	 * is the largest currently supported.
	 */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}

	/* Create the socket UDP. */
	ls_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (ls_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	}

	/* Bind the server's address to the socket. */
	if (bind(ls_UDP, (struct sockaddr *)&localaddr_in, addrlen) == -1) {
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

	/* Lanzamos el daemon del servidor y cerramos el programa padre. */
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
			// fclose(stdin);
			// fclose(stderr);

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
				FD_SET(ls_UDP, &readmask);

				/*
				 * Seleccionar el descriptor del socket que ha cambiado.
				 * Deja una marca en el conjunto de sockets (readmask)
				 */
				s_mayor = (ls_TCP > ls_UDP) ? ls_TCP : ls_UDP;

				/* Comprobamos el tipo de socket leido. */
				if ((select(s_mayor + 1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
					if (errno == EINTR) {
						FIN = 1;
						close (ls_TCP);
						close (ls_UDP);
						perror("\nFinalizando el servidor. Señal recibida en select\n");
					}
				}


				/* Comprobamos si el socket seleccionado es el socket TCP */
				if (FD_ISSET(ls_TCP, &readmask)) {
					/* Note that addrlen is passed as a pointer
					 * so that the accept call can return the
					 * size of the returned address.
					 */
					if (nHilos < MAXCLIENTS) {
						if ((datosHilo = (DatosHiloServer *)malloc(sizeof(DatosHiloServer))) == NULL) {
							perror("memoriaHilo");
							exit(151);
						}
						if ((datosHilo->addr = (struct sockaddr_in *)calloc(1, sizeof(struct sockaddr_in))) == NULL) {
							perror("memoriaHiloAddr");
							exit(151);
						}

						/* This call will block until a new
						 * connection arrives.  Then, it will
						 * return the address of the connecting
						 * peer, and a new socket descriptor, s,
						 * for that connection.
						 */
						datosHilo->usuarios = &usuarios;
						datosHilo->canales = &canales;
						datosHilo->idSoc = accept(ls_TCP, (struct sockaddr *)datosHilo->addr, &addrlen);
						if (datosHilo->idSoc == -1)  exit(1);


						/* handler, atributos del thread, función, argumentos de la función */
						if (pthread_create(&hilos[nHilos], NULL, &serverTCP, (void *)datosHilo) != 0) {
							printf("Error al crear el Thread\n");
							return(-1);
						}

						nHilos++;
					}
				} else if (FD_ISSET(ls_UDP, &readmask)) {
					/* Comprobamos si el socket seleccionado es el socket UDP */
					/* This call will block until a new
					 * request arrives.  Then, it will
					 * return the address of the client,
					 * and a buf containing its request.
					 * BUFFERSIZE - 1 bytes are read so that
					 * room is left at the end of the buf
					 * for a null character.
					 */
					if (nHilos < MAXCLIENTS) {
						if ((datosHilo = (DatosHiloServer *)malloc(sizeof(DatosHiloServer))) == NULL) {
							perror("memoriaHilo");
							exit(151);
						}
						if ((datosHilo->addr = (struct sockaddr_in *)calloc(1, sizeof(struct sockaddr_in))) == NULL) {
							perror("memoriaHiloAddr");
							exit(151);
						}

						/* This call will block until a new
						* connection arrives.  Then, it will
						* return the address of the connecting
						* peer, and a new socket descriptor, s,
						* for that connection.
						*/
						datosHilo->usuarios = &usuarios;
						datosHilo->canales = &canales;
						datosHilo->idSoc = ls_UDP;

						cc = recvfrom(ls_UDP, datosHilo->buff, TAM_BUFFER - 1, 0, (struct sockaddr *)datosHilo->addr, &addrlen);
						if (cc == -1) {
							perror(argv[0]);
							printf("%s: recvfrom error\n", argv[0]);
							exit (1);
						}
						/*
						* Make sure the message received is
						* null terminated.
						*/
						datosHilo->buff[cc] = '\0';

						/* handler, atributos del thread, función, argumentos de la función */
						if (pthread_create(&hilos[nHilos], NULL, &serverUDP, (void *)datosHilo) != 0) {
							printf("Error al crear el Thread\n");
							return(-1);
						}

						nHilos++;
					}
				}
			} /* Fin del bucle infinito de atención a clientes */

			/* Cerramos los sockets UDP y TCP */
			close(ls_TCP);
			close(ls_UDP);

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
void * serverTCP(void * datos)
{
	/* Puntero de los argumentos. */
	DatosHiloServer *datosHilo = (DatosHiloServer *)datos;

	buffer buf;  /* This example uses TAM_BUFFER byte messages. */
	ordenes orden;
	arg_1 arg1;
	arg_2 arg2;
	nick nickName;

	int salir = 0;

	char hostname[HOSTLEN];  /* remote host's name string */

	/* allow a lingering, graceful close; */
	/* used when setting SO_LINGER */
	struct linger linger;

	/*
	 * Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 * The information is unavailable for the remote
	 * host.  Just format its internet address to be
	 * printed out in the logging information.  The
	 * address will be shown in "internet dot format".
	 */
	if (getnameinfo((struct sockaddr *)&(datosHilo->addr), sizeof(datosHilo->addr),
		hostname, HOSTLEN, NULL, 0, 0))
	{
		/* inet_ntop para interoperatividad con IPv6 */
		if (NULL == inet_ntop(AF_INET, &(datosHilo->addr->sin_addr), hostname, HOSTLEN))
			perror(" inet_ntop \n");
	}

	/*
	 * The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	fprintf(stderr, "S) Startup from %s port %u at %s",
			hostname, ntohs(datosHilo->addr->sin_port), timeString());

	/*
	 * Set the socket for a lingering, graceful close.
	 * This will cause a final close of this socket to wait until all of the
	 * data sent on it has been received by the remote host.
	 */
	linger.l_onoff = 1;
	linger.l_linger = 1;
	if (-1 == setsockopt(datosHilo->idSoc, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger))) {
		fprintf(stderr , "S) Connection with %s aborted on error 1\n", hostname);
		exit(1);
	}


	strcpy(nickName, "");

	while (!salir) {
		if (recv(datosHilo->idSoc, buf, TAM_BUFFER, 0) == -1) {
			fprintf(stderr , "S) Connection with %s aborted on error 2\n", hostname);
			exit(1);
		}

		/* Dividimos la cadena del buffer. */
		dividirBuffer(&buf, &orden, &arg1, &arg2);

		/*
		 * This sleep simulates the processing of the
		 * request that a real server might do.
		 */
		sleep(1);


		if (!strcmp(orden, "NICK")) {
			switch (nickOrd(arg1, datosHilo->addr, datosHilo->idSoc, datosHilo->usuarios)) {
				case 0:
					strcpy(nickName, arg1);
					sprintf(buf, "%sUser %s registered.", "server: ", nickName);
					break;
				case ERR_NICKNAME:
					sprintf(buf, "%s:%s %u %s:Nickname is already in use.", "server: ", "h", ERR_NICKNAME, arg1);
					break;
				default:
					sprintf(buf, "%s:%s:Unknown error.", "server: ", "h");
					break;
			}
		}
		else if (!strcmp(orden, "USER")) {
			switch (userOrd(nickName, arg1, arg2, datosHilo->usuarios)) {
				case 0:
					sprintf(buf, "%sReal name %s registered.", "server: ", arg2);
					break;
				case ERR_ALREADYREGISTRED:
					sprintf(buf, "%s:%s %u %s:You may not reregister.", "server: ", "h", ERR_ALREADYREGISTRED, arg1);
					break;
				case ERR_NOVALIDUSER:
					sprintf(buf, "%s:%s %u %s:This isn't your nickname.", "server: ", "h", ERR_NOVALIDUSER, arg1);
					break;
				case ERR_NOREGISTERED:
					sprintf(buf, "%sYou may registrer a NICK first.", "server: ");
					break;
				default:
					sprintf(buf, "%s:%s:Unknown error.", "server: ", "h");
					break;
			}
		}
		else if (!strcmp(orden, "PRIVMSG")) {
			switch (mensajesOrd(nickName, arg1, arg2, datosHilo->usuarios, datosHilo->canales)) {
				case 0:
					sprintf(buf, "%sMessage %s sended to %s.", "server:", arg2, arg1);
					break;
				case ERR_NOSUCHNICK:
					sprintf(buf, "%s:%s %u %s:No such nick/channel.", "server: ", "h", ERR_NOSUCHNICK , arg1);
					break;
				case ERR_NOREGISTERED:
					sprintf(buf, "%sYou may registrer a NICK first.", "server: ");
					break;
				default:
					sprintf(buf, "%s:%s:Unknown error.", "server: ", "h");
					break;
			}
		}
		else if (!strcmp(orden, "JOIN")) {
			switch (joinOrd(nickName, arg1, datosHilo->canales)) {
				case 0:
					sprintf(buf, "%sJoined to %s chanel.", "server: ", arg1);
					break;
				case ERR_ALREADYREGISTREDINCHANEL:
					sprintf(buf, "%s:%s %u %s:The user is already at this chanel.", "server: ", "h", ERR_ALREADYREGISTREDINCHANEL, nickName);
					break;
				case ERR_NOREGISTERED:
					sprintf(buf, "%sYou may registrer a NICK first.", "server: ");
					break;
				default:
					sprintf(buf, "%s:%s:Unknown error.", "server: ", "h");
					break;
			}
		}
		else if (!strcmp(orden, "PART")) {
			switch (partOrd(nickName, arg1, arg2, datosHilo->canales)) {
				case 0:
					sprintf(buf, "%sExited from %s chanel.", "server: ", arg1);
					break;
				case ERR_NOREGISTEREDINCHANEL:
					sprintf(buf, "%s:%s %u %s:The user isn't registered at this chanel.", "server: ", "h", ERR_NOREGISTEREDINCHANEL, nickName);
					break;
				case ERR_NOSUCHCHANNEL:
					sprintf(buf, "%s:%s %u %s:No such channel.", "server: ", "h", ERR_NOSUCHCHANNEL, arg1);
					break;
				case ERR_NOREGISTERED:
					sprintf(buf, "%sYou may registrer a NICK first.", "server: ");
					break;
				default:
					sprintf(buf, "%s:%s:Unknown error.", "server: ", "h");
					break;
			}
		}
		else if (!strcmp(orden, "QUIT")) {
			quitOrd(nickName, arg1, datosHilo->usuarios, datosHilo->canales);
			sprintf(buf, "%sExited from application", "server: ");
			salir = 1;
		}
		else {
			sprintf(buf, "%sLa funcion no existente\n", "server: ");
		}


		/* Send a response back to the client. */
		if (send(datosHilo->idSoc, buf, TAM_BUFFER, 0) != TAM_BUFFER) {
			fprintf(stderr , "S) Connection with %s aborted on error 3\n", hostname);
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
	close(datosHilo->idSoc);


	/*
	 * The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	fprintf(stderr, "S) Completed %s port %u, at %s",
		hostname, ntohs((datosHilo->addr)->sin_port), timeString());


	/* Liberamos la memoria del hilo. */
	free(datosHilo->addr);
	free(datosHilo);

	return NULL;
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
void * serverUDP(void * datos)
{
	/* Puntero de los argumentos. */
	DatosHiloServer *datosHilo = (DatosHiloServer *)datos;

	nick nickName;
	ordenes orden;
	arg_1 arg1;
	arg_2 arg2;

	buffer buf;
	buffer rtBuf;

	struct in_addr reqaddr;  /* for requested host's address */
	struct addrinfo *res;
	struct addrinfo hints;
	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;

	strcpy(buf, datosHilo->buff);

	/*
	 * Esta función es la recomendada para la compatibilidad con IPv6
	 * gethostbyname queda obsoleta.
	 */
	if (getaddrinfo(rtBuf, NULL, &hints, &res) != 0) {
		reqaddr.s_addr = ADDRNOTFOUND;
	}
	else {
		/* Copy address of host into the return buf. */
		reqaddr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}

	freeaddrinfo(res);


	strcpy(nickName, "");

	/* Dividimos la cadena del buffer. */
	dividirBuffer(&buf, &orden, &arg1, &arg2);

	/*
	 * This sleep simulates the processing of the
	 * request that a real server might do.
	 */
	sleep(1);


	if (!strcmp(orden, "NICK")) {
		switch (nickOrd(arg1, datosHilo->addr, datosHilo->idSoc, datosHilo->usuarios)) {
			case 0:
				strcpy(nickName, arg1);
				sprintf(buf, "%sUser %s registered.", "server: ", nickName);
				break;
			case ERR_NICKNAME:
				sprintf(buf, "%s:%s %u %s:Nickname is already in use.", "server: ", "h", ERR_NICKNAME, arg1);
				break;
			default:
				sprintf(buf, "%s:%s:Unknown error.", "server: ", "h");
				break;
		}
	}
	else if (!strcmp(orden, "USER")) {
		switch (userOrd(nickName, arg1, arg2, datosHilo->usuarios)) {
			case 0:
				sprintf(buf, "%sReal name %s registered.", "server: ", arg2);
				break;
			case ERR_ALREADYREGISTRED:
				sprintf(buf, "%s:%s %u %s:You may not reregister.", "server: ", "h", ERR_ALREADYREGISTRED, arg1);
				break;
			case ERR_NOVALIDUSER:
				sprintf(buf, "%s:%s %u %s:This isn't your nickname.", "server: ", "h", ERR_NOVALIDUSER, arg1);
				break;
			case ERR_NOREGISTERED:
				sprintf(buf, "%sYou may registrer a NICK first.", "server: ");
				break;
			default:
				sprintf(buf, "%s:%s:Unknown error.", "server: ", "h");
				break;
		}
	}
	else if (!strcmp(orden, "PRIVMSG")) {
		switch (mensajesOrd(nickName, arg1, arg2, datosHilo->usuarios, datosHilo->canales)) {
			case 0:
				sprintf(buf, "%sMessage %s sended to %s.", "server:", arg2, arg1);
				break;
			case ERR_NOSUCHNICK:
				sprintf(buf, "%s:%s %u %s:No such nick/channel.", "server: ", "h", ERR_NOSUCHNICK , arg1);
				break;
			case ERR_NOREGISTERED:
				sprintf(buf, "%sYou may registrer a NICK first.", "server: ");
				break;
			default:
				sprintf(buf, "%s:%s:Unknown error.", "server: ", "h");
				break;
		}
	}
	else if (!strcmp(orden, "JOIN")) {
		switch (joinOrd(nickName, arg1, datosHilo->canales)) {
			case 0:
				sprintf(buf, "%sJoined to %s chanel.", "server: ", arg1);
				break;
			case ERR_ALREADYREGISTREDINCHANEL:
				sprintf(buf, "%s:%s %u %s:The user is already at this chanel.", "server: ", "h", ERR_ALREADYREGISTREDINCHANEL, nickName);
				break;
			case ERR_NOREGISTERED:
				sprintf(buf, "%sYou may registrer a NICK first.", "server: ");
				break;
			default:
				sprintf(buf, "%s:%s:Unknown error.", "server: ", "h");
				break;
		}
	}
	else if (!strcmp(orden, "PART")) {
		switch (partOrd(nickName, arg1, arg2, datosHilo->canales)) {
			case 0:
				sprintf(buf, "%sExited from %s chanel.", "server: ", arg1);
				break;
			case ERR_NOREGISTEREDINCHANEL:
				sprintf(buf, "%s:%s %u %s:The user isn't registered at this chanel.", "server: ", "h", ERR_NOREGISTEREDINCHANEL, nickName);
				break;
			case ERR_NOSUCHCHANNEL:
				sprintf(buf, "%s:%s %u %s:No such channel.", "server: ", "h", ERR_NOSUCHCHANNEL, arg1);
				break;
			case ERR_NOREGISTERED:
				sprintf(buf, "%sYou may registrer a NICK first.", "server: ");
				break;
			default:
				sprintf(buf, "%s:%s:Unknown error.", "server: ", "h");
				break;
		}
	}
	else if (!strcmp(orden, "QUIT")) {
		quitOrd(nickName, arg1, datosHilo->usuarios, datosHilo->canales);
		sprintf(buf, "%sExited from application", "server: ");
	}
	else {
		sprintf(buf, "%sLa funcion no existente\n", "server: ");
	}


	if (sendto(datosHilo->idSoc, &reqaddr, sizeof(struct in_addr), 0,
		(struct sockaddr *)datosHilo->addr, sizeof(struct sockaddr_in)) == -1) {
		perror("serverUDP");
		printf("%s: sendto error\n", "serverUDP");
		return NULL;
	}

	return NULL;
}


void finalizar()
{
	FIN = 1;
}


int nickOrd(nick nickName, struct sockaddr_in * clientAddr, int idSoc, List * usuarios)
{
	idPosition posUsuarios;
	datosUsuario * datosUsuarios;

	/* Buscamos el nick. */
	posUsuarios = (isEmpty(usuarios) ? NULL : firstPosition(usuarios));

	while (posUsuarios != NULL) {
		datosUsuarios = (datosUsuario *)getData(usuarios, posUsuarios);

		/* Nick encontrado. */
		if (!strcmp(nickName, datosUsuarios->nickName)) {
			return ERR_NICKNAME;
		}

		if ((posUsuarios = nextPosition(usuarios, posUsuarios)) == lastPosition(usuarios))
			posUsuarios = NULL;
	}

	/* Si no se encuentra el nick. */
	if ((datosUsuarios = (datosUsuario *)malloc(sizeof(datosUsuario))) == NULL) {
		perror("memoriaUsuario");
		exit(151);
	}
	strcpy(datosUsuarios->nickName, nickName);
	strcpy(datosUsuarios->nombreReal, "");
	datosUsuarios->addr = clientAddr;
	datosUsuarios->idSock = idSoc;

	insertAt(usuarios, datosUsuarios, lastPosition(usuarios));

	return 0;
}


int userOrd(nick nickName, nick usuario, nombre nombreReal, List * usuarios)
{
	if (!strcmp(nickName, "")) {
		return ERR_NOREGISTERED;
	}

	if (strcmp(nickName, usuario)) {
		return ERR_NOVALIDUSER;
	}

	idPosition posUsuarios;
	datosUsuario * datosUsuarios;

	/* Buscamos el nick. */
	posUsuarios = (isEmpty(usuarios) ? NULL : firstPosition(usuarios));

	while (posUsuarios != NULL) {
		datosUsuarios = (datosUsuario *)getData(usuarios, posUsuarios);

		/* Nick encontrado. */
		if (!strcmp(nickName, datosUsuarios->nickName)) {
			strcpy(datosUsuarios->nombreReal, nombreReal);

			return 0;
		}

		if ((posUsuarios = nextPosition(usuarios, posUsuarios)) == lastPosition(usuarios))
			posUsuarios = NULL;
	}

	/* Si no se encuentra el nick. */
	return ERR_ALREADYREGISTRED;
}


int mensajesOrd(nick nickName, char * receptor, char * mensaje, List * usuarios, List * canales)
{
	if (!strcmp(nickName, "")) {
		return ERR_NOREGISTERED;
	}

	char canalReceptor[200];
	char msgEnvio[400];
	int i = 1;
	int cont = 0;

	idPosition posUsuarios;
	datosUsuario * datosUsuarios;

	idPosition posCanales;
	datosCanal * datosCanales;

	List * nicks;
	idPosition posNicks;
	nick * datosNicks;

	sprintf(msgEnvio, "%s: %s", nickName, mensaje);


	if (receptor[0] == '#') {

		while(receptor[i] != '\0') {
			canalReceptor[i-1] = receptor[i];
			i++;
		}
		canalReceptor[i-1] = '\0';

		posCanales = (isEmpty(canales) ? NULL : firstPosition(canales));
		while (posCanales != NULL) {
			datosCanales = (datosCanal *)getData(canales, posCanales);

			if (!strcmp(canalReceptor, datosCanales->nombreCanal)) {
				nicks = &(datosCanales->nicks);

				/* Buscamos el nick. */
				posNicks = (isEmpty(nicks) ? NULL : firstPosition(nicks));

				while (posNicks != NULL) {
					datosNicks = (nick *)getData(nicks, posNicks);

					posUsuarios = (isEmpty(usuarios) ? NULL : firstPosition(usuarios));

					while (posUsuarios != NULL) {
						datosUsuarios = (datosUsuario *)getData(usuarios, posUsuarios);

						if(!strcmp(*datosNicks, datosUsuarios->nickName)) {
							if (send(datosUsuarios->idSock, msgEnvio, TAM_BUFFER, 0) != TAM_BUFFER) {
								fprintf(stderr , "S) Connection with aborted on error 4\n");
								exit(1);
							}
							cont++;
							break;
						}

						if ((posUsuarios = nextPosition(usuarios, posUsuarios)) == lastPosition(usuarios))
							posUsuarios = NULL;
					}

					if ((posNicks = nextPosition(nicks, posNicks)) == lastPosition(nicks))
						posNicks = NULL;
				}

				break;
			}

			if ((posCanales = nextPosition(canales, posCanales)) == lastPosition(canales))
				posCanales = NULL;
		}
	} else {
		posUsuarios = (isEmpty(usuarios) ? NULL : firstPosition(usuarios));

		while (posUsuarios != NULL) {
			datosUsuarios = (datosUsuario *)getData(usuarios, posUsuarios);

			if(!strcmp(receptor, datosUsuarios->nickName)) {
				if (send(datosUsuarios->idSock, msgEnvio, TAM_BUFFER, 0) != TAM_BUFFER) {
					fprintf(stderr , "S) Connection with aborted on error 5\n");
					exit(1);
				}
				return 0;
			}

			if ((posUsuarios = nextPosition(usuarios, posUsuarios)) == lastPosition(usuarios))
				posUsuarios = NULL;
		}
	}

	return (cont ? 0 : ERR_NOSUCHNICK);
}


int joinOrd(nick nickName, nombre canal, List * canales)
{
	if (!strcmp(nickName, "")) {
		return ERR_NOREGISTERED;
	}

	idPosition posCanales;
	datosCanal * datosCanales;

	List * nicks;
	idPosition posNicks;
	nick * datosNicks;

	/* Buscamos el canal. */
	posCanales = (isEmpty(canales) ? NULL : firstPosition(canales));

	while (posCanales != NULL) {
		datosCanales = (datosCanal *)getData(canales, posCanales);

		/* Canal encontrado. */
		if (!strcmp(canal, datosCanales->nombreCanal)) {
			nicks = &(datosCanales->nicks);

			/* Buscamos el nick. */
			posNicks = (isEmpty(nicks) ? NULL : firstPosition(nicks));

			while (posNicks != NULL) {
				datosNicks = (nick *)getData(nicks, posNicks);

				/* Nick encontrado. */
				if (!strcmp(nickName, *datosNicks)) {
					return ERR_ALREADYREGISTREDINCHANEL;
				}

				if ((posNicks = nextPosition(nicks, posNicks)) == lastPosition(nicks))
					posNicks = NULL;
			}

			/* Si no se encuentra el nick. */
			if ((datosNicks = (nick *)malloc(sizeof(nick))) == NULL) {
				perror("memoriaNick");
				exit(151);
			}
			strcpy(*datosNicks, nickName);

			insertAt(nicks, datosNicks, lastPosition(nicks));

			return 0;
		}

		if ((posCanales = nextPosition(canales, posCanales)) == lastPosition(canales))
			posCanales = NULL;
	}

	/* Si no se encuentra el canal. */
	if ((datosCanales = (datosCanal *)malloc(sizeof(datosCanal))) == NULL) {
		perror("memoriaCanal");
		exit(151);
	}
	strcpy(datosCanales->nombreCanal, canal);
	createEmpty(&(datosCanales->nicks));

	if ((datosNicks = (nick *)malloc(sizeof(nick))) == NULL) {
		perror("memoriaNick");
		exit(151);
	}
	strcpy(*datosNicks, nickName);

	insertAt(&(datosCanales->nicks), datosNicks, lastPosition(&(datosCanales->nicks)));
	insertAt(canales, datosCanales, lastPosition(canales));

	return 0;
}


int partOrd(nick nickName, nombre canal, char * mensaje, List * canales)
{
	if (!strcmp(nickName, "")) {
		return ERR_NOREGISTERED;
	}

	idPosition posCanales;
	datosCanal * datosCanales;

	List * nicks;
	idPosition posNicks;
	nick * datosNicks;

	/* Buscamos el canal. */
	posCanales = (isEmpty(canales) ? NULL : firstPosition(canales));

	while (posCanales != NULL) {
		datosCanales = (datosCanal *)getData(canales, posCanales);

		/* Canal encontrado. */
		if (!strcmp(canal, datosCanales->nombreCanal)) {
			nicks = &(datosCanales->nicks);

			/* Buscamos el nick. */
			posNicks = (isEmpty(nicks) ? NULL : firstPosition(nicks));

			while (posNicks != NULL) {
				datosNicks = (nick *)getData(nicks, posNicks);

				/* Nick encontrado. */
				if (!strcmp(nickName, *datosNicks)) {
					removeAt(nicks, posNicks);
					if (isEmpty(nicks)) {
						destroy(nicks);
						removeAt(canales, posCanales);
					}
					return 0;
				}

				if ((posNicks = nextPosition(nicks, posNicks)) == lastPosition(nicks))
					posNicks = NULL;
			}

			/* Si no se encuentra el nick. */
			return ERR_NOREGISTEREDINCHANEL;
		}

		if ((posCanales = nextPosition(canales, posCanales)) == lastPosition(canales))
			posCanales = NULL;
	}

	/* Si no se encuentra el canal. */
	return ERR_NOSUCHCHANNEL;
}


int quitOrd(nick nickName, char * mensaje, List * usuarios, List * canales)
{
	if (!strcmp(nickName, "")) {
		return ERR_NOREGISTERED;
	}

	idPosition posUsuarios;
	datosUsuario * datosUsuarios;

	idPosition posCanales;
	datosCanal * datosCanales;

	List * nicks;
	idPosition posNicks;
	nick * datosNicks;


	/* Eliminamos al usuario de todos los canales. */
	/* Buscamos el canal. */
	posCanales = (isEmpty(canales) ? NULL : firstPosition(canales));

	while (posCanales != NULL) {
		datosCanales = (datosCanal *)getData(canales, posCanales);

		nicks = &(datosCanales->nicks);

		/* Buscamos el nick. */
		posNicks = (isEmpty(nicks) ? NULL : firstPosition(nicks));

		while (posNicks != NULL) {
			datosNicks = (nick *)getData(nicks, posNicks);

			/* Nick encontrado. */
			if (!strcmp(nickName, *datosNicks)) {
				removeAt(nicks, posNicks);
				if (isEmpty(nicks)) {
					destroy(nicks);
					removeAt(canales, posCanales);
				}
				break;
			}

			if ((posNicks = nextPosition(nicks, posNicks)) == lastPosition(nicks))
				posNicks = NULL;
		}

		if ((posCanales = nextPosition(canales, posCanales)) == lastPosition(canales))
			posCanales = NULL;
	}

	/* Eliminamos al usuario de la lista de usuarios. */
	/* Buscamos el nick. */
	posUsuarios = (isEmpty(usuarios) ? NULL : firstPosition(usuarios));

	while (posUsuarios != NULL) {
		datosUsuarios = (datosUsuario *)getData(usuarios, posUsuarios);

		/* Nick encontrado. */
		if (!strcmp(nickName, datosUsuarios->nickName)) {
			removeAt(usuarios, posUsuarios);
			break;
		}

		if ((posUsuarios = nextPosition(usuarios, posUsuarios)) == lastPosition(usuarios))
			posUsuarios = NULL;
	}

	return 0;
}


void dividirBuffer(buffer * cadena, ordenes * orden, arg_1 * arg1, arg_2 * arg2)
{
	int i = 0;
	int j;
	int k;

	while ((*cadena)[i] != ' ' && (*cadena)[i] != '\r') {
		i++;
	}
	strncpy((*orden), (*cadena), i);
	(*orden)[i] = '\0';
	if ((*cadena)[i] != '\r') {
		j = i + 1;
		while ((*cadena)[j] != ' ' && (*cadena)[j] != '\r') {
			j++;
		}
		for(k = 0; k < (j - (i + 1)); k++) {
			(*arg1)[k] = (*cadena)[(i + 1) + k];
		}
		(*arg1)[k] = '\0';
		//Si no es final del cadena, seguimos buscando
		if((*cadena)[j] != '\r') {
			i = j +1;
			//Recorremos hasta que encuentre los dos puntos
			while ((*cadena)[i] != ':') {
				i++;
			}
			//Pasamos a la siguiente posición
			i++;
			j = 0;
			// Iteramos desde la posición de después de los dos puntos hasta
			// que encuentre un return
			while ((*cadena)[i] != '\r') {
				(*arg2)[j] = (*cadena)[i];
				i++;
				j++;
			}
			(*arg2)[j] = '\0';
		}
	}
}


/* @TODO */
/*
	-funcion mensajes
	-UDP
	-log servidor
 */
