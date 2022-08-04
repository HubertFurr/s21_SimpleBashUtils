#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

void s21_print_error(char *pname, char *str) {
    fprintf(stderr, "%s: %s\n", pname, str);
}

void *s21_calloc(size_t size) {
    void *result;
    errno = 0;
    result = malloc(size);
    if (result == NULL) {
        s21_print_error("", strerror(errno));
        exit(1);
    } else {
        memset(result, 0, size);
    }
    return result;
}

void *s21_realloc(void *memblock, size_t size) {
    void *result;
    errno = 0;
    result = realloc(memblock, size);
    if (result == NULL) {
        s21_print_error("", strerror(errno));
        exit(1);
    }

    return result;
}

void s21_push_to_string_array(char ***array, char *element, int *num) {
    int num_of_els = *num;
    if (num_of_els == 0) {
        *array = (char **) s21_calloc(sizeof(char*));
    } else {
        *array = (char **) s21_realloc(*array, (unsigned long) (num_of_els + 1) * sizeof(char*));
    }
    (*array)[num_of_els] = (char *) s21_calloc(sizeof(char) * (strlen(element) + 1));
    strcpy((*array)[num_of_els], element);
    num_of_els++;
    *num = num_of_els;
}

char *s21_pull_from_string_array(char ***array, int *num) {
    char *result = NULL;
    if (*num > 0) {
        result = s21_calloc(sizeof(char*) * (strlen((*array)[0]) + 1));
        strcpy(result, (*array)[0]);

        for (int i = 0; i < *num - 1; i++) {
            (*array)[i] = s21_realloc((*array)[i], sizeof(char*) * (strlen((*array)[i + 1]) + 1));
            strcpy((*array)[i], (*array)[i + 1]);
        }

        free((*array)[*num - 1]);
        (*num)--;
    }
    return result;
}
