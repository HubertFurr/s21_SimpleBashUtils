#ifndef SRC_COMMON_COMMON_H_
#define SRC_COMMON_COMMON_H_

#include <stdlib.h>

void s21_print_error(char *pname, char *str);
void *s21_calloc(size_t size);
void *s21_realloc(void *memblock, size_t size);
void s21_push_to_string_array(char ***array, char *element, int *num);
char *s21_pull_from_string_array(char ***array, int *num);

#endif  // SRC_COMMON_COMMON_H_
