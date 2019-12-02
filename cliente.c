#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


extern int errno;

#define ADDRNOTFOUND	0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */
#define BUFFERSIZE	1024	/* maximum size of packets to be received udp*/
#define TAM_BUFFER 10	/* maximum size of packets to be received for tcp*/
#define PUERTO 8545
#define TIMEOUT 6
#define MAXHOST 512

/*
 *			H A N D L E R
 *
 *	This routine is the signal handler for the alarm signal.
 */
void handler()
{
	printf("Alarma recibida \n");
}



int main(int argc, char const *argv[])
{
	if (argc < 4) {
		fprintf(stderr, "Usage: %s <remote host> <TCP/UDP> <ficheroOrdenes.txt>\n", argv[0]);
		exit(151);
	}

	long timevar;  /* contains time returned by time() */
	int addrlen, errcode, s;  /* s = connected socket descriptor */
	struct addrinfo hints, *res;
	struct sockaddr_in myaddr_in;  /* for local socket address */
	struct sockaddr_in servaddr_in;  /* for server socket address */

	/* clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));


	if (!strcmp(toupper(argv[2]), "TCP")) {
		int i, j;
		/* This example uses TAM_BUFFER byte messages. */
		char buf[TAM_BUFFER];

		/* Create the socket. */
		s = socket (AF_INET, SOCK_STREAM, 0);
		if (s == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to create socket\n", argv[0]);
			exit(1);
		}

		/* Set up the peer address to which we will connect. */
		servaddr_in.sin_family = AF_INET;

		/* Get the host information for the hostname that the
		* user passed in. */
		memset(&hints, 0, sizeof (hints));
		hints.ai_family = AF_INET;

		/* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
		errcode = getaddrinfo (argv[1], NULL, &hints, &res);
		if (errcode != 0) {
				/* Name was not found.  Return a
				* special value signifying the error. */
			fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
					argv[0], argv[1]);
			exit(1);
		}
		else {
			/* Copy address of host */
			servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
		}

		freeaddrinfo(res);

		/* puerto del servidor en orden de red*/
		servaddr_in.sin_port = htons(PUERTO);

		/* Try to connect to the remote server at the address
		* which was just built into peeraddr.
		*/
		if (-1 == connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in))) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
			exit(1);
		}

		/* Since the connect call assigns a free address
		* to the local end of this connection, let's use
		* getsockname to see what it assigned.  Note that
		* addrlen needs to be passed in as a pointer,
		* because getsockname returns the actual length
		* of the address.
		*/
		addrlen = sizeof(struct sockaddr_in);
		if (-1 == getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen)) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
			exit(1);
		}

		/* Print out a startup message for the user. */
		time(&timevar);

		/* The port number must be converted first to host byte
		* order before printing.  On most hosts, this is not
		* necessary, but the ntohs() call is included here so
		* that this program could easily be ported to a host
		* that does require it.
		*/
		printf("Connected to %s on port %u at %s",
				argv[1], ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));


		for (i = 1; i <= 5; i++) {
			*buf = i;
			if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) {
				fprintf(stderr, "%s: Connection aborted on error ",	argv[0]);
				fprintf(stderr, "on send number %d\n", i);
				exit(1);
			}
		}

		/* Now, shutdown the connection for further sends.
		* This will cause the server to receive an end-of-file
		* condition after it has received all the requests that
		* have just been sent, indicating that we will not be
		* sending any further requests.
		*/
		if (shutdown(s, 1) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
			exit(1);
		}

		/* Now, start receiving all of the replys from the server.
		* This loop will terminate when the recv returns zero,
		* which is an end-of-file condition.  This will happen
		* after the server has sent all of its replies, and closed
		* its end of the connection.
		*/
		while (i = recv(s, buf, TAM_BUFFER, 0)) {
			if (i == -1) {
				perror(argv[0]);
				fprintf(stderr, "%s: error reading result\n", argv[0]);
				exit(1);
			}

			/* The reason this while loop exists is that there
			* is a remote possibility of the above recv returning
			* less than TAM_BUFFER bytes.  This is because a recv returns
			* as soon as there is some data, and will not wait for
			* all of the requested data to arrive.  Since TAM_BUFFER bytes
			* is relatively small compared to the allowed TCP
			* packet sizes, a partial receive is unlikely.  If
			* this example had used 2048 bytes requests instead,
			* a partial receive would be far more likely.
			* This loop will keep receiving until all TAM_BUFFER bytes
			* have been received, thus guaranteeing that the
			* next recv at the top of the loop will start at
			* the begining of the next reply.
			*/
			while (i < TAM_BUFFER) {
				j = recv(s, &buf[i], TAM_BUFFER - i, 0);
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
		time(&timevar);
		printf("All done at %s", (char *)ctime(&timevar));
	}
	else if (!strcmp(toupper(argv[2]), "UDP")) {
		int n_retry;
		int retry = RETRIES;  /* holds the retry count */
		struct in_addr reqaddr;  /* for returned internet address */
		struct sigaction vec;
		char hostname[MAXHOST];

		/* Create the socket. */
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s == -1) {
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
		myaddr_in.sin_family = AF_INET;
		myaddr_in.sin_port = 0;
		myaddr_in.sin_addr.s_addr = INADDR_ANY;
		if (-1 == bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in))) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
			exit(1);
		}

		/* Since the connect call assigns a free address
		* to the local end of this connection, let's use
		* getsockname to see what it assigned.  Note that
		* addrlen needs to be passed in as a pointer,
		* because getsockname returns the actual length
		* of the address.
		*/
		addrlen = sizeof(struct sockaddr_in);
		if (-1 == getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen)) {
				perror(argv[0]);
				fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
				exit(1);
		}

		/* Print out a startup message for the user. */
		time(&timevar);

		/* The port number must be converted first to host byte
		* order before printing.  On most hosts, this is not
		* necessary, but the ntohs() call is included here so
		* that this program could easily be ported to a host
		* that does require it.
		*/
		printf("Connected to %s on port %u at %s",
				argv[1], ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));

		/* Set up the server address. */
		servaddr_in.sin_family = AF_INET;

		/* Get the host information for the server's hostname that the
		* user passed in.
		*/
		memset(&hints, 0, sizeof (hints));
		hints.ai_family = AF_INET;

		/* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
		errcode = getaddrinfo(argv[1], NULL, &hints, &res);

		if (errcode != 0) {
			/* Name was not found.  Return a
			* special value signifying the error.
			*/
			fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
					argv[0], argv[1]);
			exit(1);
		}
		else {
			/* Copy address of host */
			servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
		}

		freeaddrinfo(res);

		/* puerto del servidor en orden de red*/
		servaddr_in.sin_port = htons(PUERTO);

		/* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
		vec.sa_handler = (void *)handler;
		vec.sa_flags = 0;
		if (sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
			perror(" sigaction(SIGALRM)");
			fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
			exit(1);
		}

		n_retry = RETRIES;

		while (n_retry > 0) {
			/* Send the request to the nameserver. */
			if (sendto (s, argv[2], strlen(argv[2]), 0, (struct sockaddr *)&servaddr_in,
				sizeof(struct sockaddr_in)) == -1) {
				perror(argv[0]);
				fprintf(stderr, "%s: unable to send request\n", argv[0]);
				exit(1);
			}
			/* Set up a timeout so I don't hang in case the packet
			* gets lost.  After all, UDP does not guarantee
			* delivery.
			*/
			alarm(TIMEOUT);
			/* Wait for the reply to come in. */
			if (-1 == recvfrom(s, &reqaddr, sizeof(struct in_addr), 0,
				(struct sockaddr *)&servaddr_in, &addrlen)) {
				if (errno == EINTR) {
					/* Alarm went off and aborted the receive.
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
					if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
						perror(" inet_ntop \n");
					printf("Address for %s is %s\n", argv[2], hostname);
				}
				break;
			}
		}

		if (n_retry == 0) {
			printf("Unable to get response from");
			printf(" %s after %d attempts.\n", argv[1], RETRIES);
		}
	} else {
		fprintf(stderr, "Orden no valida, introduce TCP o UDP\n");
		exit(152);
	}


	return 0;
}
