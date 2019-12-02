#ifndef __CLIENTE_H
#define __CLIENTE_H


#include "utils.h"


#define RETRIES 5  /* number of times to retry before givin up */
#define TIMEOUT 6


void handler();
void nick(char * nickName);
void user(char * username);
void mensajes(char * receptor, char * mensaje);
void join(char * canal);
void part(char * canal);
void quit(char * mensaje);


#endif
