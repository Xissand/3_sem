#define _GNU_SOURCE
#include "errno.h"
#include "linux/limits.h"
#include "poll.h"
#include "signal.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/epoll.h"
#include "sys/resource.h"
#include "sys/time.h"
#include "sys/types.h"
#include "unistd.h"
#include <sys/select.h>

#define SZ 4096
#define CLIENTS_MAX 64

typedef struct {
    char* path_commands[PATH_MAX];
    char* path_file[PATH_MAX];
    char* name_file[PATH_MAX];
    int request;
} client;

void server_routine(char* name, char* fifo)
{
    // send stuff here
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

                        // FIXME: replace = with memcpy
                        memcpy(clients[client_counter].path_commands, args[1], sizeof(&args[1]));
                        memcpy(clients[client_counter].path_file, args[2], sizeof(&args[2]));
                        //clients[client_counter].name_file = NULL;
                        clients[client_counter].request = 0;
                        client_counter++;
                    } else
                    {
                      //parse file reques
                      char** args = parse_register(buf);
                      if (strcmp(args[0], "GET"))
                      {
                          printf("the fuck goes on but in a different way\n");
                          continue;
                      }
                      printf("%s\n", args[1]);
                      printf("%s\n", args[2]);

                      // FIXME: replace = with memcpy
                      memcpy(clients[client_counter].path_commands, args[1], sizeof(&args[1]));
                      memcpy(clients[client_counter].path_file, args[2], sizeof(&args[2]));
                      //clients[client_counter].name_file = NULL;
                      clients[client_counter].request = 0;
                      client_counter++;
                    }
                }
            }

            if (select_ret == -1 || ret == -1)
                perror("mate i have no clue what you did here");
            else
            {
                for (size_t i = 0; i < client_counter; i++)
                {
                    if (clients[i].request)
                        server_routine(*clients[i].name_file, *clients[i].path_file);
                }
            }
        }
    }

    return 0;
}
