#define _GNU_SOURCE
#include "errno.h"
#include "fcntl.h"
#include "linux/limits.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/stat.h"
#include "sys/time.h"
#include "sys/types.h"
#include "unistd.h"

#define SZ 256

long long get_time() // Returns current time in microseconds
{
    struct timeval time;
    gettimeofday(&time, NULL);
    long long curr = time.tv_sec * 1000000 + time.tv_usec;
    return curr;
}

int main()
{
    printf("Connecting to server\n");
    int reg = open("registry", O_RDWR);
    if (reg == -1)
    {
        perror("Could not connect to server");
        return 0;
    }

    char comms[SZ];
    char files[SZ];
    char buffer[SZ];

    srand(get_time());

    sprintf(comms, "%d.tx", rand());
    sprintf(files, "%d.rx", rand());

    sprintf(buffer, "REGISTER %s %s", comms, files);

    mknod(comms, S_IFIFO | 0666, 0);
    mknod(files, S_IFIFO | 0666, 0);

    write(reg, buffer, sizeof(buffer));
    sleep(1);
    /*read(reg, buffer, sizeof(buffer));

    if (strcmp(buffer, "ACK"))
    {
        printf("Invalid responce from server: %s\n", buffer);
        return 0;
    }*/

    int commands = open(comms, O_WRONLY);
    int transfer = open(files, O_RDONLY);

    int sz = 0;

    while (1)
    {
        printf("Input filename: ");
        read(0, buffer, sizeof(buffer));
        // buffer[sizeof(buffer)-1]=NULL;
        printf("Reqesting file: %s\n", buffer);

        sprintf(buffer, "GET %s", buffer);
        write(commands, buffer, sizeof(buffer));

        while ((sz = read(transfer, buffer, sizeof(buffer))) > 0)
        {
            write(1, buffer, sz);
        }
        printf("File transfer ended.\n");
    }
    return 0;
}
