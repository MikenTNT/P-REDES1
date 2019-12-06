#include <stdio.h>
#include <stdlib.h>

#include "lista.h"
#include "utils.h"



int creaVacia(Lista *l)
{
	if (NULL == (l->raiz = l->ultimo = (tipoCeldaRef)malloc(sizeof(tipoCelda))))
		return -1;
	else {
		l->raiz->sig = NULL;

		return 0;
	}
}


int vacia(Lista *l)
{
	if (l->raiz == NULL)  return -1;

	return ((l->raiz->sig == NULL) ? 1 : 0);
}


int destruye(Lista *l)
{
	if (l->raiz == NULL)  return -1;

	if (l->raiz->sig != NULL)
		return -2;
	else {
		free(l->raiz);
		l->raiz = l->ultimo = NULL;

		return 0;
	}
}


tipoPosicion anterior(Lista *l, tipoPosicion p)
{
	tipoPosicion anterior;

	if ((l->raiz == NULL) || (p == NULL))  return NULL;

	if (p == l->raiz)
		return l->raiz;
	else {
		anterior = l->raiz;
		while ((anterior->sig != NULL) && (anterior->sig != p))
			anterior = anterior->sig;
		return anterior;
	}
}


tipoPosicion primero(Lista *l)
{
	if (l->raiz == NULL)  return NULL;

	return l->raiz;
}


tipoPosicion fin(Lista *l)
{
	if (l->raiz == NULL)  return NULL;

	return l->ultimo;
}


int inserta(Lista *l, tipoElemento x, tipoPosicion p)
{
	if ((l->raiz == NULL) || (p == NULL))  return -1;

	tipoPosicion nueva;

	if (NULL == (nueva = (tipoCeldaRef)malloc(sizeof(tipoCelda))))
		return -3;

	nueva->elemento = x;
	nueva->sig = p->sig;
	p->sig = nueva;

	if (p == l->ultimo)  l->ultimo = nueva;

	return 0;
}


int suprime(Lista *l, tipoPosicion p)
{
	if ((l->raiz == NULL) || (p == NULL))  return -1;

	if ((l->raiz->sig == NULL) || (p == l->ultimo))  return -2;

	tipoPosicion sigCelda;

	if (p->sig != l->ultimo) {
		sigCelda = p->sig->sig;
		free(p->sig);
		p->sig = sigCelda;
	} else {
		l->ultimo = p;
		free(p->sig);
		p->sig = NULL;
	}

	return 0;
}


tipoPosicion siguiente(Lista *l, tipoPosicion p)
{
	if ((l->raiz == NULL) || (p == NULL))  return NULL;

	if ((l->raiz->sig == NULL) || (p == l->ultimo))  return NULL;

	return p->sig;
}


tipoPosicion localiza(Lista *l, tipoElemento x)
{
	if (l->raiz == NULL)  return NULL;

	if (l->raiz->sig == NULL)  return NULL;

	tipoPosicion recorre = l->raiz->sig;

	while ((recorre->elemento != x) && (recorre->sig != NULL))
		recorre = recorre->sig;

	return ((recorre->elemento == x) ? recorre : NULL);
}


tipoElemento recupera(Lista *l, tipoPosicion p)
{
	if (l->raiz == NULL || (p == NULL))  return NULL;

	if ((l->raiz->sig == NULL) || (p == l->ultimo))  return NULL;

	return p->sig->elemento;
}


int anula(Lista *l)
{
	if (l->raiz == NULL)  return -1;

	tipoPosicion aBorrar;

	while(l->raiz->sig != NULL) {
		aBorrar = l->raiz->sig;
		l->raiz->sig = l->raiz->sig->sig;
		free(aBorrar);
	}

	return 0;
}
