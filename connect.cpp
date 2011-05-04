/*
    Copyright (c) 2011 Martin Sustrik

    This file is part of vtcp project.

    vtcp is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "vtcp.hpp"

int main (int argc, char *argv [])
{
    //  Parse the command line arguments.
    if (argc != 4) {
        printf ("usage: connect <address> <port> <subport>\n");
        return 1;
    }
    in_addr_t addr = inet_addr (argv [1]);
    int port = atoi (argv [2]);
    int subport = atoi (argv [3]);

    int fd = vtcp_connect (addr, port, subport);
    if (fd < 0) {
        printf ("error in vtcp_connect: %s\n", strerror (errno));
        exit (1);
    }

    sleep (3600);

    close (fd);

    return 0;
}
