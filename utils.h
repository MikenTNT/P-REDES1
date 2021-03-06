/*
** Fichero: utils.h
** Autores:
** Carlos Manjón García
** Miguel Sánchez González
*/
#ifndef __UTILS_H
#define __UTILS_H


#include <sys/socket.h>

#define ADDRNOTFOUND 0xffffffff  /* value returned for unknown host */
#define HOSTLEN 512
#define TAM_BUFFER 512  /* maximum size of packets to be received for tcp */
#define TAM_ORDENES 20
#define PUERTO 21707
#define RETRIES 5  /* number of times to retry before givin up */
#define TIMEOUT 6
#define MAXCLIENTS 64

/* Error numbers */
#define ERR_NICKNAME 433
#define ERR_NOREGISTERED 460
#define ERR_NOVALIDUSER 461
#define ERR_ALREADYREGISTRED 462
#define ERR_ALREADYREGISTREDINCHANEL 463
#define ERR_NOSUCHNICK 401
#define ERR_NOREGISTEREDINCHANEL 402
#define ERR_NOSUCHCHANNEL 403


/*
 * Declaraciones de tipos de datos.
 */
#ifndef __NODE_DATA
#define __NODE_DATA
typedef void * nodeData;
#endif

#ifndef __NODE
#define __NODE
typedef struct Node {
	nodeData data;
	struct Node * next;
} Node;
#endif

#ifndef __LIST
#define __LIST
typedef struct List {
	Node * raiz;
	Node * ultimo;
} List;
#endif

#ifndef __ID_POSITION
#define __ID_POSITION
typedef Node * idPosition;
#endif


#ifndef __BUFFER_TYPES
#define __BUFFER_TYPES
typedef char buffer[TAM_BUFFER];
typedef char ordenes[10];
typedef char arg_1[10];
typedef char arg_2[256];
#endif

#ifndef __DATOS_HILO
#define __DATOS_HILO
typedef struct DatosHilo {
	int idSoc;
	int argc;
	char fichero[100];
} DatosHilo;
#endif

#ifndef __DATOS_HILO_SERVER
#define __DATOS_HILO_SERVER
typedef struct DatosHiloServer {
	int idSoc;
	struct sockaddr_in * addr;
	buffer buff;
	List *usuarios;
	List *canales;
} DatosHiloServer;
#endif

#ifndef __BASE_DATOS
#define __BASE_DATOS
typedef char nick[10];
typedef char nombre[200];

typedef struct datosUsuario {
	nick nickName;
	nombre nombreReal;
	int idSock;
	struct sockaddr_in * addr;
} datosUsuario;

typedef struct datosCanal {
	nombre nombreCanal;
	List nicks;
} datosCanal;
#endif


/*
 * Prototipos de funciones.
 */
void escribirFichero(const char * fichero, char * datos);
void leerFichero(const char * fichero, buffer ** datos, int * nRead);
char * timeString();


#endif
