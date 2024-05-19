#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct {
   int sd_client;
   char ip_address[INET_ADDRSTRLEN];
} Argumentos_thread;

typedef struct {
    char nombre_fichero[256];
    char descripcion[256];
    char direccion_ip[INET_ADDRSTRLEN];
    int puerto_cliente;
} RespuestaUsuarios;

typedef struct {
    char nombre_peticion [256];
    char nombre_usuario [256];
    char nombre_fichero[256];
    char fecha_hora [256];
    int op_retorno_rpc;
} Peticion_RPC;

#endif
