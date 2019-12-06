#ifndef __SERVIDOR_H
#define __SERVIDOR_H

/*
 * Includes.
 */
#include "utils.h"
#include "lista.h"

/*
 * Prototipos de funciones.
 */
void serverTCP(int idSoc, struct sockaddr_in peeraddr_in, Lista * usuarios, Lista * canales);
void serverUDP(int idSoc, char * buf, struct sockaddr_in clientaddr_in);

void finalizar();

int nickOrd(nick nickName, Lista * usuarios);
int userOrd(nick nickName, nombre nombreReal, Lista * usuarios);
int mensajesOrd(char * receptor, char * mensaje, Lista * usuarios, Lista * canales);
int joinOrd(nick nickName, nombre canal, Lista * usuarios, Lista * canales);
int partOrd(nick nickName, nombre canal, char * mensaje, Lista * canales);
int quitOrd(char * mensaje, Lista * usuarios, Lista * canales);


#endif