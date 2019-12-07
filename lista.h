#ifndef __LISTA_H
#define __LISTA_H

/*
 * Includes.
 */
#include "utils.h"

/*
 * Prototipos de funciones.
 */
int createEmpty(Lista *l);
int isEmpty(Lista *l);
int destroy(Lista *l);

tipoPosicion previousPosition(Lista *l, tipoPosicion p);
tipoPosicion nextPosition(Lista *l, tipoPosicion p);
tipoPosicion firstPosition(Lista *l);
tipoPosicion lastPosition(Lista *l);

int insertAt(Lista *l, tipoElemento x, tipoPosicion p);
int removeAt(Lista *l, tipoPosicion p);
int removeAll(Lista *l);

tipoPosicion getPosition(Lista *l, tipoElemento x);
tipoElemento getElement(Lista *l, tipoPosicion p);


#endif
