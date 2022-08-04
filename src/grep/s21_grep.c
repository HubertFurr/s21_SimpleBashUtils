#include "./s21_grep.h"
#include "./../common/common.h"

int main(int argc, char *argv[]) {
    s21_grep_settings settings = s21_grep_get_settings(argc, argv);
    #if defined(__DEBUG)
    s21_grep_print_settings(settings);
    #endif
    #if defined(__APPLE__)
    if (settings.num_patterns > 0) {
    #endif
        for (int i = 0; i < settings.num_files; i++) {
            int num_matches = s21_grep_file(settings.files[i], settings);
            if (num_matches >= 0) {
                #if defined(__APPLE__)
                s21_grep_print_lc_data_apple(settings, settings.files[i], num_matches);
                #else
                if (settings.show_count_lines_only == 1 || settings.show_files_name_only == 1) {
                    s21_grep_print_lc_data_linux(settings, settings.files[i], num_matches);
                }
                #endif
            } else if (num_matches == -2) {
                break;
            }
        }
    #if defined(__APPLE__)
    }
    #endif

    s21_grep_destroy_settings(&settings);

    return 0;
}
