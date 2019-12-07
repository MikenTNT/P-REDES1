#include <stdio.h>
#include <stdlib.h>

#include "lists.h"



int createEmpty(List *l)
{
	if ((l->raiz = l->ultimo = (Node *)malloc(sizeof(Node))) == NULL)
		return -1;
	else {
		l->raiz->data = NULL;
		l->raiz->next = NULL;

		return 0;
	}
}


int isEmpty(List *l)
{
	if (l->raiz == NULL)  return -1;

	return (l->raiz->next == NULL);
}


int destroy(List *l)
{
	if (l->raiz == NULL)  return -1;

	if (l->raiz->next != NULL) {
		removeAll(l);

		free(l->raiz);
		l->raiz = l->ultimo = NULL;
	} else {
		free(l->raiz);
		l->raiz = l->ultimo = NULL;
	}

	return 0;
}


idPosition previousPosition(List *l, idPosition p)
{
	idPosition anterior;

	if ((l->raiz == NULL) || (p == NULL))  return NULL;

	if (p == l->raiz)
		return l->raiz;
	else {
		anterior = l->raiz;
		while ((anterior->next != NULL) && (anterior->next != p))
			anterior = anterior->next;
		return anterior;
	}
}


idPosition nextPosition(List *l, idPosition p)
{
	if ((l->raiz == NULL) || (p == NULL))  return NULL;

	if ((l->raiz->next == NULL) || (p == l->ultimo))  return NULL;

	return p->next;
}


idPosition firstPosition(List *l)
{
	if (l->raiz == NULL)  return NULL;

	return l->raiz;
}


idPosition lastPosition(List *l)
{
	if (l->raiz == NULL)  return NULL;

	return l->ultimo;
}


idPosition getPosition(List *l, nodeData x)
{
	if (l->raiz == NULL)  return NULL;

	if (l->raiz->next == NULL)  return NULL;

	idPosition recorre = l->raiz->next;

	while ((recorre->data != x) && (recorre->next != NULL))
		recorre = recorre->next;

	return ((recorre->data == x) ? recorre : NULL);
}


nodeData getData(List *l, idPosition p)
{
	if (l->raiz == NULL || (p == NULL))  return NULL;

	if ((l->raiz->next == NULL) || (p == l->ultimo))  return NULL;

	return p->next->data;
}


int insertAt(List *l, nodeData x, idPosition p)
{
	if ((l->raiz == NULL) || (p == NULL))  return -1;

	idPosition nueva;

	if ((nueva = (Node *)malloc(sizeof(Node))) == NULL)
		return -3;

	nueva->data = x;
	nueva->next = p->next;
	p->next = nueva;

	if (p == l->ultimo)  l->ultimo = nueva;

	return 0;
}


int removeAt(List *l, idPosition p)
{
	if ((l->raiz == NULL) || (p == NULL))  return -1;

	if ((l->raiz->next == NULL) || (p == l->ultimo))  return -2;

	idPosition sigCelda;

	if (p->next != l->ultimo) {
		sigCelda = p->next->next;
		free(p->next->data);
		free(p->next);
		p->next = sigCelda;
	} else {
		l->ultimo = p;
		free(p->next->data);
		free(p->next);
		p->next = NULL;
	}

	return 0;
}


int removeAll(List *l)
{
	if (l->raiz == NULL)  return -1;

	idPosition aBorrar;

	while(l->raiz->next != NULL) {
		aBorrar = l->raiz->next;
		l->raiz->next = l->raiz->next->next;
		free(aBorrar->data);
		free(aBorrar);
	}

	return 0;
}
