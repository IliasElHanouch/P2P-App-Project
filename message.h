#ifndef MESSAGE_H
#define MESSAGE_H

/* Funciones de utilidad para la manipulación de datos */

/* Funcion para leer la linea de un buffer byte a byte, util
   en las operaciones con el cliente-servidor cuando no se conoce la longitud exacta
*/
ssize_t readLine(int fd, void *buffer, size_t n);
/* Funcion para mandar desde el buffer al socket, util en las operaciones 
   con el cliente-servidor cuando no se conoce la longitud exacta
*/
int sendMessage(int socket, char *buffer, int len);

#endif