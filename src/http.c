/*
 * DOSCurl - HTTP Protocol Handler
 * Implementation of HTTP request/response functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/http.h"
#include "../include/utils.h"

/*
 * Initialize HTTP response structure
 */
void http_response_init(http_response_t *response) {
    if (response == NULL) {
        return;
    }
    
    memset(response, 0, sizeof(http_response_t));
    response->status_code = 0;
    response->body = NULL;
    response->body_length = 0;
    response->num_headers = 0;
}

/*
 * Free HTTP response structure resources
 */
void http_response_free(http_response_t *response) {
    if (response == NULL) {
        return;
    }
    
    if (response->body != NULL) {
        free(response->body);
        response->body = NULL;
    }
    
    response->body_length = 0;
}

/*
 * Build HTTP request string
 */
static int build_http_request(const url_t *url, const options_t *options, 
                              char *request, int max_size) {
    char *method_str;
    int len = 0;
    int i;
    
    /* Determine method string */
    switch (options->method) {
        case HTTP_GET:
            method_str = "GET";
            break;
        case HTTP_POST:
            method_str = "POST";
            break;
        case HTTP_HEAD:
            method_str = "HEAD";
            break;
        default:
            method_str = "GET";
    }
    
    /* Build request line */
    len = sprintf(request, "%s %s HTTP/1.0\r\n", method_str, url->path);
    
    /* Add Host header */
    len += sprintf(request + len, "Host: %s\r\n", url->hostname);
    
    /* Add User-Agent header */
    len += sprintf(request + len, "User-Agent: %s\r\n", DOSCURL_USER_AGENT);
    
    /* Add Connection header */
    len += sprintf(request + len, "Connection: close\r\n");
    
    /* Add custom headers */
    for (i = 0; i < options->num_custom_headers; i++) {
        len += sprintf(request + len, "%s\r\n", options->custom_headers[i]);
    }
    
    /* Add Content-Length and Content-Type for POST */
    if (options->method == HTTP_POST && options->post_data != NULL) {
        int post_len = strlen(options->post_data);
        len += sprintf(request + len, "Content-Type: application/x-www-form-urlencoded\r\n");
        len += sprintf(request + len, "Content-Length: %d\r\n", post_len);
    }
    
    /* End of headers */
    len += sprintf(request + len, "\r\n");
    
    /* Add POST data if present */
    if (options->method == HTTP_POST && options->post_data != NULL) {
        len += sprintf(request + len, "%s", options->post_data);
    }
    
    return len;
}

/*
 * Parse HTTP response status line
 */
static int parse_status_line(const char *line, http_response_t *response) {
    char version[16];
    const char *msg_start;
    char *end;
    
    if (sscanf(line, "%s %d", version, &response->status_code) != 2) {
        return -1;
    }
    
    /* Extract status message (everything after status code) */
    msg_start = strchr(line, ' ');
    if (msg_start != NULL) {
        msg_start = strchr(msg_start + 1, ' ');
        if (msg_start != NULL) {
            msg_start++; /* Skip space */
            strncpy(response->status_message, msg_start, 
                   sizeof(response->status_message) - 1);
            response->status_message[sizeof(response->status_message) - 1] = '\0';
            
            /* Remove trailing \r\n */
            end = strchr(response->status_message, '\r');
            if (end != NULL) *end = '\0';
            end = strchr(response->status_message, '\n');
            if (end != NULL) *end = '\0';
        }
    }
    
    return 0;
}

/*
 * Parse HTTP response
 */
int http_parse_response(const char *raw_response, http_response_t *response) {
    const char *ptr = raw_response;
    const char *line_end;
    const char *body_start;
    char line[MAX_HEADER_LENGTH];
    int line_len;
    
    if (raw_response == NULL || response == NULL) {
        return -1;
    }
    
    /* Parse status line */
    line_end = strstr(ptr, "\r\n");
    if (line_end == NULL) {
        return -1;
    }
    
    line_len = line_end - ptr;
    if (line_len >= MAX_HEADER_LENGTH) {
        line_len = MAX_HEADER_LENGTH - 1;
    }
    
    strncpy(line, ptr, line_len);
    line[line_len] = '\0';
    
    if (parse_status_line(line, response) != 0) {
        return -1;
    }
    
    ptr = line_end + 2; /* Skip \r\n */
    
    /* Parse headers */
    while (*ptr != '\0') {
        line_end = strstr(ptr, "\r\n");
        if (line_end == NULL) {
            break;
        }
        
        line_len = line_end - ptr;
        
        /* Empty line indicates end of headers */
        if (line_len == 0) {
            ptr = line_end + 2;
            break;
        }
        
        /* Store header if we have space */
        if (response->num_headers < 20) {
            if (line_len >= MAX_HEADER_LENGTH) {
                line_len = MAX_HEADER_LENGTH - 1;
            }
            strncpy(response->headers[response->num_headers], ptr, line_len);
            response->headers[response->num_headers][line_len] = '\0';
            response->num_headers++;
        }
        
        ptr = line_end + 2;
    }
    
    /* Rest is body */
    body_start = ptr;
    response->body_length = strlen(body_start);
    
    if (response->body_length > 0) {
        response->body = (char *)malloc(response->body_length + 1);
        if (response->body != NULL) {
            memcpy(response->body, body_start, response->body_length);
            response->body[response->body_length] = '\0';
        }
    }
    
    return 0;
}

/*
 * Print HTTP response
 */
void http_print_response(const http_response_t *response, const options_t *options) {
    int i;
    FILE *output = stdout;
    
    if (response == NULL) {
        return;
    }
    
    /* Open output file if specified */
    if (options->output_file != NULL) {
        output = fopen(options->output_file, "wb");
        if (output == NULL) {
            print_error("Failed to open output file");
            output = stdout;
        }
    }
    
    /* Print status in verbose mode */
    if (options->verbose) {
        printf("< HTTP/1.0 %d %s\n", response->status_code, response->status_message);
        
        /* Print headers */
        for (i = 0; i < response->num_headers; i++) {
            printf("< %s\n", response->headers[i]);
        }
        printf("<\n");
    }
    
    /* Print body */
    if (response->body != NULL && response->body_length > 0) {
        fwrite(response->body, 1, response->body_length, output);
        if (output == stdout) {
            printf("\n");
        }
    }
    
    /* Close output file if opened */
    if (output != stdout) {
        fclose(output);
        printf("Output written to: %s\n", options->output_file);
    }
}

/*
 * Perform HTTP request
 */
int http_request(const url_t *url, const options_t *options, http_response_t *response) {
    socket_t sock;
    char request[MAX_BUFFER_SIZE];
    char recv_buffer[MAX_BUFFER_SIZE];
    char *full_response;
    char *new_buffer;
    int full_response_capacity;
    int request_len;
    int received;
    int total_received;
    char msg[128];
    
    full_response = NULL;
    full_response_capacity = MAX_BUFFER_SIZE;
    total_received = 0;
    
    if (url == NULL || options == NULL || response == NULL) {
        return ERROR_HTTP;
    }
    
    /* Build HTTP request */
    request_len = build_http_request(url, options, request, sizeof(request));
    
    if (options->verbose) {
        printf("> %s %s HTTP/1.0\n", 
               (options->method == HTTP_POST) ? "POST" : "GET", 
               url->path);
        printf("> Host: %s\n", url->hostname);
        printf("> User-Agent: %s\n", DOSCURL_USER_AGENT);
        printf(">\n");
    }
    
    /* Connect to server */
    sock = socket_connect(url->hostname, url->port);
    if (sock < 0) {
        print_error("Failed to connect to server");
        return ERROR_NETWORK;
    }
    
    /* Send request */
    if (socket_send(sock, request, request_len) < 0) {
        print_error("Failed to send HTTP request");
        socket_close(sock);
        return ERROR_NETWORK;
    }
    
    /* Allocate initial response buffer */
    full_response = (char *)malloc(full_response_capacity);
    if (full_response == NULL) {
        print_error("Failed to allocate response buffer");
        socket_close(sock);
        return ERROR_MEMORY;
    }
    
    /* Receive response */
    print_verbose("Receiving response...");
    
    while (1) {
        received = socket_recv(sock, recv_buffer, sizeof(recv_buffer) - 1);
        
        if (received < 0) {
            print_error("Error receiving data");
            free(full_response);
            socket_close(sock);
            return ERROR_NETWORK;
        }
        
        if (received == 0) {
            /* Connection closed or no more data */
            break;
        }
        
        /* Expand buffer if needed */
        if (total_received + received >= full_response_capacity) {
            full_response_capacity *= 2;
            new_buffer = (char *)realloc(full_response, full_response_capacity);
            if (new_buffer == NULL) {
                print_error("Failed to expand response buffer");
                free(full_response);
                socket_close(sock);
                return ERROR_MEMORY;
            }
            full_response = new_buffer;
        }
        
        /* Append received data */
        memcpy(full_response + total_received, recv_buffer, received);
        total_received += received;
    }
    
    /* Null-terminate response */
    full_response[total_received] = '\0';
    
    sprintf(msg, "Received total of %d bytes", total_received);
    print_verbose(msg);
    
    /* Close connection */
    socket_close(sock);
    
    /* Parse response */
    if (http_parse_response(full_response, response) != 0) {
        print_error("Failed to parse HTTP response");
        free(full_response);
        return ERROR_HTTP;
    }
    
    free(full_response);
    
    return SUCCESS;
}

/*
 * Perform HTTP GET request
 */
int http_get(const url_t *url, const options_t *options, http_response_t *response) {
    return http_request(url, options, response);
}

/*
 * Perform HTTP POST request
 */
int http_post(const url_t *url, const options_t *options, http_response_t *response) {
    return http_request(url, options, response);
}

// Made with Bob
