const MAX_VALUE1 = 256;

struct Peticion_server {
    opaque nombre_peticion [MAX_VALUE1];
    opaque nombre_usuario [MAX_VALUE1];
    opaque nombre_fichero[MAX_VALUE1];
    opaque fecha_hora [MAX_VALUE1];
    int op_retorno_rpc;
};

program NANODT
{
    version NANODT_VERSION {
        int d_print_nofile(struct Peticion_server) = 0;
        int d_print_file(struct Peticion_server) = 1;
    } = 1;
} = 55555;