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

	List usuarios;
	List canales;
	createEmpty(&usuarios);
	createEmpty(&canales);

	struct sigaction sa = {.sa_handler = SIG_IGN};  /* used to ignore SIGCHLD */

	struct sockaddr_in localaddr_in;  /* for local socket address */
	struct sockaddr_in clientaddr_in;  /* for peer socket address */
	socklen_t addrlen = sizeof(struct sockaddr_in);

	fd_set readmask;
	int numfds, s_mayor;

	buffer buf;  /* buf for packets to be read into */

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
						serverUDP(s_UDP, buf, clientaddr_in, &usuarios, &canales);
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
void serverTCP(int idSoc, struct sockaddr_in clientaddr_in, List * usuarios, List * canales)
{
	int reqcnt = 0;  /* keeps count of number of requests */
	buffer buf;  /* This example uses TAM_BUFFER byte messages. */
	char hostname[HOSTLEN];  /* remote host's name string */
	ordenes orden;
	arg_1 arg1;
	arg_2 arg2;
	nick nickName;
	int status;
	int checkCode;
	int salir = 0;

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

	strcpy(nickName, "");

	while (!salir) {
		if (recv(idSoc, buf, TAM_BUFFER, 0) == -1) {
			printf("S) Connection with %s aborted on error\n", hostname);
			exit(1);
		}

		dividirBuffer(&buf, &orden, &arg1, &arg2);

		/*
		 * This sleep simulates the processing of the
		 * request that a real server might do.
		 */
		sleep(1);

		if (!strcmp(orden, "NICK")) {
			checkCode = nickOrd(arg1, usuarios, &clientaddr_in);
			if(!checkCode) {
				strcpy(nickName, arg1);
				sprintf(buf, "User %s registered.", nickName);
			}
			else if (checkCode == ERR_NICKNAME)
				sprintf(buf, ":%s %u %s:Nickname is already in use.", "h", ERR_NICKNAME, arg1);
		}
		else if (!strcmp(orden, "USER")) {
			checkCode = userOrd(nickName, arg1, arg2, usuarios);
			if(!checkCode) {
				sprintf(buf, "Real name %s registered.", arg2);
			}
			else if (checkCode == ERR_ALREADYREGISTRED)
				sprintf(buf, ":%s %u %s:You may not reregister.", "h", ERR_ALREADYREGISTRED, arg1);
			else if (checkCode == ERR_NOVALIDUSER)
				sprintf(buf, ":%s %u %s:This isn't your nickname.", "h", ERR_NOVALIDUSER, arg1);
			else if (checkCode == ERR_NOREGISTERED)
				sprintf(buf, "You may registrer a NICK first.");
		}
		else if (!strcmp(orden, "PRIVMSG")) {
			checkCode = mensajesOrd(nickName, arg1, arg2, usuarios, canales);
			checkCode = 0;
			if(!checkCode) {
				sprintf(buf, "Message %s sended to %s.", arg2, arg1);
			}
			else if (checkCode == ERR_NOSUCHNICK)
				sprintf(buf, ":%s %u %s:No such nick/channel.", "h", ERR_NOSUCHNICK , arg1);
			else if (checkCode == ERR_NOREGISTERED)
				sprintf(buf, "You may registrer a NICK first.");
		}
		else if (!strcmp(orden, "JOIN")) {
			checkCode = joinOrd(nickName, arg1, usuarios, canales);
			if(!checkCode) {
				sprintf(buf, "Joined to %s chanel.", arg1);
			}
			else if (checkCode == ERR_ALREADYREGISTREDINCHANEL)
				sprintf(buf, ":%s %u %s:The user is already at this chanel.", "h", ERR_ALREADYREGISTREDINCHANEL, nickName);
			else if (checkCode == ERR_NOREGISTERED)
				sprintf(buf, "You may registrer a NICK first.");
		}
		else if (!strcmp(orden, "PART")) {
			checkCode = partOrd(nickName, arg1, arg2, canales);
			if(!checkCode) {
				sprintf(buf, "Exited from %s chanel.", arg1);
			}
			else if (checkCode == ERR_NOREGISTEREDINCHANEL)
				sprintf(buf, ":%s %u %s:The user isn't registered at this chanel.", "h", ERR_NOREGISTEREDINCHANEL, nickName);
			else if (checkCode == ERR_NOSUCHCHANNEL)
				sprintf(buf, ":%s %u %s:No such channel.", "h", ERR_NOSUCHCHANNEL, arg1);
			else if (checkCode == ERR_NOREGISTERED)
				sprintf(buf, "You may registrer a NICK first.");
		}
		else if (!strcmp(orden, "QUIT")) {
			quitOrd(nickName, arg1, usuarios, canales);
			sprintf(buf, "Exited from application");
			salir = 1;
		}
		else
			sprintf(buf, "La funcion no existente\n");


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
void serverUDP(int idSoc, buffer buf, struct sockaddr_in clientaddr_in, List * usuarios, List * canales)
{
	struct in_addr reqaddr;  /* for requested host's address */
	int nc;

	struct addrinfo hints, *res;

	int addrlen = sizeof(struct sockaddr_in);

	buffer strRecv;
	ordenes orden;
	arg_1 arg1;
	arg_2 arg2;
	nick nickName;
	int checkCode;
	strcpy(strRecv, buf);

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

	strcpy(nickName, "");

	dividirBuffer(&strRecv, &orden, &arg1, &arg2);

	if (!strcmp(orden, "NICK")) {
		checkCode = nickOrd(arg1, usuarios, &clientaddr_in);
		if(!checkCode) {
			strcpy(nickName, arg1);
			sprintf(buf, "User registered.");
		}
		else if (checkCode == ERR_NICKNAME)
			sprintf(buf, ":%s %u %s:Nickname is already in use.", "h", ERR_NICKNAME, arg1);
	}
	else if (!strcmp(orden, "USER")) {
		checkCode = userOrd(nickName, arg1, arg2, usuarios);
		if(!checkCode) {
			sprintf(buf, "Real name registered.");
		}
		else if (checkCode == ERR_ALREADYREGISTRED)
			sprintf(buf, ":%s %u %s:You may not reregister.", "h", ERR_ALREADYREGISTRED, arg1);
		else if (checkCode == ERR_NOVALIDUSER)
			sprintf(buf, ":%s %u %s:This isn't your nickname.", "h", ERR_NOVALIDUSER, arg1);
		else if (checkCode == ERR_NOREGISTERED)
			sprintf(buf, "You may registrer a NICK first.");
	}
	else if (!strcmp(orden, "PRIVMSG")) {
		checkCode = mensajesOrd(nickName, arg1, arg2, usuarios, canales);
		if(!checkCode) {
			sprintf(buf, "Message sended to %s.", arg1);
		}
		else if (checkCode == ERR_NOSUCHNICK)
			sprintf(buf, ":%s %u %s:No such nick/channel.", "h", ERR_NOSUCHNICK , arg1);
		else if (checkCode == ERR_NOREGISTERED)
			sprintf(buf, "You may registrer a NICK first.");
	}
	else if (!strcmp(orden, "JOIN")) {
		checkCode = joinOrd(nickName, arg1, usuarios, canales);
		if(!checkCode) {
			sprintf(buf, "Joined to %s chanel.", arg1);
		}
		else if (checkCode == ERR_ALREADYREGISTREDINCHANEL)
			sprintf(buf, ":%s %u %s:The user is already at this chanel.", "h", ERR_ALREADYREGISTREDINCHANEL, nickName);
		else if (checkCode == ERR_NOREGISTERED)
			sprintf(buf, "You may registrer a NICK first.");
	}
	else if (!strcmp(orden, "PART")) {
		checkCode = partOrd(nickName, arg1, arg2, canales);
		if(!checkCode) {
			sprintf(buf, "Exited from %s chanel.", arg1);
		}
		else if (checkCode == ERR_NOREGISTEREDINCHANEL)
			sprintf(buf, ":%s %u %s:The user isn't registered at this chanel.", "h", ERR_NOREGISTEREDINCHANEL, nickName);
		else if (checkCode == ERR_NOSUCHCHANNEL)
			sprintf(buf, ":%s %u %s:No such channel.", "h", ERR_NOSUCHCHANNEL, arg1);
		else if (checkCode == ERR_NOREGISTERED)
			sprintf(buf, "You may registrer a NICK first.");
	}
	else if (!strcmp(orden, "QUIT")) {
		quitOrd(nickName, arg1, usuarios, canales);
		sprintf(buf, "Exited from application");
	}
	else
		sprintf(buf, "La funcion no existente\n");


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


int nickOrd(nick nickName, List * usuarios, struct sockaddr_in * clientAddr)
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

	insertAt(usuarios, datosUsuarios, lastPosition(usuarios));
	return 0;
}


int userOrd(nick nickName, nick usuario, nombre nombreReal, List * usuarios)
{
	idPosition posUsuarios;
	datosUsuario * datosUsuarios;

	if (!strcmp(nickName, "")) {
		return ERR_NOREGISTERED;
	}

	if (strcmp(nickName, usuario)) {
		return ERR_NOVALIDUSER;
	}

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

	return ERR_NOSUCHNICK;
}


int joinOrd(nick nickName, nombre canal, List * usuarios, List * canales)
{
	idPosition posCanales;
	datosCanal * datosCanales;

	List * nicks;
	idPosition posNicks;
	nick * datosNicks;

	if (!strcmp(nickName, "")) {
		return ERR_NOREGISTERED;
	}

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
	idPosition posCanales;
	datosCanal * datosCanales;

	List * nicks;
	idPosition posNicks;
	nick * datosNicks;

	if (!strcmp(nickName, "")) {
		return ERR_NOREGISTERED;
	}

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
	}
/*
	i = j + 1;
	while((*cadena)[i] != "\n") {
		i++;
	}
*/
	strcpy(*arg2, "arg2");
}


/* @TODO */
/*
	-funcion dividir candena
	-funcion mensajes
	-UDP
	-log servidor
 */
