GREEN   =   \033[0;32m
RED		=   \033[0;31m
RESET	=   \033[0m

CC=gcc

F_LFLAGS=-g -fsanitize=leak -fsanitize=address -fsanitize=undefined -fsanitize=unreachable -fno-sanitize-recover -fstack-protector -fanalyzer
F_CFLAGS=-Wall -Werror -Wextra -c -g -fsanitize=leak -fsanitize=address -fsanitize=undefined -fsanitize=unreachable -fno-sanitize-recover -fstack-protector -fanalyzer
V_LFLAGS=-g
V_CFLAGS=-Wall -Werror -Wextra -c -g

# Дефолтное значение 1 если не задано с консоли
# Задаем так: make test MODE=2
MODE?=1

ifeq ($(MODE), 4)
	# Собираем со всеми флагами проверок и пишем идеальный код
	ZSH_MESSAGE=BUILD PERFECT CODE!
	LFLAGS=-g -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector -fanalyzer
	CFLAGS=-Wall -Werror -Wextra -c -g -O2 -pedantic -std=c11 -Wshadow -Wlogical-op -Wshift-overflow=2 -Wduplicated-cond -Wcast-align -Wfloat-equal -Wconversion -Wcast-qual -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2 -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector -fanalyzer
else
	# Стандартный режим
	ZSH_MESSAGE=BUILD BASIC!
	LFLAGS= 
	CFLAGS=-Wall -Werror -Wextra -c -std=c11 -pedantic
endif


ODIR = obj/
MAIN_C = s21_grep.c
UTILS_C = s21_grep_utils.c
MAIN_H = s21_grep.h
COMMON_C = ./../common/common.c
COMMON_H = ./../common/common.h
MAIN_O = $(MAIN_C:%.c=$(ODIR)%.o)
F_MAIN_O = $(MAIN_C:%.c=$(ODIR)%_f.o)
V_MAIN_O = $(MAIN_C:%.c=$(ODIR)%_v.o)
UTILS_O = $(UTILS_C:%.c=$(ODIR)%.o)
F_UTILS_O = $(UTILS_C:%.c=$(ODIR)%_f.o)
V_UTILS_O = $(UTILS_C:%.c=$(ODIR)%_v.o)
COMMON_O = $(COMMON_C:./../common/%.c=$(ODIR)%.o)
F_COMMON_O = $(COMMON_O:%.o=%_f.o)
V_COMMON_O = $(COMMON_O:%.o=%_v.o)

all: s21_grep
s21_grep: $(ODIR) $(MAIN_O) $(UTILS_O) $(COMMON_O)
	$(CC) $(LFLAGS) $(MAIN_O) $(UTILS_O) $(COMMON_O) -o ./s21_grep
s21_grepf: $(ODIR) $(F_MAIN_O) $(F_UTILS_O) $(F_COMMON_O)
	$(CC) $(F_LFLAGS) $(F_MAIN_O) $(F_UTILS_O) $(F_COMMON_O) -o ./s21_grepf
s21_grepv: $(ODIR) $(V_MAIN_O) $(V_UTILS_O) $(V_COMMON_O)
	$(CC) $(V_LFLAGS) $(V_MAIN_O) $(V_UTILS_O) $(V_COMMON_O) -o ./s21_grepv
$(ODIR):
	@mkdir -p $(ODIR)
	@echo "folder: $(GREEN)$(ODIR) was created$(RESET)"

$(MAIN_O): $(MAIN_C) $(MAIN_H)
	$(CC) $(CFLAGS) -o $@ $<
$(F_MAIN_O): $(MAIN_C) $(MAIN_H)
	$(CC) $(F_CFLAGS) -o $@ $<
$(V_MAIN_O): $(MAIN_C) $(MAIN_H)
	$(CC) $(V_CFLAGS) -o $@ $<
$(UTILS_O): $(UTILS_C) $(MAIN_H)
	$(CC) $(CFLAGS) -o $@ $<
$(F_UTILS_O): $(UTILS_C) $(MAIN_H)
	$(CC) $(F_CFLAGS) -o $@ $<
$(V_UTILS_O): $(UTILS_C) $(MAIN_H)
	$(CC) $(V_CFLAGS) -o $@ $<
$(COMMON_O): $(COMMON_C) $(COMMON_H)
	$(CC) $(CFLAGS) -o $@ $<
$(F_COMMON_O): $(COMMON_C) $(COMMON_H)
	$(CC) $(F_CFLAGS) -o $@ $<
$(V_COMMON_O): $(COMMON_C) $(COMMON_H)
	$(CC) $(V_CFLAGS) -o $@ $<

test: s21_grep
	python3 ./tests/test_grep.py test
test_mac: s21_grep s21_grepf s21_grepv
	python3 ./tests/test_grep.py mac
test_linux: clean s21_grep s21_grepv
	python3 ./tests/test_grep.py linux
test_hard: s21_grep
	python3 ./tests/test_grep.py hard
test_part1: s21_grep
	python3 ./tests/test_grep.py test part1
test_part2: s21_grep
	python3 ./tests/test_grep.py test part2
test_part3: s21_grep
	python3 ./tests/test_grep.py test part3
test_part4: s21_grep
	python3 ./tests/test_grep.py test part4

re: clean all
clean:
	rm -rf $(ODIR)*.o
	rm -rf *.res
	rm -rf ./s21_grep ./s21_grepf ./s21_grepv

cpplint:
	python3 ../../materials/linters/cpplint.py --extensions=c *.c *.h ./../common/*.*
cppcheck:
	cppcheck --enable=all --suppress=missingIncludeSystem *.c *.h ./../common/*.*