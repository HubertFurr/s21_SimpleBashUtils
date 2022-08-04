#ifndef SRC_CAT_S21_CAT_H_
#define SRC_CAT_S21_CAT_H_

#define PNAME "s21_cat"
#define FLAGS "bevnstET-"

typedef struct s21_cat_settings {
    char **files;
    int num_files;
    int num_all_lines;
    int num_not_empty_lines;
    int show_hidden;
    int show_tab;
    int show_line_feed;
    int squeeze_blank;
    int num_flags;
} s21_cat_settings;

typedef struct s21_cat_state {
    int line_cnt;
    int num_chars;
    int prev_file_n;
    int ch;
    int previous_ch;
    int num_line_feeds_in_row;
    int is_new_line;
    int is_print_line_num;
} s21_cat_state;

void s21_cat_print_error(char *str);
void s21_cat_print_dir_error(char *filename);
void s21_cat_print_fopen_error(char *filename);
void s21_cat_print_usage_error(char *str);
s21_cat_settings s21_get_cat_settings(int argc, char *argv[]);
void s21_cat_parse_short_settings(char argv, s21_cat_settings *settings);
void s21_cat_parse_long_settings(char *argv, s21_cat_settings *settings);
void s21_print_settings(s21_cat_settings settings);
void s21_destroy_settings(s21_cat_settings settings);
void s21_cat_file(char *filename, s21_cat_settings settings);
void s21_reset_state(s21_cat_state *state);
void s21_print_char(s21_cat_state *state, s21_cat_settings settings);
void s21_print_special_char(int c, s21_cat_settings settings);
void s21_cat_print_line_num_linux(int line_cnt, s21_cat_settings settings);
void s21_cat_print_line_num_apple(int line_cnt);
void s21_cat_print_end_file_symbol(s21_cat_state *state, s21_cat_settings settings);
void s21_cat_proccess_flag_b(s21_cat_state *state, s21_cat_settings settings);
void s21_cat_proccess_flag_n(s21_cat_state *state, s21_cat_settings settings);
int s21_cat_proccess_flag_s(s21_cat_state *state, s21_cat_settings settings);

#endif  // SRC_CAT_S21_CAT_H_
