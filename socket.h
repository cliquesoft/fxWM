// socket.h	remote command related functions
//
// created	2015/09/02 by Lauri Kasanen
// updated	2015/09/02 by Lauri Kasanen
//
// Unless a valid Cliquesoft Private License (CPLv1) has been purchased for your
// device, this software is licensed under the Cliquesoft Public License (CPLv2)
// as found on the Cliquesoft website at www.cliquesoft.org.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the appropriate Cliquesoft License for details.




#if CONTROL_SOCK


#ifndef SOCKET_H
#define SOCKET_H

#include <FL/Fl.H>

void initSockets(const char path[]);

// Callbacks
void accepter(FL_SOCKET fd, void *);
void disconnecter(FL_SOCKET fd, void *);
void commander(FL_SOCKET fd, void *);

#endif


#endif

