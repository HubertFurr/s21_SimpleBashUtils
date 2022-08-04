#include "./s21_cat.h"
#include "./../common/common.h"

int main(int argc, char *argv[]) {
    s21_cat_settings settings = s21_get_cat_settings(argc, argv);
    #if defined(__DEBUG)
    s21_print_settings(settings);
    #endif

    for (int i = 0; i < settings.num_files; i++) {
        s21_cat_file(settings.files[i], settings);
    }

    s21_destroy_settings(settings);
    return 0;
}
