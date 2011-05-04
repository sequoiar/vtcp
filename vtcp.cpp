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

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "vtcp.h"

int vtcp_connect (in_addr_t address, in_port_t port,
    vtcp_subport_t subport)
{
    //  Open the TCP socket.
    int s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
        return -1;

    //  Connect to the peer.
    struct sockaddr_in addr;
    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons (port);
    addr.sin_addr.s_addr = address;
    int rc = connect (s, (struct sockaddr*) &addr, sizeof (addr));
    if (rc != 0) {
        close (s);
        return -1;
    }

    //  Specify the subport.
    uint32_t subp = htonl (subport);
    rc = send (s, &subp, sizeof (subp), 0);
    if (rc < 0) {
        close (s);
        return -1;
    }
    assert (rc == sizeof (subp));

    return s;
}

int vtcp_bind (in_port_t port, vtcp_subport_t subport)
{
    //  Convert port into IPC filename.
    char ipc [32];
    snprintf (ipc, sizeof (ipc), "/tmp/vtcp-%d.ipc", (int) port);

    //  Open connection to vtcp daemon.
    int s = socket (AF_UNIX, SOCK_STREAM, 0);
    if (s == -1)
        return -1;
    struct sockaddr_un unaddr;
    assert (strlen (ipc) < sizeof (unaddr.sun_path));
    unaddr.sun_family = AF_UNIX;
    strcpy (unaddr.sun_path, ipc);
    int rc = connect (s, (struct sockaddr*) &unaddr, sizeof (unaddr));
    if (rc != 0) {
        close (s);
        return -1;
    }

    //  Specify the subport.
    uint32_t subp = htonl (subport);
    rc = send (s, &subp, sizeof (subp), 0);
    if (rc < 0) {
        close (s);
        return -1;
    }
    assert (rc == sizeof (subp));

    return s;
}

int vtcp_accept (int fd)
{
    //  Wait for new connection, i.e. read a message from vtcp daemon.
    unsigned char buf [1];
    struct iovec iov;
    iov.iov_base = buf;
    iov.iov_len = sizeof (buf);
    struct msghdr msg;
    memset (&msg, 0, sizeof (msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    unsigned char control [1024];
    msg.msg_control = control;
    msg.msg_controllen = sizeof (control);
    int rc = recvmsg (fd, &msg, 0);
    if (rc < 0)
        return -1;
    assert (rc != 0);
    assert (rc == 1);

    if (buf [0] == 1) {
        errno = EADDRINUSE;
        return -1;
    }

    assert (buf [0] == 0);

    //  Loop over control messages to find the embedded file descriptor.
    struct cmsghdr *cmsg = CMSG_FIRSTHDR (&msg);
    while (cmsg != NULL) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type  == SCM_RIGHTS)
            return *(int*) CMSG_DATA (cmsg);
        cmsg = CMSG_NXTHDR (&msg, cmsg);
    }
    assert (false);
}

