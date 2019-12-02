/*
 *								C L I E N T E
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

#include "cliente.h"


extern int errno;



int main(int argc, const char *argv[])
{
	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Usage: %s <remote host> <TCP/UDP> <ficheroOrdenes.txt>\n", argv[0]);
		exit(151);
	}


	buffer * datosFichero = NULL;
	int soc;  /* soc = connected socket descriptor */
	socklen_t addrlen = sizeof(struct sockaddr_in);
	struct addrinfo hints, *res;
	struct sockaddr_in clientaddr_in;  /* for local socket address */
	struct sockaddr_in serveraddr_in;  /* for server socket address */


	/* clear out address structures */
	memset(&clientaddr_in, 0, addrlen);
	memset(&serveraddr_in, 0, addrlen);
	memset(&hints, 0, sizeof(hints));

	/* Set up the peer address to which we will connect. */
	serveraddr_in.sin_family = AF_INET;

	/* Set up our own address */
	clientaddr_in.sin_family = AF_INET;

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

	/* Copy address of host */
	serveraddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	freeaddrinfo(res);

	/* puerto del servidor en orden de red */
	serveraddr_in.sin_port = htons(PUERTO);

	clientaddr_in.sin_addr.s_addr = INADDR_ANY;


	if (!strcmp(argv[2], "TCP")) {
		/* This example uses TAM_BUFFER byte messages. */
		buffer buf;

		/* Create the socket. */
		soc = socket(AF_INET, SOCK_STREAM, 0);
		if (soc == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to create socket\n", argv[0]);
			exit(1);
		}

		/*
		 * Try to connect to the remote server at the address
		 * which was just built into peeraddr.
		 */
		if (connect(soc, (const struct sockaddr *)&serveraddr_in, addrlen) == -1) {
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
		if (-1 == getsockname(soc, (struct sockaddr *)&clientaddr_in, &addrlen)) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
			exit(1);
		}


		/*
		 * The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
		printf("Connected to %s on port %u at %s",
				argv[1], ntohs(clientaddr_in.sin_port), timeString());


		for (int i = 1; i <= 5; i++) {
			if (send(soc, buf, TAM_BUFFER, 0) != TAM_BUFFER) {
				fprintf(stderr, "%s: Connection aborted on error ", argv[0]);
				fprintf(stderr, "on send number %d\n", i);
				exit(1);
			}
		}

		/*
		 * Now, shutdown the connection for further sends.
		 * condition after it has received all the requests that
		 * have just been sent, indicating that we will not be
		 * sending any further requests.
		 */
		if (shutdown(soc, 1) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
			exit(1);
		}

		/*
		 * Now, start receiving all of the replys from the server.
		 * This loop will terminate when the recv returns zero,
		 * which is an end-of-file condition.  This will happen
		 * after the server has sent all of its replies, and closed
		 * its end of the connection.
		 */
		int i, j;
		while ((i = recv(soc, buf, TAM_BUFFER, 0))) {
			if (i == -1) {
				perror(argv[0]);
				fprintf(stderr, "%s: error reading result\n", argv[0]);
				exit(1);
			}

			/*
			 * The reason this while loop exists is that there
			 * is a remote possibility of the above recv returning
			 * less than TAM_BUFFER bytes.
			 * This is because a recv returns as soon as there is some data,
			 * and will not wait for all of the requested data to arrive.
			 * Since TAM_BUFFER bytes is relatively small
			 * compared to the allowed TCP packet sizes,
			 * a partial receive is unlikely.
			 * If this example had used 2048 bytes requests instead,
			 * a partial receive would be far more likely.
			 * This loop will keep receiving until all TAM_BUFFER bytes
			 * have been received, thus guaranteeing that the
			 * next recv at the top of the loop will start at
			 * the begining of the next reply.
			 */
			while (i < TAM_BUFFER) {
				j = recv(soc, &buf[i], TAM_BUFFER - i, 0);
				if (j == -1) {
					perror(argv[0]);
					fprintf(stderr, "%s: error reading result\n", argv[0]);
					exit(1);
				}
				i += j;
			}

			/* Print out message indicating the identity of this reply. */
			printf("Received result number %d\n", *buf);
		}

		/* Print message indicating completion of task. */
		printf("All done at %s", timeString());

		free(datosFichero);
	}
	else if (!strcmp(argv[2], "UDP")) {
		int n_retry = RETRIES;  /* holds the retry count */
		char hostname[HOSTLEN];
		struct in_addr reqaddr;  /* for returned internet address */
		struct sigaction vec;

		/* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
		vec.sa_handler = (void *)handler;
		vec.sa_flags = 0;
		if (sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
			perror("sigaction(SIGALRM)");
			fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
			exit(1);
		}

		/* Create the socket. */
		soc = socket(AF_INET, SOCK_DGRAM, 0);
		if (soc == -1) {
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
		if (bind(soc, (const struct sockaddr *)&clientaddr_in, addrlen) == -1) {
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
		if (-1 == getsockname(soc, (struct sockaddr *)&clientaddr_in, &addrlen)) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
			exit(1);
		}


		/*
		 * The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
		printf("Connected to %s on port %u at %s",
				argv[1], ntohs(clientaddr_in.sin_port), timeString());


		while (n_retry > 0) {
			/* Send the request to the nameserver. */
			if (sendto (soc, argv[2], strlen(argv[2]), 0, (struct sockaddr *)&serveraddr_in,
				addrlen) == -1) {
				perror(argv[0]);
				fprintf(stderr, "%s: unable to send request\n", argv[0]);
				exit(1);
			}

			/*
			 * Set up a timeout so I don't hang in case the packet
			 * gets lost.  After all, UDP does not guarantee
			 * delivery.
			 */
			alarm(TIMEOUT);
			/* Wait for the reply to come in. */
			if (-1 == recvfrom(soc, &reqaddr, sizeof(struct in_addr), 0,
				(struct sockaddr *)&serveraddr_in, &addrlen)) {
				if (errno == EINTR) {
					/*
					 * Alarm went off and aborted the receive.
					 * Need to retry the request if we have
					 * not already exceeded the retry limit.
					 */
					printf("attempt %d (retries %d).\n", n_retry, RETRIES);
					n_retry--;
				}
				else {
					printf("Unable to get response from");
					exit(1);
				}
			}
			else {
				alarm(0);
				/* Print out response. */
				if (reqaddr.s_addr == ADDRNOTFOUND) {
					printf("Host %s unknown by nameserver %s\n", argv[2], argv[1]);
				}
				else {
					/* inet_ntop para interoperatividad con IPv6 */
					if (inet_ntop(AF_INET, &reqaddr, hostname, HOSTLEN) == NULL)
						perror(" inet_ntop \n");
					printf("Address for %s is %s\n", argv[2], hostname);
				}

				break;
			}
		}

		if (n_retry == 0) {
			printf("Unable to get response from %s after %d attempts.\n",
				argv[1], RETRIES);
		}

		free(datosFichero);
	}
	else {
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


void nick(char * nickName)
{
	fprintf(stderr, ":%s %u %s:Nickname is already in use.", "h", ERR_NICKNAME, nickName);
	exit(ERR_NICKNAME);
}


void user(char * username)
{
	fprintf(stderr, ":%s %u %s:You may not reregister.", "h", ERR_ALREADYREGISTRED, username);
	exit(ERR_ALREADYREGISTRED);
}


void mensajes(char * receptor, char * mensaje)
{
	fprintf(stderr, ":%s %u %s:No such nick/channel.", "h", ERR_NOSUCHNICK , receptor);
	exit(ERR_NOSUCHNICK);
}


void join(char * canal)
{
	return;
}


void part(char * canal)
{
	fprintf(stderr, ":%s %u %s:No such channel.", "h", ERR_NOSUCHCHANNEL,canal);
	exit(ERR_NOSUCHCHANNEL);
}


void quit(char * mensaje)
{
	return;
}
