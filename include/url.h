/*
 * DOSCurl - URL Parser
 * Header file for URL parsing functions
 */

#ifndef URL_H
#define URL_H

#include "doscurl.h"

/* URL structure */
typedef struct {
    char protocol[16];      /* http, https, etc. */
    char hostname[MAX_HOSTNAME_LENGTH];
    int port;               /* Default: 80 for http */
    char path[MAX_PATH_LENGTH];
} url_t;

/* Function prototypes */
int parse_url(const char *url_string, url_t *url);
void url_init(url_t *url);
void url_free(url_t *url);

#endif /* URL_H */