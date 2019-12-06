#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

const int SIZE = 4096;
#define DATA_SIZE 1000000000

long long get_time() // Returns current time in microseconds
{
    struct timeval time;
    gettimeofday(&time, NULL);
    long long curr = time.tv_sec * 1000000 + time.tv_usec;
    return curr;
}

int minimum(int a, int b)
{
    if (a > b)
        return b;
    else
        return a;
}

int main(int argc, char* argv[])
{
    mknod("fifo", S_IFIFO|0666, 0);
    pid_t pid = fork();

    if (pid != 0)
    {
        int buffer = open("fifo", O_WRONLY);

        int input = open("enwik9", O_RDONLY);
        char* data = malloc(DATA_SIZE);
        read(input, data, DATA_SIZE);
        close(input);

        long long start = get_time();
        FILE* log = fopen("data.csv", "a");
        fprintf(log, "fifo,%d,%lld", SIZE, start); // Add data to log file
        fclose(log);

        for (int done = 0; done < DATA_SIZE; done += SIZE)
        {
            write(buffer, &data[done], minimum(DATA_SIZE - done, SIZE));
        }
        free(data);
    }
    else
    {
        int buffer = open("fifo", O_RDONLY);

        int output = open("output", O_WRONLY | O_CREAT, 0644);
        char* data = malloc(DATA_SIZE);

        for (int done = 0; done < DATA_SIZE; done += SIZE)
        {
            read(buffer, &data[done], minimum(DATA_SIZE - done, SIZE));
        }
        long long end = get_time();
        FILE* log = fopen("data.csv", "a");
        fprintf(log, ",%lld\n", end); // Add data to log file
        fclose(log);

        write(output, data, DATA_SIZE);
        close(output);
        free(data);
    }

    return 0;
}
