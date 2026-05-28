/*
 * DOSCurl - HTTP Protocol Handler
 * Header file for HTTP request/response functions
 */

#ifndef HTTP_H
#define HTTP_H

#include "doscurl.h"
#include "url.h"
#include "socket.h"

/* HTTP response structure */
typedef struct {
    int status_code;
    char status_message[64];
    char *body;
    int body_length;
    char headers[20][MAX_HEADER_LENGTH];
    int num_headers;
} http_response_t;

/* Function prototypes */
int http_get(const url_t *url, const options_t *options, http_response_t *response);
int http_post(const url_t *url, const options_t *options, http_response_t *response);
int http_request(const url_t *url, const options_t *options, http_response_t *response);
void http_response_init(http_response_t *response);
void http_response_free(http_response_t *response);
int http_parse_response(const char *raw_response, http_response_t *response);
void http_print_response(const http_response_t *response, const options_t *options);

#endif /* HTTP_H */