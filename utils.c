/*
** Fichero: utils.c
** Autores:
** Carlos Manjón García DNI 70908545M
** Miguel Sánchez González DNI 70921138V
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"



void escribirFichero(const char * fichero, char * datos)
{
	FILE *fp;

	if ((fp = fopen(fichero, "a")) == NULL)
	{
		fprintf(stderr, "Error: No se puede abrir el fichero %s\n", fichero);
		exit(1);
	}

	fprintf(fp, "%s\n", datos);

	fclose(fp);

	return;
}


void leerFichero(const char * fichero, buffer ** datos, int * nRead)
{
	FILE *fp;
	char c;

	if ((fp = fopen(fichero, "r")) == NULL)
	{
		fprintf(stderr, "Error: No se puede abrir el fichero %s\n", fichero);
		exit(1);
	}

	c = fgetc(fp);
	while (c != EOF) {
		if (c == '\n')
			(*nRead)++;

		c = fgetc(fp);
	}

	rewind(fp);

	if (NULL == (*datos = (buffer *)malloc((*nRead) * TAM_BUFFER)))
	{
		fprintf(stderr, "Client: unable to allocate memory\n");
		exit(1);
	}

	int i = 0;
	while (fscanf(fp, "%[^\n]%*c", (*datos)[i++]) != EOF);

	fclose(fp);

	return;
}


char * timeString()
{
	time_t timeVar;
	time(&timeVar);
	return (char *)ctime(&timeVar);
}
