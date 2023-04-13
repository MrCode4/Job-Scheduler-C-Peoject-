#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/queue.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "queue.h"

#define SUBMIT "submit"
#define SHOW_JOBS "showjobs\n"
#define SUBMIT_HISTORY "submithistory\n"
#define MAX_LEN 1000

int core_numbers = 2;//default value
int running_numbers = 0;

void print_invalid_command()
{
    puts("Invalid Command!");
}

void submit(char* input[], int size)
{
    if(running_numbers < core_numbers)
    {

    }
    else
    {
        
    }
}

void show_jobs()
{

}

void submit_history()
{

}

void print_command()
{
    printf("Enter Command> ");

    int bytes_read;
    size_t size = 1024;
    char *string;
        
    string = (char *) malloc (size);
    bytes_read = getline (&string, &size, stdin);
    if (bytes_read == -1)
    {
        print_invalid_command();
    }
    else
    {
        char* inputs[MAX_LEN];
        int inputs_size = 0;

        char* token = strtok(string, " ");
        while(token != NULL)
        {
            inputs[inputs_size] = token;
            inputs_size++;

            token = strtok(NULL, " ");
        }

        if(inputs_size == 0)
        {
            print_invalid_command();
        }
        else if(strcmp(inputs[0], SUBMIT) == 0)
        {        
            submit(inputs, inputs_size);
        }
        else if(strcmp(inputs[0], SHOW_JOBS) == 0)
        {
            if(inputs_size > 1)
                print_invalid_command();
            else 
                show_jobs();
        }
        else if(strcmp(inputs[0], SUBMIT_HISTORY) == 0)
        {
            if(inputs_size > 1)
                print_invalid_command();
            else 
                submit_history();
        }
        else
        {
            print_invalid_command();
        }
    }

    print_command();
}

int main(int argc, char **argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt){
            case 'p':
                core_numbers = atoi(optarg);
                printf("Number of cores are: %d\n", core_numbers);
                break;
            
            default: /* '?' */
                fprintf(stderr, "Usage: %s [directory]\n",argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    print_command();


    return 0;
}