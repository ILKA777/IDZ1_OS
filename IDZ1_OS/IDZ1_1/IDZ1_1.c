#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <string.h>
#include <ctype.h>

const int bufSize = 6500;

void findIdentifier(const char *str, int *res) {

    for (int i = 0; str[i] != '\0'; i++) {
        // Eсли текущий символ не является буквой или цифрой
        if (!isalnum(str[i])) {
            // Переходим к следующему символу
            continue;
        }
        int j = i;
        // Пока текущий символ является буквой или цифрой
        while (isalnum(str[j])) {
            j++;
        }
        // Eсли длина найденной подстроки больше 1 и начинается с буквы
        if (j - i > 1 && isalpha(str[i])) {
            // Увеличиваем счетчик найденных подстрок
            (*res)++;
        }
        i = j - 1;
    }
}

int main(int argc, char **argv) {
    int fileDescriptor[2];

    // Создаем pipe для обмена данными между процессами
    if (pipe(fileDescriptor) < 0) {
        printf("Can\'t open pipe\n");
        exit(EXIT_FAILURE);
    }

    // Проверяем количество аргументов
    if (argc != 3) {
        printf("write name of input and output file");
        exit(EXIT_FAILURE);
    }

    // Создаем первый дочерний процесс
    int firstChild = fork();
    // Создаем второй дочерний процесс
    int secondChild = fork();

    char *input = argv[1];
    char *output = argv[2];

    // Проверяем успешность создания процессов
    if (firstChild == -1 || secondChild == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (firstChild > 0 && secondChild > 0) { // Родительский процесс
        int flag;
        wait(&flag);// ожидаем завершения выполнения всех дочерних процессов
    } else if (firstChild == 0 && secondChild > 0) { // Первый дочерний процесс
        int fileDescriptorTwo;
        char buffer[bufSize];
        ssize_t bytesReader;

        // открываем входной файл (на чтение)
        if ((fileDescriptorTwo = open(input, O_RDONLY)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // Считываем данные из входного файла.
        bytesReader = read(fileDescriptorTwo, buffer, bufSize);

        if (bytesReader == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        // Записывает содержимое buffer, размером bytesReader, в канал.
        write(fileDescriptor[1], buffer, bytesReader);
    } else if (firstChild > 0 && secondChild == 0) { // Второй дочерний процесс
        int flag;
        // Ожидание завершения первого дочернего процесса;
        waitpid(firstChild, &flag, 0);

        // Буфер для считывания данных из канала;
        char inputBuf[bufSize];
        // Чтение данных из канала в буфер inputBuf;
        size_t bytesRead = read(fileDescriptor[0], inputBuf, bufSize);
        // Счетчик найденных подстрок;
        int res = 0;
        // Вызов функции для поиска и подсчета
        findIdentifier(inputBuf, &res);
        // Буфер для преобразования числа res в строку
        char bufferTwo[32];
        // Преобразование числа res в строку и запись результата в буфер bufferTwo
        snprintf(bufferTwo, 32, "%d", res);
        // Запись строки в канал для передачи в третий дочерний процесс.
        write(fileDescriptor[1], bufferTwo, strlen(bufferTwo));

    } else { //thirdChild
        int flag;
        // Ожидание завершения второго дочернего процесса
        waitpid(secondChild, &flag, 0);
        // Буфер для считывания данных из канала;
        char bufferThree[bufSize];
        // Чтение данных из канала в буфер bufferThree;
        size_t bytesRead = read(fileDescriptor[0], bufferThree, bufSize);
        int fileDescriptorThree;
        // Открытие выходного файла для записи;
        if ((fileDescriptorThree = open(output, O_WRONLY | O_CREAT, 0666)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        // Запись данных из буфера bufferThree в выходной файл.
        write(fileDescriptorThree, bufferThree, bytesRead);
    }

    return 0;
}
