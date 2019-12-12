#define _GNU_SOURCE
#include "errno.h"
#include "linux/limits.h"
#include "poll.h"
#include "pthread.h"
#include "signal.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/epoll.h"
#include "sys/resource.h"
#include "sys/select.h"
#include "sys/time.h"
#include "sys/types.h"
#include "unistd.h"

#define SZ 4096
#define CLIENTS_MAX 64
#define TASKS_MAX 128
#define THREADS_NUM 4

typedef struct {
    char* channel;
    char* filename;
} task;

int schedule[TASKS_MAX];
task tasks[TASKS_MAX];

void post_task(char* client_channel, char* client_filename)
{
    task new = {};
    memcpy(new.channel, client_channel, sizeof(&client_channel));
    memcpy(new.filename, client_filename, sizeof(&client_channel));

    for (size_t i = 0; i < TASKS_MAX; i++)
    {
        if (!schedule[i])
        {
            schedule[i] = 1;
            memcpy(&tasks[i], &new, sizeof(new));
            printf("%s\n", tasks[i].channel);
            printf("%s\n", tasks[i].filename);
            return;
        }
        printf("We are at capacity, kindly fuck off");
    }
}

typedef struct {
    char* path_commands[PATH_MAX];
    char* path_file[PATH_MAX];
    char* name_file[PATH_MAX];
    int request;
} client;

void server_routine(void* args)
{
    int id = (int) args;

    while (1)
    {
        for (size_t i = id; i < TASKS_MAX; i += THREADS_NUM)
        {
            if (schedule[i])
            {
                /*transit file here*/
                break;
            }
        }
    }
}

char** parse_register(char* data)
{
    int counter = 0;
    char delim[] = " \n";
    char** list = (char**) malloc(PATH_MAX * sizeof(char*));

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
    int ret = 0;
    struct pollfd fds[65]; // 64 clients and control fifo

    int tv = 30000;

    fds[0].fd = 0;
    fds[0].events = 0 | POLLIN;

    client clients[CLIENTS_MAX];
    int client_counter = 0;

    memset(schedule, 0, sizeof(schedule));

    while (1)
    {
        int select_ret = poll(fds, 1, tv);
        if (select_ret == 0)
            perror("Timeout");
        else
        {
            memset(buf, 0, sizeof(buf));
            for (size_t i = 0; i < 1; i++)
            {
                if (fds[i].revents & POLLIN)
                {
                    ret = read(fd, buf, sizeof(buf));
                    if (i == 0)
                    {
                        char** args = parse_register(buf);
                        if (strcmp(args[0], "REGISTER"))
                        {
                            printf("the fuck goes on\n");
                            continue;
                        }
                        printf("%s\n", args[1]);
                        printf("%s\n", args[2]);

                        memcpy(clients[client_counter].path_commands, args[1], sizeof(&args[1]));
                        memcpy(clients[client_counter].path_file, args[2], sizeof(&args[2]));
                        clients[client_counter].request = 0;

                        //TODO: open file descriptors

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
                        clients[i - 1].request = 1;
                        post_task(clients[i - 1].path_file, clients[i - 1].name_file);
                    }
                }
            }

            if (select_ret == -1 || ret == -1)
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
