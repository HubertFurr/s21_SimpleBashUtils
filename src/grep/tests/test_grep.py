from termios import tcgetattr, tcsetattr, TCSADRAIN
from itertools import combinations_with_replacement
from itertools import combinations
from random import randrange, choice, shuffle
from tty import setcbreak
from select import select
from time import sleep
from sys import stdin
from os import system
from os import listdir
from os import path
from os import stat
import re
import sys
from datetime import datetime
import time

start_time = datetime.now()

# 1 или 0 останавливать тесты после ошибки или нет
stop = 0

# 1 или 0 показывать всегда команды
show_comms = 0

# 1 или 0 показывать вывод ошибки
more = 1

# 1 или 0 если показывать в конце список комманд
show_log = 1

# 1 или 0 если тестировать функциональность
test_func = 1

# 1 или 0 если тестировать fsanitize
test_fsan = 1

# 1 или 0 если тестировать valgrind
test_valgrind = 0

# 1 или 0 если тестировать leaks
test_leaks = 1

# 1 или 0 если генерировать полурандомные тесты
random = 0

# 0 - Запускать все тесты, <число> - запсуать 1й тест и далее каждый указанный тест из файла
run_test_num = 0

# указать фрагмент, который должны содержать имена запускаемых файлов с тестами (например, "part2", "part3", "part4")
# или оставить пустой для запуска всех тестов
test_part = ""

if len(sys.argv) > 1:
    if sys.argv[1] == 'test':
        test_func = 1
        test_fsan = 0
        test_valgrind = 0
        test_leaks = 0

        if len(sys.argv) > 2:
            if sys.argv[2] == 'part1':
                test_part = 'part1'
            elif sys.argv[2] == 'part2':
                test_part = 'part2'
            elif sys.argv[2] == 'part3':
                test_part = 'part3'
            elif sys.argv[2] == 'part4':
                test_part = 'part4'
            else:
                test_part = 'part2'

    if sys.argv[1] == 'mac':
        test_func = 1
        test_fsan = 1
        test_valgrind = 0
        test_leaks = 0

    if sys.argv[1] == 'linux':
        test_func = 1
        test_fsan = 0
        test_valgrind = 1
        test_leaks = 0

    if sys.argv[1] == 'hard':
        test_func = 1
        test_fsan = 0
        test_valgrind = 0
        test_leaks = 0
        random = 1
        show_comms = 1


# любый символы остановки вывода
quit_command = ['q', 'z']

# Указать пути до грепов
s21_comm = "./s21_grep"
s21_fcomm = "./s21_grepf"
s21_vcomm = "./s21_grepv"
comm = "grep"

# Дополнительные флаги для оригинального grep (например, '--colour=never')
grep_add_flag = '--colour=never'

valgrind_log = "valgrind.res"
valgrind = f"valgrind --log-file={valgrind_log} --trace-children=yes --track-fds=yes --track-origins=yes --leak-check=full --show-leak-kinds=all -s"
leaks = "leaks --atExit --"

# Файл со списком тестов
dir_with_test_data = './tests/data/'
list_test_file = './tests/tests.data'

# Путь где будут распологаться результаты
tmp_file = '0_{}.res'

#
#       Настройки генерируемых тестов
#

# Используемые флаги
# <флаг>, <0 - только флаг, 1 - флаг + patterns, 2 - флаг + files_with_patterns>
flags = [
    ('-e', 1),
    ('-i', 0),
    ('-v', 0),
    ('-c', 0),
    ('-l', 0),
    ('-n', 0),
    ('-h', 0),
    ('-s', 0),
    ('-f', 2),
    ('-o', 0),
]

files = [
    'tests/files/1.file',
    'tests/files/2.file',
    'tests/files/3.file',
    'tests/files/empty.file',
    'tests/files/man_grep.file',
    'tests/files/v255_2.file',
]

files_with_patterns = [
   'tests/patterns/1.ptrn',
   'tests/patterns/2.ptrn',
   'tests/patterns/3.ptrn',
   'tests/patterns/4.ptrn',
]

patterns = [
    'DESCRIPTION',
    'With',
    'with',
    'WITH',
    '20',
    '. ',
    '\. ',
    '-[a-z][a-z]',
    '-[a-z][A-Z]',
    '-[a-z]\{2\}',
    '-[a-z]{2}',
    '-[A-Z]',
    '[0-9]\{2\}',
    '\-[a-z]\{1\}[^a-z]',
    'a|b',
    '[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}',
    'output\.$',
]

# минимальное число флагов в генерируемых тестах
hard_flags_start = 0
# максимальное число флагов
hard_flags_end = len(flags) + 1
# сколько раз генерировать случайный набор файлов для одного набора тестов
nums_of_test_cases_for_files = 2

#
#        START PROGRAM
#

func_error = []
fsan_error = []
valgrind_error = []
leaks_error = []

TEST_COUNT = 0
TEST_COUNT_FAILED = 0
TEST_COUNT_FAILED_FUNC = 0
TEST_COUNT_FAILED_FSAN = 0
TEST_COUNT_FAILED_VALGRIND = 0
TEST_COUNT_FAILED_LEAKS = 0

s21_comm_file = tmp_file.format(s21_comm.split('/')[-1])
s21_comm_error_file = tmp_file.format(s21_comm.split('/')[-1] + '_err')
comm_file = tmp_file.format(comm)
comm_error_file = tmp_file.format(comm + '_err')
diff_file = tmp_file.format('diff')

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

FAILED = "\tFAILED"
SUCCESS = "\tSUCCESS"
SKIPPED = "\tSKIPPED"

def get_argv(flag: str, type: int) -> str:
    if type == 2:
        flag += f'{(" " if 1 else "")}{choice(files_with_patterns)}'
    if type == 1:
        flag += f'{(" " if 1 else "")}"{choice(patterns)}"'

    return flag

def is_data():
    return select([stdin], [], [], 0) == ([stdin], [], [])


def run_test_func(command_1: str, command_2: str) -> None:
    global TEST_COUNT_FAILED_FUNC, TEST_COUNT

    system(command_1)
    system(command_2)
    diff_command = f'diff {s21_comm_file} {comm_file} > {diff_file}'

    if test_func:
        if system(diff_command):
            if show_log:
                func_error.append(('TEST', TEST_COUNT, ':\t', command_1, '\n\t', command_2))

            print(f'{bcolors.BOLD}{bcolors.OKCYAN}{"FUNCTIONAL"}{bcolors.ENDC}{": "}{bcolors.ENDC}{bcolors.FAIL}{FAILED}{bcolors.ENDC}')

            if more:
                print()
                print(f'{bcolors.FAIL}{bcolors.BOLD}{"COMMANDS:"}{bcolors.ENDC}{bcolors.ENDC}')
                print(command_1)
                print(command_2)
                print(diff_command)
                print()
            TEST_COUNT_FAILED_FUNC += 1
            result = 0
            if stop:
                input()        
        else:
            s21_comm_file_stats = stat(s21_comm_error_file)
            comm_file_stats = stat(comm_error_file)
            if((s21_comm_file_stats.st_size == 0 and comm_file_stats.st_size > 0) or (s21_comm_file_stats.st_size > 0 and comm_file_stats.st_size == 0)):
                if show_log:
                    func_error.append(('TEST', TEST_COUNT, ':\t', command_1, '\n\t', command_2, '\n!!!STDERROR DIFF!!!!'))
                print(f'{bcolors.BOLD}{bcolors.OKCYAN}{"FUNCTIONAL"}{bcolors.ENDC}{": "}{bcolors.ENDC}{bcolors.FAIL}{FAILED}{bcolors.ENDC}')
                if more:
                    print()
                    print(f'{bcolors.FAIL}{bcolors.BOLD}{"COMMANDS:"}{bcolors.ENDC}{bcolors.ENDC}')
                    print(command_1)
                    print(command_2)
                    print(f'{bcolors.FAIL}{bcolors.BOLD}!!!STDERROR DIFF!!!!{bcolors.ENDC}{bcolors.ENDC}')
                    print()
                TEST_COUNT_FAILED_FUNC += 1
                result = 0
                if stop:
                    input()  
            else:
                result = 1
                print(f'{bcolors.BOLD}{bcolors.OKCYAN}FUNCTIONAL{bcolors.ENDC}: {bcolors.OKGREEN}{SUCCESS}{bcolors.ENDC}{bcolors.ENDC}')
                if show_comms:
                    print()
                    print(f'{bcolors.FAIL}{bcolors.BOLD}{"COMMANDS:"}{bcolors.ENDC}{bcolors.ENDC}')
                    print(command_1)
                    print(command_2)
                    print(diff_command)
                    print()
        
        system(f"rm -f {comm_file}")
        system(f"rm -f {comm_error_file}")
        system(f"rm -f {s21_comm_file}")
        system(f"rm -f {s21_comm_error_file}")
        system(f"rm -f {diff_file}")
    else:
        result = 1
        print(f'{bcolors.BOLD}{bcolors.OKCYAN}{"FUNCTIONAL"}{bcolors.ENDC}{": "}{bcolors.ENDC}{bcolors.WARNING}{SKIPPED}{bcolors.ENDC}')

    return result


def run_test_fsan(command_1: str) -> None:
    global TEST_COUNT_FAILED_FSAN, TEST_COUNT
    if test_fsan:
        if system(command_1):
            if show_log:
                fsan_error.append(('TEST', TEST_COUNT, ':\t', command_1))

            print(f'{bcolors.BOLD}{bcolors.OKCYAN}{"FSANITIZE"}{bcolors.ENDC}{": "}{bcolors.ENDC}{bcolors.FAIL}{FAILED}{bcolors.ENDC}')
            if more:
                print()
                print(f'{bcolors.FAIL}{bcolors.BOLD}{"COMMANDS:"}{bcolors.ENDC}{bcolors.ENDC}')
                print(command_1)
                print()
            TEST_COUNT_FAILED_FSAN += 1
            result = 0
            if stop:
                input()
        else:
            print(f'{bcolors.BOLD}{bcolors.OKCYAN}FSANITIZE{bcolors.ENDC}: {bcolors.OKGREEN}{SUCCESS}{bcolors.ENDC}{bcolors.ENDC}')
            result = 1
            if show_comms:
                print()
                print(f'{bcolors.FAIL}{bcolors.BOLD}{"COMMANDS:"}{bcolors.ENDC}{bcolors.ENDC}')
                print(command_1)
                print()

        system(f"rm -f {s21_comm_file}")
    else:
        result = 1
        print(f'{bcolors.BOLD}{bcolors.OKCYAN}{"FSANITIZE"}{bcolors.ENDC}{": "}{bcolors.ENDC}{bcolors.WARNING}{SKIPPED}{bcolors.ENDC}')

    return result


def run_test_valgrind(command_1: str) -> None:
    global TEST_COUNT_FAILED_VALGRIND, TEST_COUNT
    result = 1
    if test_valgrind:
        system(command_1)
        file = open(valgrind_log, "r")
        for line in file:
            if (re.search('ERROR SUMMARY', line)):
                if (re.search('ERROR SUMMARY: 0 errors', line)):
                    print(f'{bcolors.OKGREEN}{line}{bcolors.ENDC}')                
                else:
                    print(f'{bcolors.FAIL}{line}{bcolors.ENDC}')                
                    if show_log:
                        valgrind_error.append(('TEST', TEST_COUNT, ':\t', command_1))
                    if more:
                        print()
                        print(f'{bcolors.FAIL}{bcolors.BOLD}{"COMMANDS:"}{bcolors.ENDC}{bcolors.ENDC}')
                        print(command_1)
                        print()
                    result = 0
        if (result == 0):
            print(f'{bcolors.BOLD}{bcolors.OKCYAN}{"VALGRIND"}{bcolors.ENDC}{": "}{bcolors.ENDC}{bcolors.FAIL}{FAILED}{bcolors.ENDC}')
            TEST_COUNT_FAILED_VALGRIND += 1
            if stop:
                input()
                
        if result:
            print(f'{bcolors.BOLD}{bcolors.OKCYAN}VALGRIND{bcolors.ENDC}: {bcolors.OKGREEN}{SUCCESS}{bcolors.ENDC}{bcolors.ENDC}')
            if show_comms:
                print()
                print(f'{bcolors.FAIL}{bcolors.BOLD}{"COMMANDS:"}{bcolors.ENDC}{bcolors.ENDC}')
                print(command_1)
                print()

        system(f"rm -f {s21_comm_file}")
        system(f"rm -f {valgrind_log}")
    else:
        print(f'{bcolors.BOLD}{bcolors.OKCYAN}{"VALGRIND"}{bcolors.ENDC}{": "}{bcolors.ENDC}{bcolors.WARNING}{SKIPPED}{bcolors.ENDC}')

    return result


def run_test_leaks(command_1: str) -> None:
    global TEST_COUNT_FAILED_LEAKS, TEST_COUNT
    if test_leaks:
        if system(command_1):
            print(f'{bcolors.FAIL}')
            system(f'grep --color=never "leaks for" {s21_comm_file}')
            print(f'{bcolors.ENDC}')
            print(f'{bcolors.BOLD}{bcolors.OKCYAN}{"LEAKS"}{bcolors.ENDC}{": "}{bcolors.ENDC}{bcolors.FAIL}\t{FAILED}{bcolors.ENDC}')
            if show_log:
                leaks_error.append(('TEST', TEST_COUNT, ':\t', command_1))
            if more:
                print()
                print(f'{bcolors.FAIL}{bcolors.BOLD}{"COMMANDS:"}{bcolors.ENDC}{bcolors.ENDC}')
                print(command_1)
                print()
            TEST_COUNT_FAILED_LEAKS += 1
            result = 0
            if stop:
                input()
        else:
            print(f'{bcolors.BOLD}{bcolors.OKCYAN}LEAKS{bcolors.ENDC}: {bcolors.OKGREEN}\t{SUCCESS}{bcolors.ENDC}{bcolors.ENDC}')
            result = 1
            if show_comms:
                print()
                print(f'{bcolors.FAIL}{bcolors.BOLD}{"COMMANDS:"}{bcolors.ENDC}{bcolors.ENDC}')
                print(command_1)
                print()
        sleep(0.4)
        system(f"rm -f {s21_comm_file}")
    else:
        result = 1
        print(f'{bcolors.BOLD}{bcolors.OKCYAN}{"LEAKS"}{bcolors.ENDC}{": "}{bcolors.ENDC}{bcolors.WARNING}\t{SKIPPED}{bcolors.ENDC}')

    return result

def simple_test():
    global TEST_COUNT, TEST_COUNT_FAILED

    for filename in listdir(dir_with_test_data):
        if(filename[0] != '_' and filename[0] != '.'):
            if (re.search(test_part, filename)):
                fullpath = path.join(dir_with_test_data, filename)

                with open(fullpath, 'r') as f:
                    numtest = 0
                    while(True):
                        argv = f.readline()
                        if(not argv): break
                        if(argv[0] == '#' or argv[0] == '/'): continue
                        argv = argv.replace('\n', '')
                        if(len(argv) == 0): break

                        TEST_COUNT += 1
                        numtest += 1

                        if (run_test_num > 0):
                            if (numtest != 1 and numtest%run_test_num != 0):
                                continue

                        print(f'{bcolors.HEADER}======================================================================================={bcolors.ENDC}')
                        print(f'{bcolors.BOLD}{bcolors.OKCYAN}{"TEST "}{TEST_COUNT}{bcolors.ENDC}')
                        print(f'{bcolors.HEADER}======================================================================================={bcolors.ENDC}')
                        argv2 = argv + ' ' + grep_add_flag
                        argv2 = argv2.strip()

                        res_func = run_test_func(f'{s21_comm} {argv} > {s21_comm_file} 2> {s21_comm_error_file}', f'{comm} {argv2} > {comm_file} 2> {comm_error_file}')
                        res_fsan = run_test_fsan(f'{s21_fcomm} {argv} > {s21_comm_file}')
                        res_valgrind = run_test_valgrind(f'{valgrind} {s21_vcomm} {argv} > {s21_comm_file}')
                        res_leaks = run_test_leaks(f'{leaks} {s21_vcomm} {argv} > {s21_comm_file}')

                        if(res_func == 0 or res_fsan == 0 or res_valgrind == 0 or res_leaks == 0): TEST_COUNT_FAILED += 1
                        
                        if is_data():
                            c = stdin.read(1)

                            if c in quit_command:
                                return

def hard_test():
    global TEST_COUNT, TEST_COUNT_FAILED
    for i in range(hard_flags_start, hard_flags_end):
        for list_arg_m in combinations_with_replacement(flags, i):
            for list_arg in (set(list_arg_m), list_arg_m):
                list_arg = list(list_arg)

                shuffle(list_arg)

                for j in range(nums_of_test_cases_for_files):
                    _flags = []
                    _files = []

                    was_pattern = 0
                    argv = []
                    for arg_flag, arg_type in list_arg:
                        was_pattern += arg_type
                        _flags.append(get_argv(arg_flag, arg_type))
                    argv += _flags

                    for _ in range(randrange(1, len(files))):
                        _files.append(choice(files))
                    argv += _files

                    shuffle(argv)
                    if not was_pattern:
                        argv.insert(0, '"'+choice(patterns)+'"')
                    argv = ' '.join(argv)

                    TEST_COUNT += 1

                    print(f'{bcolors.HEADER}======================================================================================={bcolors.ENDC}')
                    print(f'{bcolors.BOLD}{bcolors.OKCYAN}{"TEST "}{TEST_COUNT}{bcolors.ENDC}')
                    print(f'{bcolors.HEADER}======================================================================================={bcolors.ENDC}')
                    argv2 = argv + ' ' + grep_add_flag
                    argv2 = argv2.strip()

                    res_func = run_test_func(f'{s21_comm} {argv} > {s21_comm_file} 2> {s21_comm_error_file}', f'{comm} {argv2} > {comm_file} 2> {comm_error_file}')
                    res_fsan = run_test_fsan(f'{s21_fcomm} {argv} > {s21_comm_file}')
                    res_valgrind = run_test_valgrind(f'{valgrind} {s21_vcomm} {argv} > {s21_comm_file}')
                    res_leaks = run_test_leaks(f'{leaks} {s21_vcomm} {argv} > {s21_comm_file}')

                    if(res_func == 0 or res_fsan == 0 or res_valgrind == 0 or res_leaks == 0): TEST_COUNT_FAILED += 1

                    if is_data():
                        c = stdin.read(1)

                        if c in quit_command:
                            return

if __name__ == '__main__':
    old_settings = tcgetattr(stdin)

    try:
        setcbreak(stdin.fileno())

        print("START TEST:\n")
        if(random == 0):
            simple_test()
        else: 
            hard_test()

        print()
        print(f'{bcolors.HEADER}======================================================================================={bcolors.ENDC}')
        print(f'{bcolors.BOLD}{bcolors.OKCYAN}TOTAL TEST: \t{bcolors.ENDC}{TEST_COUNT}')
        print(f'{bcolors.BOLD}{bcolors.OKCYAN}TOTAL SUCCESS: \t{bcolors.ENDC}{bcolors.OKGREEN}{TEST_COUNT - TEST_COUNT_FAILED}{bcolors.ENDC}')
        print(f'{bcolors.BOLD}{bcolors.OKCYAN}TOTAL FAILED: \t{bcolors.ENDC}{bcolors.FAIL}{TEST_COUNT_FAILED}{bcolors.ENDC}')

        if TEST_COUNT_FAILED:
            persent = ((TEST_COUNT - TEST_COUNT_FAILED) / TEST_COUNT) * 100
        else:
            persent = 100

        if persent > 80:
            print(f'{bcolors.BOLD}{bcolors.WARNING}PERCENT: \t{bcolors.ENDC}{bcolors.OKGREEN}{persent}%{bcolors.ENDC}{bcolors.ENDC}{bcolors.ENDC}')
        elif persent > 50:
            print(f'{bcolors.BOLD}{bcolors.WARNING}PERCENT: \t{bcolors.ENDC}{bcolors.WARNING}{persent}%{bcolors.ENDC}{bcolors.ENDC}{bcolors.ENDC}')
        else:
            print(f'{bcolors.BOLD}{bcolors.WARNING}PERCENT: \t{bcolors.ENDC}{bcolors.FAIL}{persent}%{bcolors.ENDC}{bcolors.ENDC}{bcolors.ENDC}')

        print()

        if test_func:
            FUNC_COUNT = TEST_COUNT
            if (TEST_COUNT_FAILED_FUNC == 0): COLOR_FUNC = bcolors.OKGREEN
            else: COLOR_FUNC = bcolors.FAIL
        else:
            FUNC_COUNT = 0
            COLOR_FUNC = bcolors.OKGREEN

        if test_fsan:
            FSAN_COUNT = TEST_COUNT
            if (TEST_COUNT_FAILED_FSAN == 0): COLOR_FSCAN = bcolors.OKGREEN
            else: COLOR_FSCAN = bcolors.FAIL
        else:
            FSAN_COUNT = 0
            COLOR_FSCAN = bcolors.OKGREEN

        if test_valgrind:
            VALGRIND_COUNT = TEST_COUNT
            if (TEST_COUNT_FAILED_VALGRIND == 0): COLOR_VALGRIND = bcolors.OKGREEN
            else: COLOR_VALGRIND = bcolors.FAIL
        else:
            VALGRIND_COUNT = 0
            COLOR_VALGRIND = bcolors.OKGREEN

        if test_leaks:
            LEAKS_COUNT = TEST_COUNT
            if (TEST_COUNT_FAILED_LEAKS == 0): COLOR_LEAKS = bcolors.OKGREEN
            else: COLOR_LEAKS = bcolors.FAIL
        else:
            LEAKS_COUNT = 0
            COLOR_LEAKS = bcolors.OKGREEN

        print(f'{bcolors.BOLD}FUNC TEST: \t{bcolors.ENDC}{COLOR_FUNC}{FUNC_COUNT - TEST_COUNT_FAILED_FUNC}{bcolors.ENDC}/{bcolors.OKGREEN}{FUNC_COUNT}{bcolors.ENDC}')
        print(f'{bcolors.BOLD}FSAN TEST: \t{bcolors.ENDC}{COLOR_FSCAN}{FSAN_COUNT - TEST_COUNT_FAILED_FSAN}{bcolors.ENDC}/{bcolors.OKGREEN}{FSAN_COUNT}{bcolors.ENDC}')
        print(f'{bcolors.BOLD}VALGRIND TEST: \t{bcolors.ENDC}{COLOR_VALGRIND}{VALGRIND_COUNT - TEST_COUNT_FAILED_VALGRIND}{bcolors.ENDC}/{bcolors.OKGREEN}{VALGRIND_COUNT}{bcolors.ENDC}')
        print(f'{bcolors.BOLD}LEAKS TEST: \t{bcolors.ENDC}{COLOR_LEAKS}{LEAKS_COUNT - TEST_COUNT_FAILED_LEAKS}{bcolors.ENDC}/{bcolors.OKGREEN}{LEAKS_COUNT}{bcolors.ENDC}')
        print(f'{bcolors.HEADER}---------------------------------------------------------------------------------------{bcolors.ENDC}')

        if show_log:
            print("\n\n\tALL ERRORS:\n\n")

            if test_func:
                print("FUNC ERRORS:")
                for i in range(len(func_error)):
                    print(*func_error[i], sep='')
                    print()
                print()

            if test_fsan:
                print("FSAN ERRORS:")
                for i in range(len(fsan_error)):
                    print(*fsan_error[i], sep='')
                    print()
                print()

            if test_valgrind:
                print("VALGRIND ERRORS:")
                for i in range(len(valgrind_error)):
                    print(*valgrind_error[i], sep='')
                    print()
                print()

            if test_leaks:
                print("LEAKS ERRORS:")
                for i in range(len(leaks_error)):
                    print(*leaks_error[i], sep='')
                    print()

    finally:
        tcsetattr(stdin, TCSADRAIN, old_settings)
        print(f'Execution time: {datetime.now() - start_time}')