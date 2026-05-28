/*
 * DOSCurl - Socket Operations
 * Header file for TCP socket functions using WATTCP
 */

#ifndef SOCKET_H
#define SOCKET_H

#include "doscurl.h"
#include "url.h"

/* Socket handle type */
typedef int socket_t;

/* Function prototypes */
int socket_init(void);
void socket_cleanup(void);
socket_t socket_connect(const char *hostname, int port);
int socket_send(socket_t sock, const char *data, int length);
int socket_recv(socket_t sock, char *buffer, int buffer_size);
void socket_close(socket_t sock);
int socket_resolve(const char *hostname, char *ip_address);

#endif /* SOCKET_H */