/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "rpc_management.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

enum clnt_stat 
d_print_nofile_1(struct Peticion_server arg1, int *clnt_res,  CLIENT *clnt)
{
	return (clnt_call(clnt, d_print_nofile,
		(xdrproc_t) xdr_Peticion_server, (caddr_t) &arg1,
		(xdrproc_t) xdr_int, (caddr_t) clnt_res,
		TIMEOUT));
}

enum clnt_stat 
d_print_file_1(struct Peticion_server arg1, int *clnt_res,  CLIENT *clnt)
{
	return (clnt_call(clnt, d_print_file,
		(xdrproc_t) xdr_Peticion_server, (caddr_t) &arg1,
		(xdrproc_t) xdr_int, (caddr_t) clnt_res,
		TIMEOUT));
}
