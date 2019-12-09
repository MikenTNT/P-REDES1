/*
** Fichero: servidor.h
** Autores:
** Carlos Manjón García DNI 70908545M
** Miguel Sánchez González DNI 70921138V
*/
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
void serverUDP(int idSoc, char * buf, struct sockaddr_in clientaddr_in, List * usuarios, List * canales);

void finalizar();

int nickOrd(nick nickName, List * usuarios, struct sockaddr_in *);
int userOrd(nick nickName, nick usuario, nombre nombreReal, List * usuarios);
int mensajesOrd(nick nickName, char * receptor, char * mensaje, List * usuarios, List * canales);
int joinOrd(nick nickName, nombre canal, List * usuarios, List * canales);
int partOrd(nick nickName, nombre canal, char * mensaje, List * canales);
int quitOrd(nick nickName, char * mensaje, List * usuarios, List * canales);

void dividirBuffer(buffer * cadena, ordenes * orden, arg_1 * arg1, arg_2 * arg2);


#endif