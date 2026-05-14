/*
 * DOSCurl - URL Parser
 * Implementation of URL parsing functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/url.h"
#include "../include/utils.h"

/*
 * Initialize URL structure with default values
 */
void url_init(url_t *url) {
    if (url == NULL) {
        return;
    }
    
    memset(url, 0, sizeof(url_t));
    strcpy(url->protocol, "http");
    url->port = 80;
    strcpy(url->path, "/");
}

/*
 * Free URL structure resources
 */
void url_free(url_t *url) {
    /* Nothing to free for now, but keep for future use */
    if (url != NULL) {
        /* Avoid unused parameter warning */
    }
}

/*
 * Parse URL string into URL structure
 * Supports: http://hostname:port/path
 * Returns: 0 on success, -1 on error
 */
int parse_url(const char *url_string, url_t *url) {
    char buffer[MAX_URL_LENGTH];
    char *ptr;
    char *host_start;
    char *path_start;
    char *port_start;
    int len;
    
    if (url_string == NULL || url == NULL) {
        return ERROR_INVALID_URL;
    }
    
    /* Initialize URL structure */
    url_init(url);
    
    /* Copy URL to buffer for parsing */
    strncpy(buffer, url_string, MAX_URL_LENGTH - 1);
    buffer[MAX_URL_LENGTH - 1] = '\0';
    
    ptr = buffer;
    
    /* Parse protocol */
    if (strncmp(ptr, "http://", 7) == 0) {
        strcpy(url->protocol, "http");
        url->port = 80;
        ptr += 7;
    }
    else if (strncmp(ptr, "https://", 8) == 0) {
        strcpy(url->protocol, "https");
        url->port = 443;
        ptr += 8;
        /* Note: HTTPS not supported, but parse it anyway */
    }
    else {
        /* No protocol specified, assume http:// */
        strcpy(url->protocol, "http");
        url->port = 80;
    }
    
    host_start = ptr;
    
    /* Find path separator */
    path_start = strchr(ptr, '/');
    
    /* Find port separator */
    port_start = strchr(ptr, ':');
    
    /* If port separator exists and comes before path */
    if (port_start != NULL && (path_start == NULL || port_start < path_start)) {
        /* Extract hostname */
        len = (int)(port_start - host_start);
        if (len >= MAX_HOSTNAME_LENGTH) {
            len = MAX_HOSTNAME_LENGTH - 1;
        }
        strncpy(url->hostname, host_start, len);
        url->hostname[len] = '\0';
        
        /* Extract port */
        port_start++; /* Skip ':' */
        url->port = atoi(port_start);
        
        if (url->port <= 0 || url->port > 65535) {
            return ERROR_INVALID_URL;
        }
        
        /* Update pointer to path start */
        if (path_start != NULL) {
            ptr = path_start;
        }
        else {
            ptr = NULL;
        }
    }
    else {
        /* No port specified, extract hostname up to path or end */
        if (path_start != NULL) {
            len = (int)(path_start - host_start);
        }
        else {
            len = strlen(host_start);
        }
        
        if (len >= MAX_HOSTNAME_LENGTH) {
            len = MAX_HOSTNAME_LENGTH - 1;
        }
        strncpy(url->hostname, host_start, len);
        url->hostname[len] = '\0';
        
        ptr = path_start;
    }
    
    /* Check if hostname is empty */
    if (strlen(url->hostname) == 0) {
        return ERROR_INVALID_URL;
    }
    
    /* Extract path */
    if (ptr != NULL && *ptr == '/') {
        strncpy(url->path, ptr, MAX_PATH_LENGTH - 1);
        url->path[MAX_PATH_LENGTH - 1] = '\0';
    }
    else {
        strcpy(url->path, "/");
    }
    
    return SUCCESS;
}

// Made with Bob
