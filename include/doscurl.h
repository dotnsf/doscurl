/*
 * DOSCurl - HTTP Client for 16-bit DOS
 * Common header file
 */

#ifndef DOSCURL_H
#define DOSCURL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Version information */
#define DOSCURL_VERSION "1.0.0"
#define DOSCURL_USER_AGENT "DOSCurl/1.0"

/* Buffer sizes */
#define MAX_URL_LENGTH 512
#define MAX_HEADER_LENGTH 256
#define MAX_BUFFER_SIZE 4096
#define MAX_HOSTNAME_LENGTH 128
#define MAX_PATH_LENGTH 256

/* HTTP methods */
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_HEAD
} http_method_t;

/* Return codes */
#define SUCCESS 0
#define ERROR_INVALID_URL -1
#define ERROR_NETWORK -2
#define ERROR_DNS -3
#define ERROR_HTTP -4
#define ERROR_MEMORY -5
#define ERROR_FILE_IO -6

/* Global options structure */
typedef struct {
    char url[MAX_URL_LENGTH];
    http_method_t method;
    char *post_data;
    char *output_file;
    int verbose;
    char custom_headers[10][MAX_HEADER_LENGTH];
    int num_custom_headers;
} options_t;

/* Function prototypes from utils.c */
void print_error(const char *message);
void print_verbose(const char *message);
char *str_trim(char *str);
int str_starts_with(const char *str, const char *prefix);

#endif /* DOSCURL_H */