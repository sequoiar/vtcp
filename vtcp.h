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

#ifndef __VTCP_H_INCLUDED__
#define __VTCP_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <arpa/inet.h>

typedef uint32_t vtcp_subport_t;

int vtcp_connect (in_addr_t address, in_port_t port, vtcp_subport_t subport);
int vtcp_bind (in_port_t port, vtcp_subport_t subport);
int vtcp_accept (int fd);

#ifdef __cplusplus
}
#endif

#endif

