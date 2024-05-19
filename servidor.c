#include <strings.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <asm-generic/socket.h>

#include "data.h"
#include "message.h"
#include "servidor.h"
#include "rpc_management.h"

#define MAX_STRING_LEN 256
#define MAX_FILELINE_LEN 1000

// Mutex de la copia del mensaje de peticion
pthread_mutex_t mutex_mensaje;
// Valor booleano que indica si una peticion ha sido copiada o no
int mensaje_no_copiado = 1;
// Variable condicion
pthread_cond_t cond_mensaje;
// Manejador de la signal para salir del bucle
int do_exit = 0;
// Descriptor de socket del servidor
int sd_server;
// Manejador del signal al hacer ctrl + c para terminar el servidor
void handler_sigint()
{
    do_exit = 1;
    printf("Saliendo...\n");
    close(sd_server);
    pthread_mutex_destroy(&mutex_mensaje);
    pthread_cond_destroy(&cond_mensaje);
    exit(0);
}
/* Tratamiento de la peticion por parte de cada hilo */
void *tratar_peticion(void *sd_args)
{
    // printf("A ber\n");
    Peticion pet;
    char filename[512]; // Espacio adicional por si se necesita algun path mas largo
    char directorio_registrados[20] = "usuarios-registrados";
    char directorio_conectados[20] = "usuarios-conectados";
    char directorio_publicaciones[20] = "publicaciones";
    char buffer[MAX_STRING_LEN]; // Bufer de mensajes para el socket servidor
    pthread_mutex_lock(&mutex_mensaje);
    // Copia local de la IP y el descriptor de socket
    Argumentos_thread *sd_params = (Argumentos_thread *)sd_args;
    int sd_client = sd_params->sd_client;
    char *ip_client = sd_params->ip_address;
    mensaje_no_copiado = 0;
    pthread_cond_signal(&cond_mensaje);
    pthread_mutex_unlock(&mutex_mensaje);

    int ret;
    // Leer la operacion del cliente
    if (readLine(sd_client, pet.codigo_operacion, MAX_STRING_LEN) == -1)
    {
        perror("Error al leer la operación del cliente");
        exit(EXIT_FAILURE);
    }

    /* Casos segun la operacion recibida */
    if (!(strcmp(pet.codigo_operacion, "REGISTER")))
    {
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.fecha_hora, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.username, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        printf("s> OPERATION FROM %s\n", pet.username);
        CLIENT *clnt;
        enum clnt_stat retval;
        int ret;
        char host[MAX_STRING_LEN] = "127.0.0.1";
        // Inicializar RPC
        clnt = clnt_create(host, NANODT, NANODT_VERSION, "tcp");
        if (clnt == NULL)
        {
            clnt_pcreateerror(host);
            exit(EXIT_FAILURE);
        }
        Peticion_server peticion_rpc;
        strcpy(peticion_rpc.fecha_hora, pet.fecha_hora);
        strcpy(peticion_rpc.nombre_peticion, pet.codigo_operacion);
        strcpy(peticion_rpc.nombre_usuario, pet.username);
        retval = d_print_nofile_1(peticion_rpc, &ret, clnt);
        if (retval != RPC_SUCCESS)
        {
            clnt_perror(clnt, "d_print_nofile_1: ");
            clnt_destroy(clnt);
            exit(EXIT_FAILURE);
        }
        clnt_destroy(clnt);

        sprintf(filename, "%s/%s", directorio_registrados, pet.username);
        // Se va a crear si no existe, un fichero dentro del directorio que lleva la cuenta de usuarios registrados que lo identifique por su username
        FILE *fichero_exist = fopen(filename, "r");
        if (fichero_exist != NULL)
        {
            // El archivo ya existe, enviamos un codigo de retorno 1 al cliente
            pet.op_retorno = 1;
            fclose(fichero_exist);
        }
        else
        {
            // El archivo no existe, lo creamos y enviamos un retorno de exito 0 al cliente
            FILE *fichero = fopen(filename, "w");
            if (fichero == NULL)
            {
                perror("Error al crear el archivo de sesión");
                exit(EXIT_FAILURE);
            }
            fclose(fichero);
            pet.op_retorno = 0;
        }
        // Enviar la operacion de retorno finalmente
        sprintf(buffer, "%d", pet.op_retorno);
        ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
        if (ret == -1)
        {
            perror("Error al enviar la respuesta al cliente");
            exit(EXIT_FAILURE);
        }
    }
    else if (!(strcmp(pet.codigo_operacion, "UNREGISTER")))
    {
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.fecha_hora, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.username, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        printf("s> OPERATION FROM %s\n", pet.username);
        CLIENT *clnt;
        enum clnt_stat retval;
        int ret;
        char host[MAX_STRING_LEN] = "127.0.0.1";
        // Inicializar RPC
        clnt = clnt_create(host, NANODT, NANODT_VERSION, "tcp");
        if (clnt == NULL)
        {
            clnt_pcreateerror(host);
            exit(EXIT_FAILURE);
        }
        Peticion_server peticion_rpc;
        strcpy(peticion_rpc.fecha_hora, pet.fecha_hora);
        strcpy(peticion_rpc.nombre_peticion, pet.codigo_operacion);
        strcpy(peticion_rpc.nombre_usuario, pet.username);
        retval = d_print_nofile_1(peticion_rpc, &ret, clnt);
        if (retval != RPC_SUCCESS)
        {
            clnt_perror(clnt, "d_print_nofile_1: ");
            clnt_destroy(clnt);
            exit(EXIT_FAILURE);
        }
        clnt_destroy(clnt);
        // Comoprobar si antes que nada existe un usuario registrado con ese nombre o no (error)
        sprintf(filename, "%s/%s", directorio_registrados, pet.username);
        FILE *fichero = fopen(filename, "r");
        if (fichero == NULL)
        {
            pet.op_retorno = 1;
        }
        else
        {
            fclose(fichero);
            remove(filename);
            pet.op_retorno = 0;
        }
        sprintf(buffer, "%d", pet.op_retorno);
        ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
        if (ret == -1)
        {
            perror("Error al enviar la respuesta al cliente");
            exit(EXIT_FAILURE);
        }
    }
    else if (!(strcmp(pet.codigo_operacion, "CONNECT")))
    {
        FILE *fichero_registro, *fichero_conexion;
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.fecha_hora, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer la fecha del cliente");
            exit(EXIT_FAILURE);
        }
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.username, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        printf("s> OPERATION FROM %s\n", pet.username);
        CLIENT *clnt;
        enum clnt_stat retval;
        int ret;
        char host[MAX_STRING_LEN] = "127.0.0.1";
        // Inicializar RPC
        clnt = clnt_create(host, NANODT, NANODT_VERSION, "tcp");
        if (clnt == NULL)
        {
            clnt_pcreateerror(host);
            exit(EXIT_FAILURE);
        }
        Peticion_server peticion_rpc;
        strcpy(peticion_rpc.fecha_hora, pet.fecha_hora);
        strcpy(peticion_rpc.nombre_peticion, pet.codigo_operacion);
        strcpy(peticion_rpc.nombre_usuario, pet.username);
        retval = d_print_nofile_1(peticion_rpc, &ret, clnt);
        if (retval != RPC_SUCCESS)
        {
            clnt_perror(clnt, "d_print_nofile_1: ");
            clnt_destroy(clnt);
            exit(EXIT_FAILURE);
        }
        clnt_destroy(clnt);
        // Comoprobar si antes que nada existe un usuario registrado con ese nombre o no (error)
        sprintf(filename, "%s/%s", directorio_registrados, pet.username);
        fichero_registro = fopen(filename, "r");
        if (fichero_registro == NULL)
        {
            // Usuario registrado
            pet.op_retorno = 1;
        }
        else
        {
            // Leer el puerto proporcionado por el cliente con el que este usuario escucha peticiones de otros clientes
            if (readLine(sd_client, buffer, MAX_STRING_LEN) == -1)
            {
                perror("Error al leer el nombre de usuario del cliente");
                exit(EXIT_FAILURE);
            }
            pet.puerto = atoi(buffer);
            // Crear si no existe, un fichero en el directorio que lleval la cuenta de los conectados con el username identificativo
            sprintf(filename, "%s/%s", directorio_conectados, pet.username);
            fichero_conexion = fopen(filename, "r");
            if (fichero_conexion != NULL)
            {
                // Usuario conectado
                pet.op_retorno = 2;
                fclose(fichero_conexion);
            }
            else
            {
                fichero_conexion = fopen(filename, "w");
                // Guardar en el fichero la IP y puerto del cliente que ha solicitado la conexion
                fprintf(fichero_conexion, "%s %d", ip_client, pet.puerto);
                pet.op_retorno = 0;
                fclose(fichero_conexion);
            }
        }
        sprintf(buffer, "%d", pet.op_retorno);
        ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
        if (ret == -1)
        {
            perror("Error al enviar la respuesta al cliente");
            exit(EXIT_FAILURE);
        }
    }
    else if (!(strcmp(pet.codigo_operacion, "DISCONNECT")))
    {
        FILE *fichero_registro, *fichero_conexion;
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.fecha_hora, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer la fecha del cliente");
            exit(EXIT_FAILURE);
        }
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.username, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        printf("s> OPERATION FROM %s\n", pet.username);

        CLIENT *clnt;
        enum clnt_stat retval;
        int ret;
        char host[MAX_STRING_LEN] = "127.0.0.1";
        // Inicializar RPC
        clnt = clnt_create(host, NANODT, NANODT_VERSION, "tcp");
        if (clnt == NULL)
        {
            clnt_pcreateerror(host);
            exit(EXIT_FAILURE);
        }
        Peticion_server peticion_rpc;
        strcpy(peticion_rpc.fecha_hora, pet.fecha_hora);
        strcpy(peticion_rpc.nombre_peticion, pet.codigo_operacion);
        strcpy(peticion_rpc.nombre_usuario, pet.username);
        retval = d_print_nofile_1(peticion_rpc, &ret, clnt);
        if (retval != RPC_SUCCESS)
        {
            clnt_perror(clnt, "d_print_nofile_1: ");
            clnt_destroy(clnt);
            exit(EXIT_FAILURE);
        }
        clnt_destroy(clnt);
        sprintf(filename, "%s/%s", directorio_registrados, pet.username);
        fichero_registro = fopen(filename, "r");
        // No existe usuario registrado
        if (fichero_registro == NULL)
        {
            pet.op_retorno = 1;
        }
        else
        {
            sprintf(filename, "%s/%s", directorio_conectados, pet.username);
            fichero_conexion = fopen(filename, "r");
            // No existe usuario conectado
            if (fichero_conexion == NULL)
            {
                pet.op_retorno = 2;
            }
            else
            {
                // Borrar el fichero de la lista de conectados para ese usuario
                remove(filename);
                pet.op_retorno = 0;
            }
        }
        sprintf(buffer, "%d", pet.op_retorno);
        ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
        if (ret == -1)
        {
            perror("Error al enviar la respuesta al cliente");
            exit(EXIT_FAILURE);
        }
    }
    else if (!(strcmp(pet.codigo_operacion, "PUBLISH")))
    {
        // Descriptores de fichero para comprobar que esta registrado, conectado, y que no haya publicado ya un fichero con ese nombre
        FILE *fichero_registro, *fichero_conexion, *fichero_exist, *fichero_publicacion;
        Contenido publicacion;
        // Recibir el username, nombre de fichero a publicar y una breve descripcion
        if (readLine(sd_client, pet.fecha_hora, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        if (readLine(sd_client, pet.username, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        printf("s> OPERATION FROM %s\n", pet.username);

        if (readLine(sd_client, publicacion.nombre_fichero, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre del fichero a publicar");
            exit(EXIT_FAILURE);
        }
        if (readLine(sd_client, buffer, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer la descripción del fichero a publicar");
            exit(EXIT_FAILURE);
        }
        CLIENT *clnt;
        enum clnt_stat retval;
        int ret;
        char host[MAX_STRING_LEN] = "127.0.0.1";
        // Inicializar RPC
        clnt = clnt_create(host, NANODT, NANODT_VERSION, "tcp");
        if (clnt == NULL)
        {
            clnt_pcreateerror(host);
            exit(EXIT_FAILURE);
        }
        Peticion_server peticion_rpc;
        strcpy(peticion_rpc.fecha_hora, pet.fecha_hora);
        strcpy(peticion_rpc.nombre_peticion, pet.codigo_operacion);
        strcpy(peticion_rpc.nombre_fichero, publicacion.nombre_fichero);
        strcpy(peticion_rpc.nombre_usuario, pet.username);
        retval = d_print_file_1(peticion_rpc, &ret, clnt);
        if (retval != RPC_SUCCESS)
        {
            clnt_perror(clnt, "d_print_file_1: ");
            clnt_destroy(clnt);
            exit(EXIT_FAILURE);
        }
        clnt_destroy(clnt);
        strcpy(publicacion.descripcion_fichero, buffer);
        sprintf(filename, "%s/%s", directorio_registrados, pet.username);
        fichero_registro = fopen(filename, "r");
        if (fichero_registro == NULL)
        {
            // Usuario no registrado
            pet.op_retorno = 1;
        }
        else
        {
            sprintf(filename, "%s/%s", directorio_conectados, pet.username);
            fichero_conexion = fopen(filename, "r");
            if (fichero_conexion == NULL)
            {
                // Usuario no conectado
                pet.op_retorno = 2;
            }
            else
            {

                sprintf(filename, "%s/%s", directorio_publicaciones, publicacion.nombre_fichero);
                fichero_exist = fopen(filename, "r");
                if (fichero_exist != NULL)
                {
                    // Fichero ya publicado
                    pet.op_retorno = 3;
                    fclose(fichero_exist);
                }
                else
                {
                    // Crear fichero truncado, y aunque no contiene su contenido, si que se le imprime el usuario y descripcion asociados a el
                    fichero_publicacion = fopen(filename, "w");
                    fprintf(fichero_publicacion, "%s %s\n", pet.username, publicacion.descripcion_fichero);
                    fclose(fichero_publicacion);
                    pet.op_retorno = 0;
                }
                fclose(fichero_conexion);
            }
            fclose(fichero_registro);
        }
        sprintf(buffer, "%d", pet.op_retorno);
        ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
        if (ret == -1)
        {
            perror("Error al enviar la respuesta al cliente");
            exit(EXIT_FAILURE);
        }
    }
    else if (!(strcmp(pet.codigo_operacion, "DELETE")))
    {
        FILE *fichero_registro, *fichero_conexion, *fichero_exist, *fichero_desc;
        Contenido publicacion;

        if (readLine(sd_client, pet.fecha_hora, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        if (readLine(sd_client, pet.username, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        if (readLine(sd_client, publicacion.nombre_fichero, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre del fichero a publicar");
            exit(EXIT_FAILURE);
        }
        printf("s> OPERATION FROM %s\n", pet.username);

        CLIENT *clnt;
        enum clnt_stat retval;
        int ret;
        char host[MAX_STRING_LEN] = "127.0.0.1";
        // Inicializar RPC
        clnt = clnt_create(host, NANODT, NANODT_VERSION, "tcp");
        if (clnt == NULL)
        {
            clnt_pcreateerror(host);
            exit(EXIT_FAILURE);
        }
        Peticion_server peticion_rpc;
        strcpy(peticion_rpc.fecha_hora, pet.fecha_hora);
        strcpy(peticion_rpc.nombre_peticion, pet.codigo_operacion);
        strcpy(peticion_rpc.nombre_fichero, publicacion.nombre_fichero);
        strcpy(peticion_rpc.nombre_usuario, pet.username);
        retval = d_print_file_1(peticion_rpc, &ret, clnt);
        if (retval != RPC_SUCCESS)
        {
            clnt_perror(clnt, "d_print_file_1: ");
            clnt_destroy(clnt);
            exit(EXIT_FAILURE);
        }
        clnt_destroy(clnt);
        sprintf(filename, "%s/%s", directorio_registrados, pet.username);
        fichero_registro = fopen(filename, "r");
        if (fichero_registro == NULL)
        {
            // Usuario no registrado
            pet.op_retorno = 1;
        }
        else
        {
            sprintf(filename, "%s/%s", directorio_conectados, pet.username);
            fichero_conexion = fopen(filename, "r");
            if (fichero_conexion == NULL)
            {
                // Usuario no conectado
                pet.op_retorno = 2;
            }
            else
            {
                sprintf(filename, "%s/%s", directorio_publicaciones, publicacion.nombre_fichero);
                fichero_exist = fopen(filename, "r");
                if (fichero_exist == NULL)
                {
                    // No existe un fichero para esa publicacion
                    pet.op_retorno = 3;
                }
                else
                {
                    fclose(fichero_exist);
                    remove(filename);
                    sprintf(filename, "%s/%s-description", directorio_publicaciones, publicacion.nombre_fichero);
                    fichero_desc = fopen(filename, "a");
                    if (fichero_desc == NULL)
                    {
                        // Manejador de error para lectura/escritura
                        perror("Error al abrir el descriptor");
                        exit(EXIT_FAILURE);
                    }
                    fclose(fichero_desc);
                    remove(filename);
                    pet.op_retorno = 0;
                }
                fclose(fichero_conexion);
            }
            fclose(fichero_registro);
        }
        sprintf(buffer, "%d", pet.op_retorno);
        ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
        if (ret == -1)
        {
            perror("Error al enviar la respuesta al cliente");
            exit(EXIT_FAILURE);
        }
    }
    else if (!(strcmp(pet.codigo_operacion, "LIST_CONTENT")))
    {
        FILE *fichero_registro, *fichero_conexion;
        // Array dinamico en el que se almacenaran todas las publicaciones para ese usuario recibido
        RespuestaUsuarios *respuestas = NULL;
        int num_respuestas = 0;
        // Descriptor de directorio para las publicaciones
        DIR *dir;
        struct dirent *ent; // Entrada de directorio
        char username_leido[MAX_STRING_LEN], descripcion_leida[MAX_STRING_LEN], remote_username[MAX_STRING_LEN];
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.fecha_hora, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.username, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        if (readLine(sd_client, remote_username, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario remoto del cliente");
            exit(EXIT_FAILURE);
        }
        printf("s> OPERATION FROM %s\n", pet.username);

        CLIENT *clnt;
        enum clnt_stat retval;
        int ret;
        char host[MAX_STRING_LEN] = "127.0.0.1";
        // Inicializar RPC
        clnt = clnt_create(host, NANODT, NANODT_VERSION, "tcp");
        if (clnt == NULL)
        {
            clnt_pcreateerror(host);
            exit(EXIT_FAILURE);
        }
        Peticion_server peticion_rpc;
        strcpy(peticion_rpc.fecha_hora, pet.fecha_hora);
        strcpy(peticion_rpc.nombre_peticion, pet.codigo_operacion);
        strcpy(peticion_rpc.nombre_usuario, pet.username);
        retval = d_print_nofile_1(peticion_rpc, &ret, clnt);
        if (retval != RPC_SUCCESS)
        {
            clnt_perror(clnt, "d_print_nofile_1: ");
            clnt_destroy(clnt);
            exit(EXIT_FAILURE);
        }
        clnt_destroy(clnt);
        sprintf(filename, "%s/%s", directorio_registrados, pet.username);
        fichero_registro = fopen(filename, "r");
        if (fichero_registro == NULL)
        {
            // Usuario no registrado
            pet.op_retorno = 1;
            sprintf(buffer, "%d", pet.op_retorno);
            ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
            if (ret == -1)
            {
                perror("Error al enviar la respuesta al cliente");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            sprintf(filename, "%s/%s", directorio_conectados, pet.username);
            fichero_conexion = fopen(filename, "r");
            if (fichero_conexion == NULL)
            {
                // Usuario no conectado
                pet.op_retorno = 2;
                sprintf(buffer, "%d", pet.op_retorno);
                ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
                if (ret == -1)
                {
                    perror("Error al enviar la respuesta al cliente");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                sprintf(filename, "%s/%s", directorio_registrados, remote_username);
                fichero_registro = fopen(filename, "r");
                if (fichero_registro == NULL)
                {
                    // Usuario no registrado
                    pet.op_retorno = 3;
                    sprintf(buffer, "%d", pet.op_retorno);
                    ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
                    if (ret == -1)
                    {
                        perror("Error al enviar la respuesta al cliente");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if ((dir = opendir(directorio_publicaciones)) != NULL)
                    {
                        // Vamos a iterar sobre todos los ficheros de publicaciones, recogiendo aquellos en los que este impreso el nombre del usuario a listar su contenido
                        while ((ent = readdir(dir)) != NULL)
                        {
                            if (ent->d_type == DT_REG)
                            {
                                sprintf(filename, "%s/%s", directorio_publicaciones, ent->d_name);
                                FILE *fichero = fopen(filename, "r");
                                if (fichero != NULL)
                                {
                                    RespuestaUsuarios respuesta;
                                    fscanf(fichero, "%s %[^\n]", username_leido, descripcion_leida); // Leer para la descripcion hasta el final de linea (permitir saltos en blanco)
                                    if (!strcmp(remote_username, username_leido))
                                    {
                                        strcpy(respuesta.nombre_fichero, ent->d_name);
                                        strcpy(respuesta.descripcion, descripcion_leida);
                                        // Se aumenta la longitud de las respuestas a medida que haya mas entradas para el usuario que se van a asignar
                                        respuestas = realloc(respuestas, (num_respuestas + 1) * sizeof(RespuestaUsuarios));
                                        if (respuestas == NULL)
                                        {
                                            perror("Error en realloc");
                                            exit(EXIT_FAILURE);
                                        }
                                        respuestas[num_respuestas] = respuesta;
                                        num_respuestas++;
                                        fclose(fichero);
                                        pet.op_retorno = 0; // Exito
                                    }
                                }
                            }
                        }
                        closedir(dir);
                        // Vamos a enviarle en primer lugar al cliente que se ha recopilado de forma exitosa la operacion antes de copiar el resultado
                        sprintf(buffer, "%d", pet.op_retorno);
                        ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
                        if (ret == -1)
                        {
                            perror("Error al enviar la respuesta al cliente");
                            exit(EXIT_FAILURE);
                        }
                        // Vamos a avisar al usuario cuantas respuestas hemos obtenido para que pueda iterar sobre el numero esperado
                        sprintf(buffer, "%d", num_respuestas);
                        ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
                        if (ret == -1)
                        {
                            perror("Error al enviar la respuesta al cliente");
                            exit(EXIT_FAILURE);
                        }
                        // Vamos a enviar al cliente entrada por entrada del directorio los resultados recopilados
                        for (int i = 0; i < num_respuestas; ++i)
                        {
                            char buffer2[1024]; // Aceptar varios campos +256 caracteres
                            sprintf(buffer2, "%s\t%s", respuestas[i].nombre_fichero, respuestas[i].descripcion);
                            int ret = sendMessage(sd_client, buffer2, strlen(buffer2) + 1);
                            if (ret == -1)
                            {
                                perror("Error al enviar los datos al cliente");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    else
                    {
                        // No se pudo abrir el directorio por algun error
                        perror("Error al abrir el directorio de usuarios conectados");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    else if (!(strcmp(pet.codigo_operacion, "LIST_USERS")))
    {
        // Al igual que list content, vamos a recoger en un array dinamico las respuestas TODOX2: para los 2 list queda el free
        RespuestaUsuarios *respuestas = NULL;
        int num_respuestas = 0;
        DIR *dir;
        struct dirent *ent;
        FILE *fichero_registro, *fichero_conexion;
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.fecha_hora, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        // Leer el nombre de usuario
        if (readLine(sd_client, pet.username, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        printf("s> OPERATION FROM %s\n", pet.username);

        CLIENT *clnt;
        enum clnt_stat retval;
        int ret;
        char host[MAX_STRING_LEN] = "127.0.0.1";
        // Inicializar RPC
        clnt = clnt_create(host, NANODT, NANODT_VERSION, "tcp");
        if (clnt == NULL)
        {
            clnt_pcreateerror(host);
            exit(EXIT_FAILURE);
        }
        Peticion_server peticion_rpc;
        strcpy(peticion_rpc.fecha_hora, pet.fecha_hora);
        strcpy(peticion_rpc.nombre_peticion, pet.codigo_operacion);
        strcpy(peticion_rpc.nombre_usuario, pet.username);
        retval = d_print_nofile_1(peticion_rpc, &ret, clnt);
        if (retval != RPC_SUCCESS)
        {
            clnt_perror(clnt, "d_print_nofile_1: ");
            clnt_destroy(clnt);
            exit(EXIT_FAILURE);
        }
        clnt_destroy(clnt);
        // Casos de error para usuario no registrado o no conectado
        sprintf(filename, "%s/%s", directorio_registrados, pet.username);
        fichero_registro = fopen(filename, "r");
        if (fichero_registro == NULL)
        {
            pet.op_retorno = 1; // Fichero no encontrado
            sprintf(buffer, "%d", pet.op_retorno);
            ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
            if (ret == -1)
            {
                perror("Error al enviar la respuesta al cliente");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            sprintf(filename, "%s/%s", directorio_conectados, pet.username);
            fichero_conexion = fopen(filename, "r");
            if (fichero_conexion == NULL)
            {
                pet.op_retorno = 2; // Fichero no encontrado
                sprintf(buffer, "%d", pet.op_retorno);
                ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
                if (ret == -1)
                {
                    perror("Error al enviar la respuesta al cliente");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if ((dir = opendir(directorio_conectados)) != NULL)
                {
                    // Iterar sobre el directorio de usuarios conectados
                    while ((ent = readdir(dir)) != NULL)
                    {
                        if (ent->d_type == DT_REG)
                        {
                            sprintf(filename, "%s/%s", directorio_conectados, ent->d_name);
                            FILE *fichero = fopen(filename, "r");
                            if (fichero != NULL)
                            {
                                RespuestaUsuarios respuesta;
                                // Obtener la IP y puerto impresos en cada identificador de usuario (fichero)
                                fscanf(fichero, "%s %d", respuesta.direccion_ip, &respuesta.puerto_cliente);
                                strcpy(respuesta.nombre_fichero, ent->d_name);
                                //  Agregar la 'respuesta' al array dinamico
                                respuestas = realloc(respuestas, (num_respuestas + 1) * sizeof(RespuestaUsuarios));
                                if (respuestas == NULL)
                                {
                                    perror("Error en realloc");
                                    exit(EXIT_FAILURE);
                                }
                                respuestas[num_respuestas] = respuesta;
                                num_respuestas++;
                                fclose(fichero);
                                pet.op_retorno = 0;
                            }
                        }
                    }
                    closedir(dir);
                    // Al igual que en el otro listado, se avisa al cliente del exito de la operacion con el numero de resultados esperados
                    sprintf(buffer, "%d", pet.op_retorno);
                    ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
                    if (ret == -1)
                    {
                        perror("Error al enviar la respuesta al cliente");
                        exit(EXIT_FAILURE);
                    }
                    sprintf(buffer, "%d", num_respuestas);
                    ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
                    if (ret == -1)
                    {
                        perror("Error al enviar la respuesta al cliente");
                        exit(EXIT_FAILURE);
                    }
                    // Enviar respuesta por respuesta al cliente para que pueda listarlas
                    for (int i = 0; i < num_respuestas; ++i)
                    {
                        char buffer2[1024];
                        sprintf(buffer2, "%s %s %d", respuestas[i].nombre_fichero, respuestas[i].direccion_ip, respuestas[i].puerto_cliente);
                        int ret = sendMessage(sd_client, buffer2, strlen(buffer2) + 1);
                        if (ret == -1)
                        {
                            perror("Error al enviar los datos al cliente");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else
                {
                    // No se pudo abrir el directorio por algun error
                    perror("Error al abrir el directorio de usuarios conectados");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    /*** Aunque no se especifica, hemos agregado esta funcionalidad para que al hacer el get_file, dado que el clienta aunque pueda conocer al listar los usuarios la IP y su puerto,
     *   se asume que no lo puede introducir manualmente para esta funcion, asi que le solicita al servidor conocer antes el numero de estos datos, y con el valor devuelto,
     *   operar con el cliente de destino ***/
    else if (!(strcmp(pet.codigo_operacion, "GET_FILE")))
    {
        FILE *fichero_conexion;
        RespuestaUsuarios respuesta;
        DIR *dir;
        struct dirent *ent;
        int encontrado = 0; // Variable para indicar si se encontro el fichero o no
        char remote_filename[MAX_STRING_LEN];

        if (readLine(sd_client, pet.username, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        printf("s> OPERATION FROM %s\n", pet.username);
        if (readLine(sd_client, remote_filename, MAX_STRING_LEN) == -1)
        {
            perror("Error al leer el nombre de usuario del cliente");
            exit(EXIT_FAILURE);
        }
        // Casos de error para usuario no conectado
        sprintf(filename, "%s/%s", directorio_conectados, pet.username);
        fichero_conexion = fopen(filename, "r");
        if (fichero_conexion == NULL)
        {
            pet.op_retorno = 2; // Fichero no encontrado
            sprintf(buffer, "%d", pet.op_retorno);
            ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
            if (ret == -1)
            {
                perror("Error al enviar la respuesta al cliente");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            fscanf(fichero_conexion, "%s %d", respuesta.direccion_ip, &respuesta.puerto_cliente);
            if ((dir = opendir(directorio_publicaciones)) != NULL)
            {
                // Iterar sobre todos los ficheros en el directorio
                while ((ent = readdir(dir)) != NULL)
                {
                    if (ent->d_type == DT_REG)
                    {
                        if (!strcmp(ent->d_name, remote_filename))
                        {
                            // Construir el nombre completo del fichero
                            sprintf(filename, "%s/%s", directorio_publicaciones, ent->d_name);
                            // Abrir el fichero
                            FILE *fichero = fopen(filename, "r");
                            if (fichero != NULL)
                            {
                                char username_leido[MAX_STRING_LEN];
                                // Leer comprobar si coincide
                                fscanf(fichero, "%s", username_leido);
                                if (!strcmp(username_leido, pet.username))
                                {
                                    // Si coincide, establecer encontrado a 1 y salir del bucle
                                    encontrado = 1;
                                    fclose(fichero);
                                    break;
                                }
                                fclose(fichero);
                            }
                        }
                    }
                }
                // Comprobar si se encontro el fichero, en caso contrario mandamos error
                if (!encontrado)
                {
                    pet.op_retorno = 1;
                    sprintf(buffer, "%d", pet.op_retorno);
                    ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
                    if (ret == -1)
                    {
                        perror("Error al enviar la respuesta al cliente");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    pet.op_retorno = 0;
                    // Enviar respuesta al cliente
                    sprintf(buffer, "%d", pet.op_retorno);
                    ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
                    if (ret == -1)
                    {
                        perror("Error al enviar la respuesta al cliente");
                        exit(EXIT_FAILURE);
                    }
                    // Todo ha ido OK
                    if (pet.op_retorno == 0)
                    {
                        sprintf(buffer, "%s", respuesta.direccion_ip);
                        ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
                        if (ret == -1)
                        {
                            perror("Error al enviar la respuesta al cliente");
                            exit(EXIT_FAILURE);
                        }
                        sprintf(buffer, "%d", respuesta.puerto_cliente);
                        ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
                        if (ret == -1)
                        {
                            perror("Error al enviar la respuesta al cliente");
                            exit(EXIT_FAILURE);
                        }
                    }
                    fclose(fichero_conexion); // Cerrar el fichero de conexión
                }
                closedir(dir);
            }
        }
    }
    else
    {
        pet.op_retorno = 2;
        sprintf(buffer, "%d", pet.op_retorno);
        ret = sendMessage(sd_client, buffer, strlen(buffer) + 1);
        if (ret == -1)
        {
            perror("Error al enviar la respuesta al cliente");
            exit(EXIT_FAILURE);
        }
    }
    // Cerrar el socket copia local del cliente
    close(sd_client);
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    // Trata de argumentos
    if (argc != 2)
    {
        fprintf(stderr, "Error: expected './servidor PORT_NUMBER' f.e.: './servidor 4050'\n");
        return -1;
    }
    // Declarar los sockets de cliente usados
    int sd_client;
    struct sockaddr_in server_addr, client_addr;
    // Manejador de signal para salir del bucle
    // Declarar el hilo de cada cliente, atributo e inicializar los mutex y varconds a ser usadas para la sincronizacion concurrente
    pthread_t thid;
    pthread_attr_t t_attr;
    pthread_mutex_init(&mutex_mensaje, NULL);
    pthread_cond_init(&cond_mensaje, NULL);
    pthread_attr_init(&t_attr);
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);

    // Crear el socket del servidor
    if ((sd_server = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket: ");
        exit(-1);
    }
    // Establecer las opciones del socket del servidor
    int opt = 1;
    if (setsockopt(sd_server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt: ");
        exit(-1);
    }
    // Si habia datos asociados a la direccion del servidor, restablecerlo a cero todo y reasignar valores
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));
    // Hacer el bind + listen del servidor
    if (bind(sd_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind: ");
        exit(-1);
    }
    if (listen(sd_server, 3) < 0)
    {
        perror("listen: ");
        exit(-1);
    }
    int addrlen = sizeof(server_addr);
    signal(SIGINT, handler_sigint);
    // Imprimira por defecto 0.0.0.0 ya que con INADDR_ANY se indica que el servidor esucha desde cualquier interfaz de red a su disposicion
    printf("s> init server %s:%s\n", inet_ntoa(server_addr.sin_addr), argv[1]);
    while (0 == do_exit)
    {

        // Aceptar peticiones del cliente
        sd_client = accept(sd_server, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen);
        if (sd_client <= 0)
        {
            perror("accept");
            exit(-1);
        }
        Argumentos_thread args_thread;
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        args_thread.sd_client = sd_client;
        strcpy(args_thread.ip_address, client_ip);
        // Crear el hilo para tratar las peticiones por cada cliente
        if (pthread_create(&thid, &t_attr, (void *)tratar_peticion, (void *)&args_thread) == 0)
        {
            pthread_mutex_lock(&mutex_mensaje);
            while (mensaje_no_copiado)
                pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
            mensaje_no_copiado = 1;
            pthread_mutex_unlock(&mutex_mensaje);
        }
    }
    // Fin del bucle y liberacion de recursos
    return 0;
}