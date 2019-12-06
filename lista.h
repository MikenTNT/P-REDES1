#ifndef __LISTA_H
#define __LISTA_H

/*
 * Includes.
 */
#include "utils.h"

/*
 * Prototipos de funciones.
 */
int creaVacia(Lista *l);
tipoPosicion primero(Lista *l);
tipoPosicion siguiente(Lista *l, tipoPosicion p);
tipoPosicion anterior(Lista *l, tipoPosicion p);
tipoPosicion fin(Lista *l);

int inserta(Lista *l, tipoElemento x, tipoPosicion p);
int suprime(Lista *l, tipoPosicion p);
int anula(Lista *l);

int vacia(Lista *l);
tipoPosicion localiza(Lista *l, tipoElemento x);
tipoElemento recupera(Lista *l, tipoPosicion p);
int destruye(Lista *l);


#endif
