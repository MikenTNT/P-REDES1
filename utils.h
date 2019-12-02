#ifndef __UTILS_H
#define __UTILS_H


#define ADDRNOTFOUND 0xffffffff  /* value returned for unknown host */
#define HOSTLEN 512
#define TAM_BUFFER 10  /* maximum size of packets to be received for tcp */
#define TAM_ORDENES 20
#define PUERTO 8545


typedef char buffer[TAM_BUFFER];


void escribirFichero(const char * fichero, char * datos);
void leerFichero(const char * fichero, buffer ** datos);
char * timeString();


#endif