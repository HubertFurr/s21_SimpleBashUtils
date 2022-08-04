#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <dirent.h>
#include <locale.h>
#include <string.h>
#include <sys/stat.h>

#include "./s21_grep.h"
#include "./../common/common.h"

#include <regex.h>
#ifndef REG_STARTEND
#define REG_STARTEND 00004
#endif

void s21_grep_destroy_settings_files(s21_grep_settings *settings) {
    for (int i = 0; i < settings->num_files; i++) {
        free(settings->files[i]);
    }
    if (settings->num_files > 0) {
        free(settings->files);
        settings->num_files = 0;
    }
}

void s21_grep_destroy_settings_patterns(s21_grep_settings *settings) {
    for (int i = 0; i < settings->num_patterns; i++) {
        free(settings->patterns[i]);
    }
    if (settings->num_patterns > 0) {
        free(settings->patterns);
        settings->num_patterns = 0;
    }
}

void s21_grep_destroy_settings(s21_grep_settings *settings) {
    s21_grep_destroy_settings_files(settings);
    s21_grep_destroy_settings_patterns(settings);
}

void s21_grep_print_instructions_and_exit() {
    fprintf(stderr,
     "usage: grep [-ivclnhso] [-e pattern] [-f file] [--color=when]\n\t[pattern] [file ...]\n");
    exit(0);
}

void s21_grep_print_error(char *str) {
    s21_print_error(PNAME, str);
}

s21_grep_settings s21_grep_get_settings(int argc, char *argv[]) {
    s21_grep_settings settings = {NULL, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const char* short_options = "e:ivclnhsof:";
    const struct option long_options[] = {
        { "regexp", required_argument, NULL, 'e' },
        { "ignore-case", no_argument, NULL, 'i' },
        { "invert-match", no_argument, NULL, 'v' },
        { "count", no_argument, NULL, 'c' },
        { "files-with-matches", no_argument, NULL, 'l' },
        { "line-number", no_argument, NULL, 'n' },
        { "no-filename", no_argument, NULL, 'h' },
        { "no-messages", no_argument, NULL, 's' },
        { "file", required_argument, NULL, 'f' },
        { "only-matching", no_argument, NULL, 'o' },
        { NULL, 0, NULL, 0 }
    };
    int res;
    int exist_flag_f = 0;

    settings.regexp_cflags |= REG_NEWLINE;
    #if defined(__APPLE__)
    settings.regexp_cflags |= REG_ENHANCED;
    #endif

    while ((res = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (res) {
            case 'e':
                s21_push_to_string_array(&settings.patterns, optarg, &settings.num_patterns);
                break;
            case 'i':
                settings.ignore_case = 1;
                break;
            case 'v':
                settings.invert_match = 1;
                break;
            case 'c':
                settings.show_count_lines_only = 1;
                break;
            case 'l':
                settings.show_files_name_only = 1;
                break;
            case 'n':
                settings.show_lines_num = 1;
                break;
            case 'h':
                settings.show_no_files_name = 1;
                break;
            case 's':
                settings.silent_mode = 1;
                break;
            case 'f':
            exist_flag_f = 1;
                s21_grep_parse_ffiles(&settings, optarg);
                break;
            case 'o':
                settings.show_matching_only = 1;
                break;
            case '?':
            default:
                s21_grep_print_instructions_and_exit();  // unknown option or option requires an argument
                break;
        }
    }

    if (optind < argc) {
        while (optind < argc) {
            s21_push_to_string_array(&settings.files, argv[optind], &settings.num_files);
            optind++;
        }
    } else {
        s21_grep_print_instructions_and_exit();
    }

    if (settings.num_patterns == 0 && exist_flag_f == 0) {
        char *pull_str = s21_pull_from_string_array(&settings.files, &settings.num_files);
        if (pull_str != NULL) {
            s21_push_to_string_array(&settings.patterns, pull_str, &settings.num_patterns);
        }
        free(pull_str);
    }

    if (settings.ignore_case == 1) {
        settings.regexp_cflags |= REG_ICASE;
    }

    for (int i = 0; i < settings.num_patterns; i++) {
        if (strlen(settings.patterns[i]) == 0) {
            settings.has_empty_pattern = 1;
            break;
        }
    }

    return settings;
}

void s21_grep_print_settings(s21_grep_settings settings) {
    printf("\n");
    char format[1024];
    strcpy(format, "ignore_case:[%d]\n");
    strcat(format, "invert_match:[%d]\n");
    strcat(format, "show_count_lines_only:[%d]\n");
    strcat(format, "show_files_name_only:[%d]\n");
    strcat(format, "show_lines_num:[%d]\n");
    strcat(format, "show_no_files_name:[%d]\n");
    strcat(format, "silent_mode:[%d]\n");
    strcat(format, "show_matching_only:[%d]\n");
    printf(
     format,
     settings.ignore_case, settings.invert_match, settings.show_count_lines_only,
     settings.show_files_name_only, settings.show_lines_num, settings.show_no_files_name,
     settings.silent_mode, settings.show_matching_only);
    printf("\n");

    printf("Files(%d):\n", settings.num_files);
    for (int i = 0; i < settings.num_files; i++) {
        printf("  [%s]\n", settings.files[i]);
    }
    printf("\n");

    printf("Patterns(%d):\n", settings.num_patterns);
    for (int i = 0; i < settings.num_patterns; i++) {
        printf("  [%s]\n", settings.patterns[i]);
    }
    printf("\n");
}

void s21_grep_parse_ffiles(s21_grep_settings *settings, char *filename) {
    DIR *dir;
    dir = opendir(filename);
    if (dir) {
        closedir(dir);
    } else {
        errno = 0;
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            char *error = strerror(errno);
            char *error_full = s21_calloc(sizeof(char) * (strlen(filename) + 255));
            sprintf(error_full, "%s: %s", filename, error);
            s21_grep_print_error(error_full);
            free(error_full);
            exit(0);
        } else {
            char *line = NULL;
            size_t len = 0;
            while ((getline(&line, &len, fp)) != -1) {
                size_t line_len = strlen(line);
                if (line[line_len - 1] == '\n') {
                    line[line_len - 1] = '\0';
                }

                s21_push_to_string_array(&settings->patterns, line, &settings->num_patterns);
            }
            free(line);
            fclose(fp);
        }
    }
}

int s21_grep_file(char *filename, s21_grep_settings settings) {
    int num_matches = -1;

    DIR *dir = opendir(filename);
    if (dir) {
        char *error_dir_open = "Is a directory";
        char error_dir_open_full[512];
        sprintf(error_dir_open_full, "%s: %s", filename, error_dir_open);
        #if defined(__APPLE__)
        s21_grep_print_error(error_dir_open_full);
        #endif
        closedir(dir);
    } else {
        errno = 0;
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            if (settings.silent_mode == 0) {
                char *error = strerror(errno);
                char *error_full = s21_calloc(sizeof(char) * (strlen(filename) + 255));
                sprintf(error_full, "%s: %s", filename, error);
                s21_grep_print_error(error_full);
                free(error_full);
            }
        } else {
            #if defined(__APPLE__)
            struct stat fpstat;
            int fd;
            fd = fileno(fp);
            fstat(fd, &fpstat);
            if (fpstat.st_size == 0 && settings.has_empty_pattern == 1) {
                fclose(fp);
                return -2;  //  error with empty file at Mac
            }
            #endif

            char *line = NULL;
            size_t len = 0;
            unsigned long string_num = 0;
            num_matches = 0;
            while ((getline(&line, &len, fp)) != -1) {
                s21_string str;
                if (strlen(line) > 0) {
                    if (line[strlen(line) - 1] == '\n') {
                        line[strlen(line) - 1] = '\0';
                    }
                }
                string_num++;
                str.data = line;
                str.file_name = filename;
                str.line_num = string_num;

                #if defined(__APPLE__)
                int res = s21_grep_line_apple(str, settings);
                #else
                int res = s21_grep_line_linux(str, settings);
                #endif

                if (res > 0) {
                    num_matches++;
                    if (settings.show_files_name_only == 1) {
                        break;
                    }
                }
            }
            free(line);
            fclose(fp);
        }
    }
    return num_matches;
}

void print_nchars(char *str, regoff_t n) {
    for (long long int i = 0; i < n; i++) {
        putchar(str[i]);
    }
}

int s21_grep_line_apple(s21_string line, s21_grep_settings settings) {
    int num_matches = 0;
    int memory_num = MIN_MALLOC_SIZE;
    regmatch_t *matches;
    regoff_t shift = 0;
    regmatch_t pm;
    int result = 0;

    matches = s21_calloc(sizeof(regmatch_t) * (size_t) (memory_num));

    while (1) {
        int force_quit = 0;
        pm.rm_so = shift;
        pm.rm_eo = (regoff_t) strlen(line.data);

        for (int i = 0; i < settings.num_patterns; i++) {
            if (settings.has_empty_pattern == 0) {
                    regex_t preg;
                    int err;

                    err = regcomp(&preg, settings.patterns[i], settings.regexp_cflags);

                    if (err != 0) {
                        char buff[512];
                        regerror(err, &preg, buff, sizeof(buff));
                        s21_grep_print_error(buff);
                    } else {
                        int regerr;
                        regerr = regexec(&preg, line.data, 1, &pm, REG_STARTEND);

                        if (regerr == 0) {
                            num_matches++;
                            if (num_matches >= memory_num) {
                                memory_num += MIN_MALLOC_SIZE;
                                matches = s21_realloc(matches, sizeof(regmatch_t) * (size_t) (memory_num));
                            }
                            matches[num_matches - 1] = pm;
                        }
                    }
                    shift = pm.rm_eo;
                    regfree(&preg);
            } else {
                num_matches++;
                matches[num_matches - 1] = pm;
            }

            if ((settings.show_matching_only == 0 && num_matches > 0) || settings.has_empty_pattern == 1) {
                force_quit = 1;
                break;
            }
        }

        if (shift == (regoff_t) strlen(line.data) || force_quit == 1) {
            break;
        }
    }

    result = s21_grep_print_line_apple(line, settings, matches, num_matches);

    free(matches);
    return result;
}

int s21_grep_print_line_apple(s21_string line,
 s21_grep_settings settings, regmatch_t *matches, int num_matches) {
    int result;
    setlocale(LC_ALL, "");

    result = (num_matches > 0 && settings.invert_match == 0)
     || (num_matches == 0 && settings.invert_match == 1);

    if (settings.show_count_lines_only == 0 && settings.show_files_name_only == 0) {
        if (result == 1) {
            if (settings.num_files > 1 && settings.show_no_files_name == 0) {
                printf("%s:", line.file_name);
            }

            if (settings.show_lines_num == 1) {
                printf("%lu:", line.line_num);
            }

            if (settings.show_matching_only == 1 && settings.invert_match == 0) {
                for (int i = 0; i < num_matches; i++) {
                    print_nchars(line.data + matches[i].rm_so, matches[i].rm_eo - matches[i].rm_so);
                    putchar('\n');
                }
            } else {
                printf("%s\n", line.data);
            }
        }
    }
    return result;
}

int s21_grep_line_linux(s21_string line, s21_grep_settings settings) {
    int force_quit = 0;
    int num_matches = 0;
    int memory_num = MIN_MALLOC_SIZE;
    regmatch_t *matches;
    regoff_t shift = 0;
    regmatch_t pm;
    int result = 0;
    char *ptr = line.data;

    matches = s21_calloc(sizeof(regmatch_t) * (size_t) (memory_num));

    pm.rm_so = 0;
    pm.rm_eo = (regoff_t) strlen(line.data);

    for (int i = settings.num_patterns - 1; i >= 0 ; i--) {
        if (settings.has_empty_pattern == 1 && settings.show_matching_only == 0) {
            num_matches++;
            matches[num_matches - 1] = pm;
        } else {
            if (strlen(settings.patterns[i]) == 0) {
                force_quit = 1;
            } else {
                regex_t preg;
                int err;

                err = regcomp(&preg, settings.patterns[i], settings.regexp_cflags);

                if (err != 0) {
                    char buff[512];
                    regerror(err, &preg, buff, sizeof(buff));
                    s21_grep_print_error(buff);
                } else {
                    int regerr;
                    regerr = regexec(&preg, ptr, 1, &pm, 0);

                    if (regerr == 0) {
                        while (regerr == 0) {
                            num_matches++;
                            if (num_matches >= memory_num) {
                                memory_num += MIN_MALLOC_SIZE;
                                matches = s21_realloc(matches, sizeof(regmatch_t) * (size_t) (memory_num));
                            }

                            pm.rm_so += shift;
                            pm.rm_eo += shift;

                            matches[num_matches - 1] = pm;
                            shift = pm.rm_eo;
                            ptr = line.data + shift;
                            if (strlen(line.data) == 0) {
                                break;
                            }
                            regerr = regexec(&preg, ptr, 1, &pm, 0);
                        }
                        force_quit = 1;
                    }
                }
                regfree(&preg);
            }
        }

        if (force_quit == 1) {
            break;
        }
    }

    result = s21_grep_print_line_linux(line, settings, matches, num_matches);

    free(matches);
    return result;
}

int s21_grep_print_line_linux(s21_string line,
 s21_grep_settings settings, regmatch_t *matches, int num_matches) {
    int result;
    setlocale(LC_ALL, "");

    result = (num_matches > 0 && settings.invert_match == 0)
        || (num_matches == 0 && settings.invert_match == 1);

    if (settings.show_count_lines_only == 0 && settings.show_files_name_only == 0) {
        if (result == 1 && !(settings.show_matching_only == 1 && settings.invert_match == 1)) {
            if (settings.show_matching_only == 1 && settings.invert_match == 0) {
                for (int i = 0; i < num_matches; i++) {
                    if (settings.num_files > 1 && settings.show_no_files_name == 0) {
                        printf("%s:", line.file_name);
                    }

                    if (settings.show_lines_num == 1) {
                        printf("%lu:", line.line_num);
                    }
                    print_nchars(line.data + matches[i].rm_so, matches[i].rm_eo - matches[i].rm_so);
                    if (!(settings.show_matching_only == 1 && (matches[i].rm_eo - matches[i].rm_so) == 0)) {
                        putchar('\n');
                    }
                }
            } else {
                if (settings.num_files > 1 && settings.show_no_files_name == 0) {
                    printf("%s:", line.file_name);
                }

                if (settings.show_lines_num == 1) {
                    printf("%lu:", line.line_num);
                }
                printf("%s\n", line.data);
            }
        }
    }

    return result;
}

void s21_grep_print_lc_data_apple(s21_grep_settings settings, char *filename, int num_matches) {
    if (settings.show_count_lines_only == 1) {
        if (settings.num_files > 1 && settings.show_no_files_name == 0) {
            printf("%s:", filename);
        }
        printf("%d\n", num_matches);
    }

    if (settings.show_files_name_only == 1 && num_matches > 0) {
        printf("%s\n", filename);
    }
}

void s21_grep_print_lc_data_linux(s21_grep_settings settings, char *filename, int num_matches) {
    if (num_matches > 0 || settings.show_count_lines_only == 1) {
        if ((settings.num_files > 1 || settings.show_files_name_only == 1)
         && (settings.show_no_files_name == 0
         || (settings.show_no_files_name == 1 && settings.show_files_name_only == 1))
         && !(num_matches == 0 && settings.num_files == 1 && settings.show_count_lines_only == 1
         && settings.show_files_name_only == 1)) {
            if (settings.show_count_lines_only == 1
             && !(settings.show_files_name_only == 1 && num_matches > 0)) {
                printf("%s:", filename);
            } else {
                printf("%s", filename);
            }
        }

        if (settings.show_count_lines_only == 1 && !(settings.show_files_name_only == 1 && num_matches > 0)) {
            printf("%d", num_matches);
        }
        putchar('\n');
    }
}
