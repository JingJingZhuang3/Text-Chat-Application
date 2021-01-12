/**
 * @jzhuang3_assignment1
 * @author  JingJing Zhuang <jzhuang3@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../include/global.h"
#include "../include/logger.h"
#include "../include/logger.h"
#include "../include/client.h"
#include "../include/server.h"

using namespace std;

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/* Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	/*Start Here*/
	/******
		References:
		- https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf 
		- server.c and client.c from https://cse.buffalo.edu/~lusu/cse4589/ 
	******/
	if (argc != 3)
	{
		perror("Missing arguments.\n");
		return 0;
	}
	int port;
	char *ptr;
	port = strtol(argv[2], &ptr, 10); //store input port
	if (port > 0 && port < 65535)
	{
		if (strcmp(argv[1], "s") == 0)
		{
			server server_socket(port);
		}
		if (strcmp(argv[1], "c") == 0)
		{
			client client_socket(argv[2]);
		}
	}
	return 0;
}
