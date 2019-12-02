#ifndef __SERVIDOR_H
#define __SERVIDOR_H


#include "utils.h"


#define BUFFERSIZE 1024  /* maximum size of packets to be received */

/* Error numbers */
#define ERR_NICKNAME 433
#define ERR_ALREADYREGISTRED 462
#define ERR_NOSUCHNICK 401
#define ERR_NOSUCHCHANNEL 403


void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char * buf, struct sockaddr_in clientaddr_in);

void finalizar();

void nick(char * nickName);
void user(char * username);
void mensajes(char * receptor, char * mensaje);
void join(char * canal);
void part(char * canal);
void quit(char * mensaje);


#endif