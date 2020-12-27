/*
** Fichero: cliente.h
** Autores:
** Carlos Manjón García
** Miguel Sánchez González
*/
#ifndef __CLIENTE_H
#define __CLIENTE_H

/*
 * Includes.
 */
#include "utils.h"

/*
 * Prototipos de funciones.
 */
void handler();
void finalizar();

void * recibirTCP(void * d);
void * recibirUDP(void * d);


#endif
