#ifndef __SERVIDOR_H
#define __SERVIDOR_H

/*
 * Includes.
 */
#include "utils.h"
#include "lists.h"

/*
 * Prototipos de funciones.
 */
void serverTCP(int idSoc, struct sockaddr_in peeraddr_in, List * usuarios, List * canales);
void serverUDP(int idSoc, char * buf, struct sockaddr_in clientaddr_in);

void finalizar();

int nickOrd(nick nickName, List * usuarios);
int userOrd(nick nickName, nick usuario, nombre nombreReal, List * usuarios);
int mensajesOrd(nick nickName, char * receptor, char * mensaje, List * usuarios, List * canales);
int joinOrd(nick nickName, nombre canal, List * usuarios, List * canales);
int partOrd(nick nickName, nombre canal, char * mensaje, List * canales);
int quitOrd(nick nickName, char * mensaje, List * usuarios, List * canales);


#endif