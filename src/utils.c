/*
 * DOSCurl - Utility Functions
 * Implementation of utility and helper functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/utils.h"

/* Global verbose flag */
extern int g_verbose;

/*
 * Print error message to stderr
 */
void print_error(const char *message) {
    fprintf(stderr, "Error: %s\n", message);
}

/*
 * Print verbose message if verbose mode is enabled
 */
void print_verbose(const char *message) {
    if (g_verbose) {
        printf("* %s\n", message);
    }
}

/*
 * Print usage information
 */
void print_usage(void) {
    printf("DOSCurl - HTTP client for DOS\n");
    printf("Version: %s\n\n", DOSCURL_VERSION);
    printf("Usage: doscurl [options] <URL>\n\n");
    printf("Options:\n");
    printf("  -X METHOD     HTTP method (GET, POST)\n");
    printf("  -d DATA       POST data\n");
    printf("  -H HEADER     Custom header\n");
    printf("  -v            Verbose output\n");
    printf("  -o FILE       Output to file\n");
    printf("  --help        Show this help\n\n");
    printf("Examples:\n");
    printf("  doscurl http://example.com/\n");
    printf("  doscurl -X POST -d \"key=value\" http://example.com/api\n");
    printf("  doscurl -v -o output.txt http://example.com/data.json\n");
}

/*
 * Trim whitespace from both ends of a string
 */
char *str_trim(char *str) {
    char *end;
    
    /* Trim leading space */
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) {
        return str;
    }
    
    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    /* Write new null terminator */
    *(end + 1) = '\0';
    
    return str;
}

/*
 * Check if string starts with prefix
 */
int str_starts_with(const char *str, const char *prefix) {
    size_t len_prefix = strlen(prefix);
    size_t len_str = strlen(str);
    
    if (len_str < len_prefix) {
        return 0;
    }
    
    return strncmp(str, prefix, len_prefix) == 0;
}

/*
 * Case-insensitive string comparison
 */
int str_case_compare(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = tolower((unsigned char)*s1);
        char c2 = tolower((unsigned char)*s2);
        
        if (c1 != c2) {
            return c1 - c2;
        }
        
        s1++;
        s2++;
    }
    
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

/*
 * Duplicate a string (similar to strdup)
 */
char *str_duplicate(const char *str) {
    size_t len;
    char *dup;
    
    if (str == NULL) {
        return NULL;
    }
    
    len = strlen(str) + 1;
    dup = (char *)malloc(len);
    
    if (dup == NULL) {
        return NULL;
    }
    
    memcpy(dup, str, len);
    return dup;
}