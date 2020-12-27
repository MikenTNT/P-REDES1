/*
** Fichero: servidor.h
** Autores:
** Carlos Manjón García
** Miguel Sánchez González
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
void * serverTCP(void *datos);
void * serverUDP(void *datos);

void handler();
void finalizar();

int nickOrd(nick nickName, struct sockaddr_in *, int idSoc, List * usuarios);
int userOrd(nick nickName, nick usuario, nombre nombreReal, List * usuarios);
int mensajesOrd(nick nickName, char * receptor, char * mensaje, List * usuarios, List * canales);
int mensajesOrdUDP(nick nickName, char * receptor, char * mensaje, List * usuarios, List * canales);
int joinOrd(nick nickName, nombre canal, List * canales);
int partOrd(nick nickName, nombre canal, char * mensaje, List * canales);
int quitOrd(nick nickName, char * mensaje, List * usuarios, List * canales);

void dividirBuffer(buffer * cadena, ordenes * orden, arg_1 * arg1, arg_2 * arg2);


#endif
