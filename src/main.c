/*
 * DOSCurl - HTTP Client for 16-bit DOS
 * Main entry point
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/doscurl.h"
#include "../include/utils.h"
#include "../include/url.h"
#include "../include/socket.h"
#include "../include/http.h"

/* Global verbose flag */
int g_verbose = 0;

/*
 * Parse command line arguments
 */
int parse_arguments(int argc, char *argv[], options_t *options) {
    int i;
    
    /* Initialize options */
    memset(options, 0, sizeof(options_t));
    options->method = HTTP_GET;
    options->verbose = 0;
    options->post_data = NULL;
    options->output_file = NULL;
    options->num_custom_headers = 0;
    
    /* Check if no arguments */
    if (argc < 2) {
        print_usage();
        return -1;
    }
    
    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            return -1;
        }
        else if (strcmp(argv[i], "-v") == 0) {
            options->verbose = 1;
            g_verbose = 1;
        }
        else if (strcmp(argv[i], "-X") == 0) {
            if (i + 1 >= argc) {
                print_error("Missing argument for -X");
                return -1;
            }
            i++;
            if (str_case_compare(argv[i], "GET") == 0) {
                options->method = HTTP_GET;
            }
            else if (str_case_compare(argv[i], "POST") == 0) {
                options->method = HTTP_POST;
            }
            else if (str_case_compare(argv[i], "HEAD") == 0) {
                options->method = HTTP_HEAD;
            }
            else {
                print_error("Unsupported HTTP method");
                return -1;
            }
        }
        else if (strcmp(argv[i], "-d") == 0) {
            if (i + 1 >= argc) {
                print_error("Missing argument for -d");
                return -1;
            }
            i++;
            options->post_data = argv[i];
            options->method = HTTP_POST;
        }
        else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                print_error("Missing argument for -o");
                return -1;
            }
            i++;
            options->output_file = argv[i];
        }
        else if (strcmp(argv[i], "-H") == 0) {
            if (i + 1 >= argc) {
                print_error("Missing argument for -H");
                return -1;
            }
            i++;
            if (options->num_custom_headers < 10) {
                strncpy(options->custom_headers[options->num_custom_headers],
                       argv[i], MAX_HEADER_LENGTH - 1);
                options->custom_headers[options->num_custom_headers][MAX_HEADER_LENGTH - 1] = '\0';
                options->num_custom_headers++;
            }
            else {
                print_error("Too many custom headers (max 10)");
                return -1;
            }
        }
        else if (argv[i][0] != '-') {
            /* This should be the URL */
            strncpy(options->url, argv[i], MAX_URL_LENGTH - 1);
            options->url[MAX_URL_LENGTH - 1] = '\0';
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return -1;
        }
    }
    
    /* Check if URL was provided */
    if (options->url[0] == '\0') {
        print_error("No URL specified");
        print_usage();
        return -1;
    }
    
    return 0;
}

/*
 * Main entry point
 */
int main(int argc, char *argv[]) {
    options_t options;
    url_t url;
    http_response_t response;
    int result;
    
    printf("DOSCurl v%s - HTTP Client for DOS\n\n", DOSCURL_VERSION);
    
    /* Parse command line arguments */
    if (parse_arguments(argc, argv, &options) != 0) {
        return 1;
    }
    
    /* Parse URL */
    if (parse_url(options.url, &url) != 0) {
        print_error("Invalid URL format");
        return 1;
    }
    
    if (options.verbose) {
        printf("* Parsed URL:\n");
        printf("*   Protocol: %s\n", url.protocol);
        printf("*   Hostname: %s\n", url.hostname);
        printf("*   Port: %d\n", url.port);
        printf("*   Path: %s\n", url.path);
    }
    
    /* Initialize network */
    if (socket_init() != 0) {
        print_error("Failed to initialize network");
        return 1;
    }
    
    /* Initialize response structure */
    http_response_init(&response);
    
    /* Perform HTTP request */
    result = http_request(&url, &options, &response);
    
    if (result == 0) {
        /* Print response */
        http_print_response(&response, &options);
    }
    else {
        print_error("HTTP request failed");
    }
    
    /* Cleanup */
    http_response_free(&response);
    socket_cleanup();
    
    return (result == 0) ? 0 : 1;
}