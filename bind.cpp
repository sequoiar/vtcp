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

#include "vtcp.hpp"

int main (int argc, char *argv [])
{
    //  Parse the command line arguments.
    if (argc != 3) {
        printf ("usage: bind <port> <subport>\n");
        return 1;
    }
    int port = atoi (argv [1]);
    int subport = atoi (argv [2]);

    int listener = vtcp_bind (port, subport);
    if (listener < 0) {
        printf ("error in vtcp_bind: %s\n", strerror (errno));
        exit (1);
    }

    int fd = vtcp_accept (listener);
    if (fd < 0) {
        printf ("error in vtcp_accept: %s\n", strerror (errno));
        exit (1);
    }

    printf ("got connection!\n");

    sleep (3600);

    close (fd);
    close (listener);

    return 0;
}

