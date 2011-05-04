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
#include <stdint.h>
#include <assert.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <poll.h>

#include <map>
#include <set>
#include <vector>

int main (int argc, char *argv [])
{
    //  Parse the command line arguments.
    if (argc != 2) {
        printf ("usage: vtcpd <port>\n");
        return 1;
    }
    int port = atoi (argv [1]);

    //  Start listening on the TCP port.
    int tcplistener = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcplistener == -1) {
        printf ("error in socket: %s\n", strerror (errno));
        exit (1);
    }
    int reuseaddr = 1;
    int rc = setsockopt (tcplistener, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
        sizeof (reuseaddr));
    if (rc != 0) {
        printf ("error in setsockopt: %s\n", strerror (errno));
        exit (1);
    }
    struct sockaddr_in addr;
    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons (port);
    addr.sin_addr.s_addr = INADDR_ANY;
    rc = bind (tcplistener, (struct sockaddr*) &addr, sizeof (addr));
    if (rc != 0) {
        printf ("error in bind: %s\n", strerror (errno));
        exit (1);
    }
    rc = listen (tcplistener, 100);
    if (rc != 0) {
        printf ("error in listen: %s\n", strerror (errno));
        exit (1);
    }

    //  Start listening on the IPC endpoint.
    char ipc [32];
    snprintf (ipc, sizeof (ipc), "/tmp/vtcp-%d.ipc", (int) port);

    unlink (ipc);
    int ipclistener = socket (AF_UNIX, SOCK_STREAM, 0);
    if (ipclistener == -1) {
        printf ("error in socket: %s\n", strerror (errno));
        exit (1);
    }
    struct sockaddr_un unaddr;
    if (strlen (ipc) >= sizeof (unaddr.sun_path))
    {
        printf ("IPC endpoint name too long\n");
        exit (1);
    }
    unaddr.sun_family = AF_UNIX;
    strcpy (unaddr.sun_path, ipc);
    rc = bind (ipclistener, (struct sockaddr*) &unaddr, sizeof (unaddr));
    if (rc != 0) {
        printf ("error in bind: %s\n", strerror (errno));
        exit (1);
    }
    rc = listen (ipclistener, 100);
    if (rc != 0) {
        printf ("error in listen: %s\n", strerror (errno));
        exit (1);
    }

    //  The data structure to hold all the connections from the local apps.
    typedef std::set <int> connections_t;
    connections_t connections;

    //  The data structure to hold all the subport registrations.
    typedef std::map <int, int> registrations_t;
    registrations_t registrations;

    while (true) {

        //  Fill in the pollset.
        typedef std::vector <struct pollfd> pollset_t;
        pollset_t pollset (2 + connections.size ());
        pollset [0].fd = tcplistener;
        pollset [0].events = POLLIN;
        pollset [0].revents = 0;
        pollset [1].fd = ipclistener;
        pollset [1].events = POLLIN;
        pollset [1].revents = 0;
        connections_t::size_type pos = 2;
        for (connections_t::iterator it = connections.begin ();
              it != connections.end (); ++it, ++pos) {
            pollset [pos].fd = *it;
            pollset [pos].events = POLLIN;
            pollset [pos].revents = 0;
        }

        //  Wait for events.
        rc = poll (&pollset [0], pollset.size (), -1);
        if (rc < 0) {
            printf ("error in poll: %s\n", strerror (errno));
            exit (1);
        }

        //  Activity on the TCP listener.
        if (pollset [0].revents & POLLIN) {
            int fd = accept (tcplistener, NULL, NULL);
            if (fd < 0) {
                printf ("error in accept: %s\n", strerror (errno));
                exit (1);
            }
            uint32_t subport;
            rc = recv (fd, &subport, sizeof (subport), 0);
            if (rc == -1) {
                printf ("error in recv: %s\n", strerror (errno));
                exit (1);
            }
            if (rc == 0) {
                close (fd);
                continue;
            }
            assert (rc == sizeof (uint32_t));
            subport = ntohl (subport);

            //  If the subport is not bound, we'll simply close
            //  the connection causing the connecting party to receive error.
            registrations_t::iterator it = registrations.find (subport);
            if (it == registrations.end ()) {
                close (fd);
                continue;
            }

            //  We are going to pass the file descriptor. Thus we'll use 
            //  a single byte dummy payload.
            struct iovec iov;
            unsigned char buf [] = {0};
            iov.iov_base = buf;
            iov.iov_len = 1;

            //  Compose the message.
            struct msghdr msg;
            memset (&msg, 0, sizeof (msg));
            msg.msg_iov = &iov;
            msg.msg_iovlen = 1;
            char control [sizeof (struct cmsghdr) + 10];
            msg.msg_control = control;
            msg.msg_controllen = sizeof (control);

            //  Attach the file descriptor to the message.
            struct cmsghdr *cmsg;
            cmsg = CMSG_FIRSTHDR (&msg);
            cmsg->cmsg_level = SOL_SOCKET;
            cmsg->cmsg_type = SCM_RIGHTS;
            cmsg->cmsg_len = CMSG_LEN (sizeof (fd));
            *(int*) CMSG_DATA (cmsg) = fd;

            //  Adjust the size of the control to match the data.
            msg.msg_controllen = cmsg->cmsg_len;

            //  Pass the file descriptor to the registered process.
            rc = sendmsg (it->second, &msg, 0);
            if (rc == -1) {
                printf ("error in sendmsg: %s\n", strerror (errno));
                exit (1);
            }
            assert (rc == 1);
        }

        //  Activity on the IPC listener.
        if (pollset [1].revents & POLLIN) {
            int fd = accept (ipclistener, NULL, NULL);
            if (fd < 0) {
                printf ("error in accept: %s\n", strerror (errno));
                exit (1);
            }
            connections.insert (fd);
        }

        //  Check connections from apps for vtcp binds.
        for (pollset_t::size_type i = 2; i != pollset.size (); ++i) {
            if (pollset [i].revents & POLLNVAL)
                assert (false);
            else if (pollset [i].revents & POLLERR)
                goto close_connection;
            else if (pollset [i].revents & POLLHUP)
                goto close_connection;
            else if (pollset [i].revents & POLLIN) {
                uint32_t subp;
                rc = recv (pollset [i].fd, &subp, sizeof (subp), 0);
                if (rc == -1) {
                    printf ("error in recv: %s\n", strerror (errno));
                    exit (1);
                }
                if (rc == 0) {
close_connection:
                    close (pollset [i].fd);
                    for (registrations_t::iterator it = registrations.begin ();
                          it != registrations.end (); ++it) {
                        if (it->second == pollset [i].fd) {
                            registrations_t::iterator o = it;
                            --o;
                            registrations.erase (it);
                            it = o;
                        }
                    }
                    connections.erase (pollset [i].fd);
                    break;
                }
                assert (rc == sizeof (uint32_t));
                uint32_t subport = ntohl (subp);
                if (!registrations.insert (std::make_pair (subport,
                      pollset [i].fd)).second) {

                    //  If subport is already in use, pass the error
                    //  to the binding party.
                    unsigned char buf [] = {1};
                    rc = send (pollset [i].fd, buf, 1, 0);
                    if (rc < 0) {
                        printf ("error in send: %s\n", strerror (errno));
                        exit (1);
                    }
                    assert (rc == 1);
                }
            }
        }
    }

    return 0;
}
