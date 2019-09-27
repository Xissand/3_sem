#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct {
	int txd[2];		//Transmission from parent to child
	int rxd[2]; 	//Transmission from child to parent
} dpipe_t; 			//Duplex pipe


int main()
{
	dpipe_t dpipe;
	char buf[4096];
	int size;

	if ((pipe(dpipe.txd) < 0) | (pipe(dpipe.rxd) < 0))	//Initialize pipes
	{
		puts("Pipe creation failed");
		return 42;
	}

	const pid_t pid = fork();	//Create child

	if (pid < 0)
	{
		puts("Fork failed");
		return 42;
	}

	while (1)
	{
		if (pid) //Process is a parent
		{
			close(dpipe.txd[0]); //Close p->ch exit
			close(dpipe.rxd[1]); //Close ch->p entrance

			size = read(0, buf, sizeof(buf) - 1); //Transmit data to child
			buf[size] = '\0'; // the text string data is expected
			write(dpipe.txd[1], buf, size);
			printf("Parent:	Sent to child: %s", buf);

			if (strcmp(buf, "exit\n") == 0)	//Exit on exit message (input as always ends with new line)
			{
				puts("Parent terminated by user");
				return 0;
			}

			sleep(1); //Wait for child to process input

			size = read(dpipe.rxd[0], buf, sizeof(buf) - 1); //Read data from child

			buf[size] = '\0'; // the text string data is expected
			printf("Parent:	Received from child: %s", buf);

			if (strcmp(buf, "exit\n") == 0) //Exit if child exited whithout fatal errors from write
			{
				puts("Parent terminated by child");
				return 0;
			}


		}
		else //Process is a child
		{
			close(dpipe.txd[1]); //Close p->ch exit
			close(dpipe.rxd[0]); //Close ch->p entrance

			size = read(dpipe.txd[0], buf, sizeof(buf) - 1); //Read data from parent

			buf[size] = '\0'; // the text string data is expected
			printf("Child:	Received from parent: %s", buf);

			if (strcmp(buf, "exit\n") == 0)	//Exit if parent exited whithout fatal errors from write
			{
				puts("Child terminated by parent");
				return 0;
			}

			sleep(1);

			size = read(0, buf, sizeof(buf) - 1); //Transmit data to parent

			buf[size] = '\0'; // the text string data is expected
			write(dpipe.rxd[1], buf, size);
			printf("Child:	Sent to parent: %s", buf);

			if (strcmp(buf, "exit\n") == 0)	//Exit on exit message (input as always ends with new line)
			{
				puts("Child terminated by user");
				return 0;
			}

		}
	}
	return 0;
}