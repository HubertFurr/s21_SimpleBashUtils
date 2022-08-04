#ifndef SRC_GREP_S21_GREP_H_
#define SRC_GREP_S21_GREP_H_

#include <regex.h>

#define PNAME "s21_grep"
#define MIN_MALLOC_SIZE 32

typedef struct s21_grep_settings {
    char **files;
    int num_files;
    char **patterns;
    int num_patterns;
    int has_empty_pattern;
    int ignore_case;
    int invert_match;
    int show_count_lines_only;
    int show_files_name_only;
    int show_lines_num;
    int show_no_files_name;
    int silent_mode;
    int show_matching_only;
    int regexp_cflags;
} s21_grep_settings;

typedef struct s21_string {
    char *data;
    char *file_name;
    unsigned long line_num;
} s21_string;

void s21_grep_destroy_settings_files(s21_grep_settings *settings);
void s21_grep_destroy_settings_patterns(s21_grep_settings *settings);
void s21_grep_destroy_settings(s21_grep_settings *settings);
void s21_grep_print_instructions_and_exit();
void s21_grep_print_error(char *str);
s21_grep_settings s21_grep_get_settings(int argc, char *argv[]);
void s21_grep_print_settings(s21_grep_settings settings);
void s21_grep_parse_ffiles(s21_grep_settings *settings, char *filename);
void print_nchars(char *str, regoff_t n);
int s21_grep_file(char *filename, s21_grep_settings settings);
int s21_grep_line_apple(s21_string line, s21_grep_settings settings);
int s21_grep_line_linux(s21_string line, s21_grep_settings settings);
int s21_grep_print_line_apple(s21_string line,
 s21_grep_settings settings, regmatch_t *matches, int num_matches);
int s21_grep_print_line_linux(s21_string line,
 s21_grep_settings settings, regmatch_t *matches, int num_matches);
void s21_grep_print_lc_data_apple(s21_grep_settings settings, char *filename, int num_matches);
void s21_grep_print_lc_data_linux(s21_grep_settings settings, char *filename, int num_matches);

#endif  // SRC_GREP_S21_GREP_H_
