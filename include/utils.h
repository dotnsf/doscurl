/*
 * DOSCurl - Utility Functions
 * Header file for utility and helper functions
 */

#ifndef UTILS_H
#define UTILS_H

#include "doscurl.h"

/* Function prototypes */
void print_error(const char *message);
void print_verbose(const char *message);
void print_usage(void);
char *str_trim(char *str);
int str_starts_with(const char *str, const char *prefix);
int str_case_compare(const char *s1, const char *s2);
char *str_duplicate(const char *str);

#endif /* UTILS_H */

// Made with Bob
