#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include "./s21_cat.h"
#include "./../common/common.h"

void s21_cat_print_error(char *str) {
    s21_print_error(PNAME, str);
}

void s21_cat_print_dir_error(char *filename) {
    char *error_dir_open = "Is a directory";
    char error_dir_open_full[256];
    sprintf(error_dir_open_full, "%s: %s", filename, error_dir_open);
    s21_cat_print_error(error_dir_open_full);
}

void s21_cat_print_fopen_error(char *filename) {
    char *error = strerror(errno);
    char *error_full = s21_calloc(strlen(filename) + 255);
    sprintf(error_full, "%s: %s", filename, error);
    s21_cat_print_error(error_full);
    free(error_full);
}

void s21_cat_print_usage_error(char *str) {
    char *error_arg_long_full = s21_calloc(strlen(str) + 255);
    sprintf(error_arg_long_full,
        "illegal option %s\nusage: s21_cat [-benstuv] [file ...]", str);
    s21_cat_print_error(error_arg_long_full);
    free(error_arg_long_full);
}


s21_cat_settings s21_get_cat_settings(int argc, char *argv[]) {
    if (argc == 1) {
        s21_cat_print_error("Command line has no arguments");
        exit(0);
    }
    s21_cat_settings settings = {NULL, 0, 0, 0, 0, 0, 0, 0, 0};
    #if defined(__APPLE__)
    int save_files_name = 0;
    #endif
    for (int i = 1 ; i < argc; i++) {
        #if defined(__APPLE__)
        if (save_files_name == 0 && argv[i][0] == '-') {
        #else
        if (argv[i][0] == '-') {
        #endif
            settings.num_flags++;
            if (argv[i][1] == '-') {
                s21_cat_parse_long_settings(argv[i], &settings);
            } else {
                for (size_t j = 1; j < strlen(argv[i]); j++) {
                    s21_cat_parse_short_settings(argv[i][j], &settings);
                }
            }
        } else {
            #if defined(__APPLE__)
            save_files_name = 1;
            #endif
            s21_push_to_string_array(&settings.files, argv[i], &settings.num_files);
        }
    }

    return settings;
}

void s21_cat_parse_short_settings(char argv, s21_cat_settings *settings) {
    if (strchr(FLAGS, argv) == NULL) {
        char error_arg_short[2];
        sprintf(error_arg_short, "%c", argv);
        s21_cat_print_error(error_arg_short);
        exit(0);
    } else {
        switch (argv) {
        case 'n':
            if (settings->num_not_empty_lines != 1) {
                settings->num_all_lines = 1;
            }
            break;
        case 'b':
            settings->num_not_empty_lines = 1;
            settings->num_all_lines = 0;
            break;
        case 'v':
            settings->show_hidden = 1;
            break;
        case 'e':
            settings->show_line_feed = 1;
            settings->show_hidden = 1;
            break;
        case 'E':
            settings->show_line_feed = 1;
            break;
        case 't':
            settings->show_tab = 1;
            settings->show_hidden = 1;
            break;
        case 'T':
            settings->show_tab = 1;
            break;
        case 's':
            settings->squeeze_blank = 1;
            break;
        default:
            break;
        }
    }
}

void s21_cat_parse_long_settings(char *argv, s21_cat_settings *settings) {
    if (strcmp(argv, "--number-nonblank") == 0) {
        settings->num_not_empty_lines = 1;
        settings->num_all_lines = 0;
    } else if (strcmp(argv, "--number") == 0) {
        if (settings->num_not_empty_lines != 1) {
            settings->num_all_lines = 1;
        }
    } else if (strcmp(argv, "--squeeze-blank") == 0) {
        settings->squeeze_blank = 1;
    } else {
        s21_cat_print_usage_error(argv);
        exit(0);
    }
}

void s21_print_settings(s21_cat_settings settings) {
    printf("\n");
    printf(
        "num_all_lines:[%d]\nnum_not_empty_lines:[%d]\n",
        settings.num_all_lines, settings.num_not_empty_lines);

    for (int i = 0; i < settings.num_files; i++) {
        if (i == 0) {
            printf("Files:\n");
        }
        printf("  [%s]\n", settings.files[i]);
    }
    printf("\n");
}

void s21_destroy_settings(s21_cat_settings settings) {
    for (int i = 0; i < settings.num_files; i++) {
        free(settings.files[i]);
    }
    free(settings.files);
}

void s21_reset_state(s21_cat_state *state) {
    #if defined(__APPLE__)
    state->line_cnt = 0;
    #endif
    state->num_chars = 0;
    state->ch = -1;
    state->previous_ch = -1;
    state->num_line_feeds_in_row = 0;
    state->is_new_line = 1;
    state->is_print_line_num = 0;
}

void s21_cat_file(char *filename, s21_cat_settings settings) {
    static struct s21_cat_state state = {0, 0, -1, -1, -1, 0, 1, 0};

    DIR *dir;
    dir = opendir(filename);
    if (dir) {
        #if defined(__linux__)
        if (settings.num_flags == 0)
        #endif
            s21_cat_print_dir_error(filename);
        closedir(dir);
    } else {
        errno = 0;
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            s21_cat_print_fopen_error(filename);
        } else {
            s21_reset_state(&state);

            #if defined(__linux__)
            if (settings.show_hidden == 1 && state.prev_file_n == 0) {
                state.is_new_line = 0;
            }
            #endif

            while (1) {
                int ch = fgetc(fp);

                if (feof(fp)) {
                    break;
                }

                state.ch = ch;

                if (s21_cat_proccess_flag_s(&state, settings) == 1) {
                    continue;
                }

                if (settings.num_not_empty_lines == 1) {
                    s21_cat_proccess_flag_b(&state, settings);
                } else if (settings.num_all_lines == 1) {
                    s21_cat_proccess_flag_n(&state, settings);
                }

                if (state.is_print_line_num == 1) {
                    #if defined(__linux__)
                    s21_cat_print_line_num_linux(state.line_cnt, settings);
                    #else
                    printf("%6d\t", state.line_cnt);
                    #endif
                    state.is_print_line_num = 0;
                }

                state.num_chars++;
                s21_print_char(&state, settings);
                state.previous_ch = state.ch;
            }

            #if defined(__linux__)
            s21_cat_print_end_file_symbol(&state, settings);
            #endif

            fclose(fp);
        }
    }
}

void s21_print_char(s21_cat_state *state, s21_cat_settings settings) {
    #if defined(__linux__)
    if (state->ch == 0 && (settings.num_not_empty_lines == 1 || settings.num_all_lines == 1)
        && settings.show_hidden == 0) {
        printf("\n");
    } else {
        s21_print_special_char(state->ch, settings);
    }
    #else
    s21_print_special_char(state->ch, settings);
    #endif
}

void s21_print_special_char(int c, s21_cat_settings settings) {
    if (settings.show_hidden == 0 && settings.show_line_feed == 0 && settings.show_tab == 0) {
        printf("%c", c);
    } else {
        if (c == 9) {   //  \t
            if (settings.show_tab == 1) {
                printf("^I");
            } else {
                printf("%c", c);
            }
        } else if (c == 10) {   //  \n
            if (settings.show_line_feed == 1) {
                printf("$%c", c);
            } else {
                printf("%c", c);
            }
        } else if (c < 32) {
            if (settings.show_hidden == 1) {
                printf("^%c", c + 64);
            } else {
                printf("%c", c);
            }
        } else if (c < 127) {
            printf("%c", c);
        } else if (c == 127) {
            if (settings.show_hidden == 1) {
                printf("^?");
            } else {
                printf("%c", c);
            }
        } else if (c < 160) {
            if (settings.show_hidden == 1) {
                printf("M-^%c", c + 192);
            } else {
                printf("%c", c);
            }
        } else if (c <= 255) {
            #if defined(__linux__)
            if (settings.show_hidden == 1) {
                if (c == 255) {
                    printf("M-^?");
                } else {
                    printf("M-%c", c + 128);
                }
            } else {
                printf("%c", c);
            }
            #else
            printf("%c", c);
            #endif
        } else {
            printf("%c", c);
        }
    }
}

void s21_cat_print_line_num_linux(int line_cnt, s21_cat_settings settings) {
    if (settings.show_hidden == 1) {
        printf("%6d  ", line_cnt);
    } else {
        printf("%6d\t", line_cnt);
    }
}

void s21_cat_print_end_file_symbol(s21_cat_state *state, s21_cat_settings settings) {
    if (state->num_chars > 0) {
        state->prev_file_n = -1;
        if (state->previous_ch != '\n' && state->previous_ch != 0 &&
            (settings.num_not_empty_lines == 1 || settings.num_all_lines == 1)
            && settings.show_hidden == 0) {
            printf("\n");
        }
        if (state->previous_ch != '\n') {
            state->prev_file_n = 0;
        }
    }
}

void s21_cat_proccess_flag_b(s21_cat_state *state, s21_cat_settings settings) {
    if (state->is_new_line == 1) {
        #if defined(__linux__)
        if (!(state->ch == '\n' || (state->ch == 0 &&
            (settings.num_not_empty_lines == 1 || settings.num_all_lines == 1)
            && settings.show_hidden == 0))) {
        #else
        if (state->ch != '\n') {
        #endif
            state->is_print_line_num = 1;
            state->line_cnt++;
        }
        state->is_new_line = 0;
    }

    #if defined(__linux__)
    if (state->ch == '\n' || (state->ch == 0 &&
        (settings.num_not_empty_lines == 1 || settings.num_all_lines == 1)
        && settings.show_hidden == 0)) {
    #else
    if (state->ch == '\n') {
    #endif
        state->is_new_line = 1;
    }

    #if defined(__APPLE__)
    if (settings.show_hidden == 1 && state->ch == 138) {
        state->is_new_line = 1;
    }
    #endif
}

void s21_cat_proccess_flag_n(s21_cat_state *state, s21_cat_settings settings) {
    if (state->is_new_line == 1) {
        state->is_print_line_num = 1;
        state->is_new_line = 0;
        state->line_cnt++;
    }
    #if defined(__linux__)
    if (state->ch == '\n' || (state->ch == 0 &&
        (settings.num_not_empty_lines == 1 || settings.num_all_lines == 1)
        && settings.show_hidden == 0)) {
    #else
    if (state->ch == '\n') {
    #endif
        state->is_new_line = 1;
    }
    #if defined(__APPLE__)
    if (settings.show_hidden == 1 && state->ch == 138) {
        state->is_new_line = 1;
    }
    #endif
}

int s21_cat_proccess_flag_s(s21_cat_state *state, s21_cat_settings settings) {
    int result = 0;
    if (settings.squeeze_blank == 1) {
        if (state->ch == '\n' && (state->previous_ch == '\n' || state->previous_ch == -1)) {
            state->num_line_feeds_in_row++;
        } else {
            state->num_line_feeds_in_row = 0;
        }

        if (state->num_line_feeds_in_row > 1) {
            result = 1;
        }
    }

    return result;
}
