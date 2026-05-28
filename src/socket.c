/*
 * DOSCurl - Socket Operations
 * Implementation of TCP socket functions using WATTCP
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tcp.h>
#include "../include/socket.h"
#include "../include/utils.h"

/* WATTCP socket structure */
static tcp_Socket *sock_ptr = NULL;

/*
 * Initialize WATTCP library
 * Returns: 0 on success, -1 on error
 */
int socket_init(void) {
    print_verbose("Initializing WATTCP...");
    
    /* Initialize WATTCP */
    sock_init();
    
    /* Check if packet driver is loaded */
    if (_watt_do_exit) {
        print_error("Packet driver not found or network not configured");
        return ERROR_NETWORK;
    }
    
    print_verbose("WATTCP initialized successfully");
    return SUCCESS;
}

/*
 * Cleanup WATTCP library
 */
void socket_cleanup(void) {
    print_verbose("Cleaning up network resources...");
    
    if (sock_ptr != NULL) {
        sock_close((sock_type *)sock_ptr);
        sock_ptr = NULL;
    }
}

/*
 * Resolve hostname to IP address
 * Returns: 0 on success, -1 on error
 */
int socket_resolve(const char *hostname, char *ip_address) {
    DWORD ip;
    char msg[128];
    
    if (hostname == NULL || ip_address == NULL) {
        return ERROR_DNS;
    }
    
    sprintf(msg, "Resolving hostname: %s", hostname);
    print_verbose(msg);
    
    /* Try to resolve hostname */
    ip = resolve(hostname);
    
    if (ip == 0) {
        sprintf(msg, "Failed to resolve hostname: %s", hostname);
        print_error(msg);
        return ERROR_DNS;
    }
    
    /* Convert IP to string */
    sprintf(ip_address, "%lu.%lu.%lu.%lu",
            (ip >> 24) & 0xFF,
            (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF,
            ip & 0xFF);
    
    sprintf(msg, "Resolved to: %s", ip_address);
    print_verbose(msg);
    
    return SUCCESS;
}

/*
 * Connect to remote host
 * Returns: socket handle on success, -1 on error
 */
socket_t socket_connect(const char *hostname, int port) {
    DWORD host_ip;
    char msg[128];
    tcp_Socket *s;
    int timeout;
    
    if (hostname == NULL || port <= 0 || port > 65535) {
        return -1;
    }
    
    sprintf(msg, "Connecting to %s:%d...", hostname, port);
    print_verbose(msg);
    
    /* Resolve hostname */
    host_ip = resolve(hostname);
    if (host_ip == 0) {
        sprintf(msg, "Failed to resolve hostname: %s", hostname);
        print_error(msg);
        return -1;
    }
    
    /* Allocate socket structure */
    s = (tcp_Socket *)malloc(sizeof(tcp_Socket));
    if (s == NULL) {
        print_error("Failed to allocate socket structure");
        return -1;
    }
    
    /* Open TCP connection - use _w32_ prefix directly */
    if (!_w32_tcp_open(s, 0, host_ip, port, NULL)) {
        print_error("Failed to open TCP connection");
        free(s);
        return -1;
    }
    
    /* Wait for connection to establish */
    sprintf(msg, "Waiting for connection to establish...");
    print_verbose(msg);
    
    /* Wait up to sock_delay seconds for connection */
    timeout = sock_delay;
    while (!sock_established((sock_type *)s) && timeout > 0) {
        if (!tcp_tick((sock_type *)s)) {
            print_error("Connection failed or timed out");
            sock_close((sock_type *)s);
            free(s);
            return -1;
        }
        timeout--;
    }
    
    /* Check if connection is established */
    if (!sock_established((sock_type *)s)) {
        print_error("Failed to establish connection");
        sock_close((sock_type *)s);
        free(s);
        return -1;
    }
    
    print_verbose("Connection established");
    
    /* Store socket pointer for cleanup */
    sock_ptr = s;
    
    return (socket_t)s;
}

/*
 * Send data through socket
 * Returns: number of bytes sent, -1 on error
 */
int socket_send(socket_t sock, const char *data, int length) {
    sock_type *s = (sock_type *)sock;
    int sent = 0;
    char msg[128];
    
    if (s == NULL || data == NULL || length <= 0) {
        return -1;
    }
    
    sprintf(msg, "Sending %d bytes...", length);
    print_verbose(msg);
    
    /* Send data */
    sent = sock_write(s, (BYTE *)data, length);
    
    if (sent < 0) {
        print_error("Failed to send data");
        return -1;
    }
    
    /* Flush output buffer - use _w32_ prefix directly */
    _w32_sock_flush(s);
    
    sprintf(msg, "Sent %d bytes", sent);
    print_verbose(msg);
    
    return sent;
}

/*
 * Receive data from socket
 * Returns: number of bytes received, 0 on connection close, -1 on error
 */
int socket_recv(socket_t sock, char *buffer, int buffer_size) {
    sock_type *s = (sock_type *)sock;
    int received = 0;
    int timeout;
    char msg[128];
    
    if (s == NULL || buffer == NULL || buffer_size <= 0) {
        return -1;
    }
    
    /* Wait for data with timeout */
    timeout = sock_delay;
    while (sock_dataready(s) <= 0 && timeout > 0) {
        /* Check if connection is still alive */
        if (!sock_established(s)) {
            print_verbose("Connection closed by remote host");
            return 0;
        }
        
        if (!tcp_tick(s)) {
            return 0;
        }
        
        timeout--;
    }
    
    /* Check if data is available */
    if (sock_dataready(s) <= 0) {
        return 0; /* No data available */
    }
    
    /* Read data */
    received = sock_read(s, (BYTE *)buffer, buffer_size);
    
    if (received < 0) {
        print_error("Failed to receive data");
        return -1;
    }
    
    if (received > 0) {
        sprintf(msg, "Received %d bytes", received);
        print_verbose(msg);
    }
    
    return received;
}

/*
 * Close socket connection
 */
void socket_close(socket_t sock) {
    sock_type *s = (sock_type *)sock;
    
    if (s == NULL) {
        return;
    }
    
    print_verbose("Closing connection...");
    
    /* Close socket */
    sock_close(s);
    
    /* Free socket structure */
    free(s);
    
    /* Clear global pointer if it matches */
    if (sock_ptr == (tcp_Socket *)s) {
        sock_ptr = NULL;
    }
    
    print_verbose("Connection closed");
}