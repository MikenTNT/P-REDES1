# MAKEFILE DE PROYECTO C,  rev 6.3
# Autor: MikenTNT  02-DEC-2019.

SHELL = /bin/bash

#-----------------------------------AJUSTES-------------------------------------

# Compilador de C 'gcc'.
CC = gcc-8
# CFLAGS: depurador '-g', matematicas '-lm'.
CFLAGS = -Wall
LIBS = -lpthread

#----------------------------------VARIABLES------------------------------------

PROGS = run/servidor run/cliente

# Phony targets.
PHONY := all PROGS clean tar untar

#----------------------------OBJETIVOS PRINCIPALES------------------------------

# Objetivos a ejecutar con el comando make.
all: $(PROGS)


run/servidor: servidor.o utils.o lists.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

run/cliente: cliente.o utils.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compilacion general de archivos.
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

#-------------------------------OTROS OBJETIVOS---------------------------------

# Objetivo para limpieza.
clean:
	rm -rf *.o $(PROGS) proyecto.tar run/logs/*

# Objetivo para comprimir.
tar: makefile
	tar -cvf proyecto.tar $^

# Objetivo para descomprimir.
untar: proyecto.tar
	tar -xvf $<

#-------------------------------------------------------------------------------

.PHONY: $(PHONY)
