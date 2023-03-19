#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

const int bufSize = 6500;

void findIdentifier(const char *str, int *res) {

    for (int i = 0; str[i] != '\0'; i++) {
        // Eсли текущий символ не является буквой или цифрой.
        if (!isalnum(str[i])) {
            // Переходим к следующему символу.
            continue;
        }
        int j = i;
        // Пока текущий символ является буквой или цифрой.
        while (isalnum(str[j])) {
            j++;
        }
        // Eсли длина найденной подстроки больше 1 и начинается с буквы.
        if (j - i > 1 && isalpha(str[i])) {
            // Увеличиваем счетчик найденных подстрок.
            (*res)++;
        }
        i = j - 1;
    }
}

int main(int argc, char **argv) {
    // Первый канал
    int firstToSecond[2];
    // Второй канал
    int secondToFirst[2];

    if (pipe(firstToSecond) < 0) {
        printf("Can\'t open pipe\n");
        exit(EXIT_FAILURE);
    }

    if (pipe(secondToFirst) < 0) {
        printf("Can\'t open pipe\n");
        exit(EXIT_FAILURE);
    }

    // Если не указаны оба аргумента
    if (argc != 3) {
        printf("write name of input and output file");
        exit(EXIT_FAILURE);
    }

    // Получаем имя входного файла
    char *input = argv[1];
    // Получаем имя выходного файла
    char *output = argv[2];

    int firstChild = fork();

    // Если не удалось создать первый дочерний процесс
    if (firstChild == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (firstChild == 0) { // Первый дочерний процесс
        // Дескриптор файла
        int fileDescriptor;
        // Буфер для хранения данных, считанных из файла
        char buffer[bufSize];
        // Количество считанных байт
        ssize_t bytesReader;

        // Если не удалось открыть входной файл
        if ((fileDescriptor = open(input, O_RDONLY)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // Читаем данные из входного файла в буфер
        bytesReader = read(fileDescriptor, buffer, bufSize);

        // Если не удалось считать данные из входного файла
        if (bytesReader == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        if (close(firstToSecond[0]) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }

        // Записываем данные из буфера в первый канал
        write(firstToSecond[1], buffer, bytesReader);

        // Закрываем дескриптор записи в первый канал
        if (close(firstToSecond[1]) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }

        // Закрываем дескриптор записи во второй канал
        if (close(secondToFirst[1]) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }

        char bufResult[bufSize];
        bytesReader = read(secondToFirst[0], bufResult, bufSize);

        if ((fileDescriptor = open(output, O_WRONLY | O_CREAT, 0666)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        write(fileDescriptor, bufResult, bytesReader);
    } else { // Родительский процесс
        // Создаем второй дочерний процесс
        int secondChild = fork();
        if (secondChild == 0) { // Второй дочерний процесс
            char bufResult[bufSize];

            if (close(firstToSecond[1]) == -1) {
                perror("close");
                exit(EXIT_FAILURE);
            }
            size_t reader = read(firstToSecond[0], bufResult, bufSize);

            if (close(secondToFirst[0]) == -1) {
                perror("close");
                exit(EXIT_FAILURE);
            }
            int res = 0;
            // Вызов функции для поиска и подсчета
            findIdentifier(bufResult, &res);
            // Буфер для преобразования числа res в строку
            char bufferTwo[32];
            // Преобразование числа res в строку и запись результата в буфер bufferTwo
            snprintf(bufferTwo, 32, "%d", res);

            write(secondToFirst[1], bufferTwo, strlen(bufferTwo));

            if (close(secondToFirst[1]) == -1) {
                perror("close");
                exit(EXIT_FAILURE);
            }

            if (close(firstToSecond[0]) == -1) {
                perror("close");
                exit(EXIT_FAILURE);
            }
        } else {
            int flag;
            wait(&flag);
        }
    }
    return 0;
}
