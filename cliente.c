/*
** Fichero: cliente.c
** Autores:
** Carlos Manjón García
** Miguel Sánchez González
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

#include "cliente.h"


extern int errno;

int FIN = 0; /* Para el cierre ordenado */


int main(int argc, const char *argv[])
{
	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Usage: %s <remote host> <TCP/UDP> <ficheroOrdenes.txt>\n", argv[0]);
		exit(151);
	}

	int idSoc;  /* idSoc = connected socket descriptor */
	socklen_t addrlen = sizeof(struct sockaddr_in);
	struct addrinfo hints, *res;
	struct sockaddr_in localaddr_in;  /* for local socket address */
	struct sockaddr_in serveraddr_in;  /* for server socket address */
	pthread_t hiloRecibir;  /* hilo de recepción */
	struct sigaction sigSalir;
	struct sigaction vec;
	int nRead = 0;
	int n_retry = 0;
	buffer * datosFichero;
	buffer buf;
	char outF[1024];
	DatosHilo datosHilo;  /* Struct con los datos del socket */


	/* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
	vec.sa_handler = (void *) handler;
	vec.sa_flags = 0;
	if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
		perror(" sigaction(SIGALRM)");
		fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
		exit(1);
	}

	/* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
	sigSalir.sa_handler = (void *)finalizar;
	sigSalir.sa_flags = 0;
	if (sigaction(SIGTERM, &sigSalir, (struct sigaction *)0) == -1) {
		perror("sigaction(SIGTERM)");
		fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
		exit(1);
	}

	/* clear out address structures */
	memset(&localaddr_in, 0, addrlen);
	memset(&serveraddr_in, 0, addrlen);
	memset(&hints, 0, sizeof(hints));

	/* Set up the peer address to which we will connect. */
	serveraddr_in.sin_family = AF_INET;

	/* Set up our own address */
	localaddr_in.sin_family = AF_INET;

	/* Get the host information for the hostname that the user passed in. */
	hints.ai_family = AF_INET;

	/*
	 * Esta función es la recomendada para la compatibilidad con IPv6
	 * gethostbyname queda obsoleta
	 */
	if (getaddrinfo(argv[1], NULL, &hints, &res) != 0) {
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				argv[0], argv[1]);
		exit(1);
	}
	datosHilo.argc = argc;

	/* Copy address of host */
	serveraddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	freeaddrinfo(res);

	/* puerto del servidor en orden de red */
	serveraddr_in.sin_port = htons(PUERTO);
	localaddr_in.sin_addr.s_addr = INADDR_ANY;


	if (!strcmp(argv[2], "TCP")) {
		/* Create the socket. */
		idSoc = socket(AF_INET, SOCK_STREAM, 0);
		if (idSoc == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to create socket\n", argv[0]);
			exit(1);
		}

		/*
		 * Try to connect to the remote server at the address
		 * which was just built into peeraddr.
		 */
		if (connect(idSoc, (const struct sockaddr *)&serveraddr_in, addrlen) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
			exit(1);
		}

		/*
		 * Since the connect call assigns a free address
		 * to the local end of this connection, let's use
		 * getsockname to see what it assigned.  Note that
		 * addrlen needs to be passed in as a pointer,
		 * because getsockname returns the actual length
		 * of the address.
		 */
		if (-1 == getsockname(idSoc, (struct sockaddr *)&localaddr_in, &addrlen)) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
			exit(1);
		}

		/* Cargamos los datos necesarios para los hilos en el struct */
		datosHilo.idSoc = idSoc;
		sprintf(datosHilo.fichero, "logs/%u.txt", ntohs(localaddr_in.sin_port));

		/*
		 * The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
		sprintf(outF, "Connected to %s on port %u at %s",
				argv[1], ntohs(localaddr_in.sin_port), timeString());
		escribirFichero(datosHilo.fichero, outF);


		if (argc == 4) {
			leerFichero(argv[3], &datosFichero, &nRead);

			/* handler, atributos del thread, función, argumentos de la función */
			if (pthread_create(&hiloRecibir, NULL, &recibirTCP, (void *)&datosHilo) != 0) {
				printf ("Error al crear el Thread\n");
				exit(1);
			}
			/* Enviamos los datos leidos. */
			for (int i = 0; i < nRead; i++) {
				if (strcmp(datosFichero[i], "")) {
					sprintf(outF, "Sended: %s", datosFichero[i]);
					strcat(datosFichero[i], "\r\n");
					escribirFichero(datosHilo.fichero, outF);
					if (send(idSoc, datosFichero[i], TAM_BUFFER, 0) != TAM_BUFFER) {
						fprintf(stderr, "%s: Connection aborted on error ", argv[0]);
						exit(1);
					}
				}
			}
		} else {
			/* handler, atributos del thread, función, argumentos de la función */
			if (pthread_create(&hiloRecibir, NULL, &recibirTCP, (void *)&datosHilo) != 0) {
				printf("Error al crear el Thread\n");
				exit(1);
			}

			while (!FIN) {
				strcpy(buf, "QUIT\r\n");
				if (send(idSoc, buf, TAM_BUFFER, 0) != TAM_BUFFER) {
					fprintf(stderr, "%s: Connection aborted on error ", argv[0]);
					exit(1);
				}
				break;
			}
		}


		/* Espera a que el thread termine */
		if (pthread_join(hiloRecibir, NULL) != 0)
			printf("Error al esperar por el ThreadRecibir\n");


		/* Print message indicating completion of task. */
		sprintf(outF, "All done at %s", timeString());
		escribirFichero(datosHilo.fichero, outF);
	}
	else if (!strcmp(argv[2], "UDP")) {
		struct sigaction vec;
		localaddr_in.sin_port = 0;
		/* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
		vec.sa_handler = (void *)handler;
		vec.sa_flags = 0;
		if (sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
			perror("sigaction(SIGALRM)");
			fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
			exit(1);
		}

		/* Create the socket. */
		idSoc = socket(AF_INET, SOCK_DGRAM, 0);
		if (idSoc == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to create socket\n", argv[0]);
			exit(1);
		}

		/* Bind socket to some local address so that the
		 * server can send the reply back.  A port number
		 * of zero will be used so that the system will
		 * assign any available port number.  An address
		 * of INADDR_ANY will be used so we do not have to
		 * look up the internet address of the local host.
		 */
		if (bind(idSoc, (const struct sockaddr *)&localaddr_in, addrlen) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
			exit(1);
		}

		/*
		 * Since the connect call assigns a free address
		 * to the local end of this connection, let's use
		 * getsockname to see what it assigned.  Note that
		 * addrlen needs to be passed in as a pointer,
		 * because getsockname returns the actual length
		 * of the address.
		 */
		if (-1 == getsockname(idSoc, (struct sockaddr *)&localaddr_in, &addrlen)) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
			exit(1);
		}

		sprintf(datosHilo.fichero, "logs/%u.txt", ntohs(localaddr_in.sin_port));

		/*
		 * The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
		sprintf(outF, "Connected to %s on port %u at %s",
				argv[1], ntohs(localaddr_in.sin_port), timeString());
		escribirFichero(datosHilo.fichero, outF);


		if (argc == 4) {
			leerFichero(argv[3], &datosFichero, &nRead);

			n_retry = RETRIES;
			int i = 0;
			/*
			 * Bucle encargado de reenviar el mensaje un número predeterminado
			 * de veces, si no es posible la recepción cerramos el soquet
			 * y finalizamos la comunciación
			 */
			while(n_retry > 0) {
				if (i < nRead) {
					sprintf(outF, "Sended: %s", datosFichero[i]);
					strcat(datosFichero[i], "\r\n");
					if (sendto(idSoc, datosFichero[i++], TAM_BUFFER, 0, (struct sockaddr *)&serveraddr_in, addrlen) == -1) {
						fprintf(stderr, "%s: unable to send request\n", argv[0]);
						exit(1);
					}
					escribirFichero(datosHilo.fichero, outF);
				}

				alarm(5);

				if (-1 == recvfrom(idSoc, buf, TAM_BUFFER, 0, (struct sockaddr *)&serveraddr_in, &addrlen)) {
					if (errno == EINTR) {
						/* Alarm went off and aborted the receive.
						* Need to retry the request if we have
						* not already exceeded the retry limit.
						*/
						fprintf(stderr, "attempt %d (retries %d).\n", n_retry, RETRIES);
						n_retry--;
					}else {
						fprintf(stderr, "%s: error reading result\n", argv[0]);
						exit(1);
					}
				}else {
					alarm(0);
					/*
					 * Si la señal recibida es un QUIT nos salimos del bucle
					 * y finalizamos la aplicacion
					*/
					if (!strcmp(buf, "server: Exited from application"))
						break;

					n_retry = RETRIES;
					/* Print out message indicating the identity of this reply. */
					sprintf(outF, "Message from %s", buf);
					escribirFichero(datosHilo.fichero, outF);
				}
			}
		}
		if (n_retry == 0) {
			fprintf(stderr, "Imposible comuncicacion con el servidor\n");
		} else {
			/* Print message indicating completion of task. */
			sprintf(outF, "All done at %s", timeString());
			escribirFichero(datosHilo.fichero, outF);
		}

	} else {
		/*
		* El tercer parametro no se corresponde con UDP o TCP
		*/
		fprintf(stderr, "Orden no valida, introduce TCP o UDP\n");
		exit(152);
	}

	return 0;
}


/*
 *						H A N D L E R
 *
 *	This routine is the signal handler for the alarm signal.
 */
void handler()
{
	printf("Alarma recibida \n");
}


void finalizar()
{
	FIN = 1;
}


void * recibirTCP(void * pDatos)
{
	DatosHilo *datosHilo = (DatosHilo *)pDatos;
	buffer buf;
	char outF[1024];
	int salir = 0;

	if (datosHilo->argc == 4) {
		while (!salir) {
			if (-1 == recv(datosHilo->idSoc, buf, TAM_BUFFER, 0)) {
				fprintf(stderr, "Client: error reading result\n");
				exit(1);
			}

			if (!strcmp(buf, "server: Exited from application"))
				salir = 1;

			/* Print out message indicating the identity of this reply. */
			sprintf(outF, "Message from %s", buf);
			escribirFichero(datosHilo->fichero, outF);
		}
	} else {
		while (!FIN) {
			if (-1 == recv(datosHilo->idSoc, buf, TAM_BUFFER, 0)) {
				fprintf(stderr, "Client: error reading result\n");
				exit(1);
			}

			/* Print out message indicating the identity of this reply. */
			sprintf(outF, "Message from %s", buf);
			escribirFichero(datosHilo->fichero, outF);
			break;
		}
	}

	return NULL;
}
