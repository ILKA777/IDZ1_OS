#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <errno.h>

const int bufSize = 6500;
const char fifoName[] ="IDZ1_2.fifo";

void findIdentifier(const char *str, int *res) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalnum(str[i])) {
            continue;
        }
        int j = i;
        while (isalnum(str[j])) {
            j++;
        }
        if (j - i > 1 && isalpha(str[i])) {
            (*res)++;
        }
        i = j - 1;
    }
}

int main(int argc, char **argv) {
    int fileDescriptor;

    // Создаем именованный канал, если его еще не существует.
    if (mknod(fifoName, S_IFIFO | 0666, 0) < 0 && errno != EEXIST) {
        perror("mknod");
        exit(EXIT_FAILURE);
    }

    // Проверяем корректность количества аргументов командной строки.
    if (argc != 3) {
        printf("write name of input and output file");
        exit(EXIT_FAILURE);
    }

    // Создаем двух дочерних процессов
    int firstChild = fork();
    int secondChild = fork();

    // Получаем имена входного и выходного файлов из аргументов командной строки.
    char *input = argv[1];
    char *output = argv[2];

    // Проверяем, были ли дочерние процессы успешно созданы.
    if (firstChild == -1 || secondChild == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Родительский процесс.
    if (firstChild > 0 && secondChild > 0) {
        int status;
        // Ожидаем завершения выполнения всех дочерних процессов.
        if (wait(&status) == -1) {
            perror("wait");
            exit(EXIT_FAILURE);
        }
    } else if (firstChild == 0 && secondChild > 0) {// Первый дочерний процесс
        // Открываем входной файл (на чтение).
        if ((fileDescriptor = open(input, O_RDONLY)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        char buffer[bufSize];
        ssize_t bytesReader;

        // Считываем данные из входного файла.
        bytesReader = read(fileDescriptor, buffer, bufSize);

        if (bytesReader == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        // Записываем в IDZ1_2.fifo
        if ((fileDescriptor = open(fifoName, O_WRONLY)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        if (write(fileDescriptor, buffer, bytesReader) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        // Закрываем файл.
        if (close(fileDescriptor) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    } else if (firstChild > 0 && secondChild == 0) {// Второй дочерний процесс
        // Открываем именованный канал на чтение.
        if ((fileDescriptor = open(fifoName, O_RDONLY)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        char buffer[bufSize];
        ssize_t bytesReader;

        // Считываем данные из именованного канала.
        bytesReader = read(fileDescriptor, buffer, bufSize);

        if (bytesReader == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        // Закрываем именованный канал.
        if (close(fileDescriptor) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }

        // Считаем количество идентификаторов в полученной строке.
        int identifiers = 0;
        findIdentifier(buffer, &identifiers);

        // Открываем выходной файл (на запись).
        if ((fileDescriptor = open(output, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // Записываем результат в выходной файл.
        char result[128];
        snprintf(result, sizeof(result), "Number of identifiers: %d\n", identifiers);
        if (write(fileDescriptor, result, strlen(result)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        // Закрываем файл.
        if (close(fileDescriptor) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

