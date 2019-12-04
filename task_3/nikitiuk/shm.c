#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SIZE 4096
#define DATA_SIZE 1000000

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
    key_t key = ftok("shm.c", 128);

    pid_t pid = fork();

    if (pid != 0)
    {
        int shmid = shmget(key, 5000, IPC_CREAT | 0666);
        char* buffer = shmat(shmid, NULL, 0);

        int input = open("enwik9", O_RDONLY);
        char* data = malloc(DATA_SIZE);
        read(input, data, DATA_SIZE);
        close(input);

        buffer[SIZE] = 0;

        long long start = get_time();

        for (int done = 0; done < DATA_SIZE; done += SIZE)
        {
            memcpy(buffer, &data[done], minimum(DATA_SIZE - done, SIZE));
            buffer[SIZE] = 1;
            //printf("%d\n", done);
            while (buffer[SIZE])
                usleep(1);
        }

        long long end = get_time();

        long delta = end - start;

        printf("%ld\n", delta);

        shmdt(buffer);
        usleep(10);
        free(data);
    }
    else
    {
        int shmid = shmget(key, SIZE + 1, IPC_CREAT | 0666);
        char* buffer = shmat(shmid, NULL, 0);

        int output = open("output", O_WRONLY | O_CREAT, 0644);
        char* data = malloc(DATA_SIZE);

        for (int done = 0; done < DATA_SIZE; done += SIZE)
        {
            while (!(buffer[SIZE]))
                usleep(1);
            //printf("%s\n", buffer);
            memcpy(&data[done], buffer, minimum(DATA_SIZE - done, SIZE));
						buffer[SIZE] = 0;
        }
				printf("%s\n", data);

        write(output, data, DATA_SIZE);
        close(output);
        free(data);
        shmdt(buffer);
    }

    return 0;
}
