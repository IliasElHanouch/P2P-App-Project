/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "rpc_management.h"


void
nanodt_1(char *host)
{
	CLIENT *clnt;
	enum clnt_stat retval_1;
	int result_1;
	struct Peticion_server d_print_nofile_1_arg1;
	enum clnt_stat retval_2;
	int result_2;
	struct Peticion_server d_print_file_1_arg1;

#ifndef	DEBUG
	clnt = clnt_create (host, NANODT, NANODT_VERSION, "udp");
	if (clnt == NULL) {
		clnt_pcreateerror (host);
		exit (1);
	}
#endif	/* DEBUG */

	retval_1 = d_print_nofile_1(d_print_nofile_1_arg1, &result_1, clnt);
	if (retval_1 != RPC_SUCCESS) {
		clnt_perror (clnt, "call failed");
	}
	retval_2 = d_print_file_1(d_print_file_1_arg1, &result_2, clnt);
	if (retval_2 != RPC_SUCCESS) {
		clnt_perror (clnt, "call failed");
	}
#ifndef	DEBUG
	clnt_destroy (clnt);
#endif	 /* DEBUG */
}


int
main (int argc, char *argv[])
{
	char *host;

	if (argc < 2) {
		printf ("usage: %s server_host\n", argv[0]);
		exit (1);
	}
	host = argv[1];
	nanodt_1 (host);
exit (0);
}
