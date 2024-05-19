typedef struct {
    char codigo_operacion[256];
    int op_retorno;
    char username[256];
    int puerto;
    char fecha_hora [256];
} Peticion;

typedef struct {
    char nombre_fichero[256];
    char descripcion_fichero[256];
} Contenido;