#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int txd[2]; // Transmission from parent to child
    int rxd[2]; // Transmission from child to parent
} dpipe_t;      // Duplex pipe

int main()
{
    dpipe_t dpipe;
    char buf[65536];
    int size;

    if ((pipe(dpipe.txd) < 0) | (pipe(dpipe.rxd) < 0)) // Initialize pipes
    {
        puts("Pipe creation failed");
        return 42;
    }

    const pid_t pid = fork(); // Create child

    if (pid < 0)
    {
        puts("Fork failed");
        return 42;
    }

    while (1)
    {
        if (pid) // Process is a parent
        {
            close(dpipe.txd[0]); // Close p->ch exit
            close(dpipe.rxd[1]); // Close ch->p entrance

            /*Buffer size of read is the size of a pipe buffer. Hence any message that gets read will fit into the pipe,
            so everything we write is guaranteed to be transmitted. If user inputs more then 16 pages into "0" file
            descriptor, after writing the buffer read will simply continue to read user's input with new input being
            appended at the end. Therefore i/o doesn't need to be wrapped in loop.*/

            size = read(0, buf, sizeof(buf) - 1); // Get data from user
            buf[size] = '\0';                     // Format to standart string
            write(dpipe.txd[1], buf, size);       // Transmit data to child

            if (strcmp(buf, "exit\n") == 0) // Exit on exit message from user
            {
                puts("Parent terminated by user");
                return 0;
            }

            printf("Parent:	Sent to child: %s", buf);

            size = read(dpipe.rxd[0], buf, sizeof(buf) - 1); // Read data from child

            buf[size] = '\0';

            if (strcmp(buf, "exit\n") == 0) // Exit if child was terminated by user
            {
                puts("Parent terminated by child");
                return 0;
            }

            printf("Parent:	Received from child: %s", buf);
        }
        else // Process is a child
        {
            close(dpipe.txd[1]); // Close p->ch exit
            close(dpipe.rxd[0]); // Close ch->p entrance

            size = read(dpipe.txd[0], buf, sizeof(buf) - 1); // Read data from parent

            buf[size] = '\0';

            if (strcmp(buf, "exit\n") == 0) // Exit if parent was terminated by user
            {
                puts("Child terminated by parent");
                return 0;
            }

            printf("Child:	Received from parent: %s", buf);

            size = read(0, buf, sizeof(buf) - 1); // Get data from user
            buf[size] = '\0';                     // Format to standart string
            write(dpipe.rxd[1], buf, size);       // Transmit data to parent

            if (strcmp(buf, "exit\n") == 0) // Exit on exit message from user
            {
                puts("Child terminated by user");
                return 0;
            }

            printf("Child:	Sent to parent: %s", buf);
        }
    }
    return 0;
}
