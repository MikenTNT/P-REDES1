#ifndef __UTILS_H
#define __UTILS_H


#define ADDRNOTFOUND 0xffffffff  /* value returned for unknown host */
#define HOSTLEN 512
#define TAM_BUFFER 512  /* maximum size of packets to be received for tcp */
#define TAM_ORDENES 20
#define PUERTO 21707
#define BUFFERSIZE 1024  /* maximum size of packets to be received */
#define RETRIES 5  /* number of times to retry before givin up */
#define TIMEOUT 6

/* Error numbers */
#define ERR_NICKNAME 433
#define ERR_ALREADYREGISTRED 462
#define ERR_NOSUCHNICK 401
#define ERR_NOREGISTEREDINCHANEL 402
#define ERR_NOSUCHCHANNEL 403


/*
 * Declaraciones de tipos de datos.
 */
#ifndef __TIPO_ELEMENTO
#define __TIPO_ELEMENTO
typedef void * tipoElemento;
#endif

#ifndef __TIPO_CELDA
#define __TIPO_CELDA
typedef struct tipoCelda {
	tipoElemento elemento;
	struct tipoCelda *sig;
} tipoCelda;
typedef tipoCelda * tipoCeldaRef;
#endif

#ifndef __TIPO_LISTA
#define __TIPO_LISTA
typedef struct Lista {
	tipoCeldaRef raiz;
	tipoCeldaRef ultimo;
} Lista;
#endif

#ifndef __TIPO_POS
#define __TIPO_POS
typedef tipoCelda * tipoPosicion;
#endif

/*
 * Prototipos de funciones.
 */
#ifndef __BUFFER
#define __BUFFER
typedef char buffer[TAM_BUFFER];
#endif

#ifndef __DATOS_HILO
#define __DATOS_HILO
typedef struct datosHilo {
	int idSoc;
	char * argv;
} datosHilo;
#endif

#ifndef __BASE_DATOS
#define __BASE_DATOS
typedef char nick[10];
typedef char nombre[200];

typedef struct datosUsuario {
	nick nickName;
	nombre nombreReal;
} datosUsuario;

typedef struct datosCanal {
	nombre nombreCanal;
	Lista * nicks;
} datosCanal;
#endif


/*
 * Prototipos de funciones.
 */
void escribirFichero(const char * fichero, char * datos);
void leerFichero(const char * fichero, buffer ** datos);
char * timeString();


#endif