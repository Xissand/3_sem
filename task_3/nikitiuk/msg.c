#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SIZE 1024
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

typedef struct {
    long mtype;
    char mtext[SIZE];
} message_buf;

int main(int argc, char* argv[])
{
    key_t key = ftok("msg.c", 128);

    pid_t pid = fork();

		int msqid = msgget(key, IPC_CREAT | 0666);
		message_buf buffer;
		buffer.mtype = 10;

    if (pid != 0)
    {
        int input = open("enwik9", O_RDONLY);
        char* data = malloc(DATA_SIZE);
        read(input, data, DATA_SIZE);
        close(input);

        long long start = get_time();
        FILE* log = fopen("data.csv", "a");
        fprintf(log, "msg,%d,%lld", SIZE, start); // Add data to log file
        fclose(log);

        for (int done = 0; done < DATA_SIZE; done += SIZE)
        {
            memcpy(buffer.mtext, &data[done], minimum(DATA_SIZE - done, SIZE));
            msgsnd(msqid, &buffer, SIZE, 0);
        }
        free(data);
    }
    else
    {
        int output = open("output", O_WRONLY | O_CREAT, 0644);
        char* data = malloc(DATA_SIZE);

        for (int done = 0; done < DATA_SIZE; done += SIZE)
        {
					msgrcv(msqid, &buffer, SIZE, 10, MSG_NOERROR);
					//For whatever reason reading minimum(...) instead of SIZE was causing
					// one message to be left in queue. MSG_NOERROR just cuts out the useless part
					memcpy(&data[done], buffer.mtext, minimum(DATA_SIZE - done, SIZE));
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
