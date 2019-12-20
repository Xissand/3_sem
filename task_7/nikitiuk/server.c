#define _GNU_SOURCE
#include "errno.h"
#include "fcntl.h"
#include "linux/limits.h"
#include "poll.h"
#include "pthread.h"
#include "signal.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/stat.h"
#include "sys/time.h"
#include "sys/types.h"
#include "unistd.h"

#define SZ 256
#define CLIENTS_MAX 64
#define TASKS_MAX 128
#define THREADS_NUM 4

typedef struct {
    int channel;
    char filename;
} task;

int schedule[TASKS_MAX];
task tasks[TASKS_MAX];

void post_task(int client_channel, char client_filename[SZ])
{
    task new;
    new.channel = client_channel;
    memcpy(&new.filename, &client_filename, SZ*sizeof(char));

    for (size_t i = 0; i < TASKS_MAX; i++)
    {
        if (!schedule[i])
        {
            schedule[i] = 1;
            memcpy(&tasks[i], &new, sizeof(new));
            printf("%d\n", tasks[i].channel);
            printf("%c\n", tasks[i].filename);
            return;
        }
        printf("We are at capacity, kindly fuck off");
    }
}

typedef struct {
    int path_commands; // fd
    int path_file;     // fd
    char name_file[SZ];
} client;

void* server_routine(void* args)
{
    int id = *(int*) args;
    printf("server thread %d initialized\n", id);

    while (1)
    {
        for (size_t i = id; i < TASKS_MAX; i += THREADS_NUM)
        {
            if (schedule[i])
            {
                int channel = tasks[i].channel;
                char name[SZ];
                memcpy(&name, &tasks[i].filename, SZ*sizeof(char));

                write(channel, "HUI", sizeof("HUI"));
                break;
            }
        }
    }
}

char** parse_register(char* data)
{
    int counter = 0;
    char delim[] = " \n";
    char** list = (char**) malloc(SZ * sizeof(char*));
    for (char* p = strtok(data, delim); p != NULL; p = strtok(NULL, delim))
    {
        list[counter] = p;
        counter = counter + 1;
    }
    list[counter] = NULL;
    return list;
}

int main()
{
    int fd = 0;
    char buf[SZ];
    int read_ret = 0;
    int tv = 30000;
    struct pollfd fds[CLIENTS_MAX + 1]; // 64 clients and control fifo

    system("rm registry");
    //system("rm *.tx *.rx");
    mknod("registry", S_IFIFO | 0666, 0);
    int reg = open("registry", O_RDWR);
    fds[0].fd = reg;
    fds[0].events = 0 | POLLIN;

    client clients[CLIENTS_MAX + 1];
    int client_counter = 1;
    memset(schedule, 0, sizeof(schedule));

    int ids[THREADS_NUM];
    for (size_t i = 0; i < THREADS_NUM; i++) {
      ids[i] = i;
    }

    pthread_t thread[THREADS_NUM];
    for (int i = 0; i < THREADS_NUM; i++)
        pthread_create(&thread[i], NULL, server_routine, &ids[i]);

    printf("server waiting for connection\n");

    while (1)
    {
        errno = 0;
        int poll_ret = poll(fds, 1, tv);
        if (poll_ret == 0)
            perror("Timeout");
        else
        {
            memset(buf, 0, sizeof(buf));
            for (size_t i = 0; i < client_counter; i++)
            {
                if (fds[i].revents & POLLIN)
                {
                    read_ret = read(fds[i].fd, buf, sizeof(buf));
                    if (i == 0)
                    {
                        char** args = parse_register(buf);
                        if (strcmp(args[0], "REGISTER"))
                        {
                            printf("the fuck goes on: %s\n", args[0]);
                            continue;
                        }
                        printf("%s\n", args[1]);
                        printf("%s\n", args[2]);

                        int client_comms = open(args[1], O_RDWR);
                        int client_transfer = open(args[2], O_RDWR);

                        clients[client_counter].path_commands = client_comms;
                        clients[client_counter].path_file = client_transfer;

                        fds[client_counter].fd = client_comms;
                        fds[client_counter].events = 0 | POLLIN;

                        //write(reg, "ACK", sizeof("ACK"));
                        client_counter++;
                    }
                    else
                    {
                        // parse file requests
                        char** args = parse_register(buf);
                        if (strcmp(args[0], "GET"))
                        {
                            printf("the fuck goes on but in a different way\n");
                            continue;
                        }
                        printf("%s\n", args[1]);

                        memcpy(clients[client_counter].name_file, args[1], sizeof(&args[1]));
                        post_task(clients[i].path_file, clients[i].name_file);
                    }
                }
            }

            if (poll_ret == -1 || read_ret == -1)
                perror("mate i have no clue what you did here");
            else
            {
                /*for (size_t i = 0; i < client_counter; i++)
                {
                    if (clients[i].request)
                        server_routine(*clients[i].name_file, *clients[i].path_file);
                }*/
            }
        }
    }

    return 0;
}
