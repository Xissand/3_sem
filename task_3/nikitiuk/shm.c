#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
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

const int SIZE = 524288;
const int DATA_SIZE = 1000000000;

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
    key_t key = ftok("enwik9", 128);

    pid_t pid = fork();

    sem_t* sem[2];
    sem[0] = sem_open("semaphore", O_CREAT, 0666, 0);
    sem[1] = sem_open("another_semaphore", O_CREAT, 0666, 0);

    if (pid != 0)
    {
        int shmid = shmget(key, SIZE + 1, IPC_CREAT | 0666 | SHM_HUGETLB);
        char* buffer = shmat(shmid, NULL, 0);

        int input = open("enwik9", O_RDONLY);
        char* data = malloc(DATA_SIZE);
        read(input, data, DATA_SIZE);
        close(input);

        long long start = get_time();
        FILE* log = fopen("data.csv", "a");
        fprintf(log, "shm,%d,%lld", SIZE, start); // Add data to log file
        fclose(log);

        for (int done = 0; done < DATA_SIZE; done += SIZE)
        {
            memcpy(buffer, &data[done], minimum(DATA_SIZE - done, SIZE));
            sem_post(sem[0]);
            sem_wait(sem[1]);
        }
        shmdt(buffer);
        free(data);
    }
    else
    {
        int shmid = shmget(key, SIZE + 1, IPC_CREAT | 0666 | SHM_HUGETLB);
        char* buffer = shmat(shmid, NULL, 0);

        int output = open("output", O_WRONLY | O_CREAT, 0644);
        char* data = malloc(DATA_SIZE);

        for (int done = 0; done < DATA_SIZE; done += SIZE)
        {
            sem_wait(sem[0]);
            memcpy(&data[done], buffer, minimum(DATA_SIZE - done, SIZE));
            sem_post(sem[1]);
        }
        long long end = get_time();
        FILE* log = fopen("data.csv", "a");
        fprintf(log, ",%lld\n", end); // Add data to log file
        fclose(log);
        // printf("%s\n", data);

        write(output, data, DATA_SIZE);
        close(output);
        free(data);
        shmdt(buffer);
    }
    sem_close(sem[0]);
    sem_close(sem[1]);
    sem_unlink("semaphore");
    sem_unlink("another_semaphore");

    return 0;
}
