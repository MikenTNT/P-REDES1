#ifndef __SERVIDOR_H
#define __SERVIDOR_H


#include "utils.h"


#define BUFFERSIZE 1024  /* maximum size of packets to be received */


void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char * buf, struct sockaddr_in clientaddr_in);

void finalizar();


#endif