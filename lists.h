/*
** Fichero: lists.h
** Autores:
** Carlos Manjón García DNI 70908545M
** Miguel Sánchez González DNI 70921138V
*/
#ifndef __LISTS_H
#define __LISTS_H


/*
 * Includes.
 */
#include "utils.h"


/*
 * Function prototypes.
 */
int createEmpty(List *l);
int isEmpty(List *l);
int destroy(List *l);

idPosition previousPosition(List *l, idPosition p);
idPosition nextPosition(List *l, idPosition p);
idPosition firstPosition(List *l);
idPosition lastPosition(List *l);

idPosition getPosition(List *l, nodeData x);
nodeData getData(List *l, idPosition p);

int insertAt(List *l, nodeData x, idPosition p);
int removeAt(List *l, idPosition p);
int removeAll(List *l);


#endif
