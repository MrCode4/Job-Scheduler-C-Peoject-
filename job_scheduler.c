#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

#define SIG_SUBMIT 40
#define SIG_SHOW_JOB 41
#define SIG_SUBMIT_HISTORY 42

int core_numbers = 2;//default value
int running_numbers = 0;
int jobs_number = 0;

enum STATUS{
    WAITING     = 0,
    RUNNING     = 1,
    COMPLETED   = 2
};

typedef struct jobhistory{
    int id;
    char* start_time;
    char* end_time;
    int status;
    pid_t process_id;
    char* cmd;
}Job;

Job jobs[MAX_LEN];

queue* waiting_jobs;

const char* status_array[] = {"Watting", "Running", "Completed"};

pid_t child_pid;

FILE* file;

char* int_to_string(int num)
{
    char* string = malloc(sizeof(char) * MAX_LEN);
    sprintf(string, "%d", num);

    return string; 
}

char* get_input()
{
    file = fopen("input.in", "r");
 
    char* input = malloc(sizeof(char) * MAX_LEN);
    int size = 0;

    if (NULL == file) {
        return "";
    }

    char ch;
    while(true) {
        ch = fgetc(file);
        
        if(ch == EOF || ch == '\n')
            break;

        input[size] = ch;
        size++;
    } 

    input[size] = '\0';
    
    return input;
}

void open_write_to_file(char* input[], int size)
{
    file = fopen("input.in", "w"); // "w" defines "writing mode"

    for (int i = 1; i < size; i++) {
        /* write to file using fputc() function */
        for(int j=0; input[i][j] != '\0'; j++)
            fputc(input[i][j], file);
        
        fputc(' ', file);
    }
    fclose(file);
}

const char* status_to_string(int ix)
{
    return status_array[ix];
}

char* get_current_time()
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    char * current_time = asctime (timeinfo);
    current_time[strlen(current_time)-1] = '\0';

    return current_time;
}

void print_invalid_command()
{
    puts("Invalid Command!");
}

void submit()
{
    signal(SIG_SUBMIT, submit);
    
    char* input = get_input();

    remove("input.in");

    Job new_job;
    new_job.id = jobs_number;
    new_job.status = WAITING;
    new_job.cmd = input;

    jobs[jobs_number] = new_job;
    queue_insert(waiting_jobs, jobs_number);

    jobs_number++;
}

int len(int num)
{
    if(num == 0)
        return 1;

    int res = 0;
    while(num > 0)
    {
        res++;
        num /= 10;
    }

    return res;
}

void show_jobs()
{
    signal(SIG_SHOW_JOB, show_jobs);

    printf("\n---------------------------------------------------\r\n");
    printf("Job_ID          Command                    STATUS\r\n");
    printf("---------------------------------------------------\r\n");

    for(int i=0;i<jobs_number;i++)
    {
        if(jobs[i].status != COMPLETED)
        {
            printf("%d", jobs[i].id);
            for(int j=0; j<16-len(jobs[i].id); j++)
                printf(" ");

            printf("%s", jobs[i].cmd);
            for(int j=0; j<27-strlen(jobs[i].cmd); j++)
                printf(" ");

            printf("%s\r\n", status_array[jobs[i].status]);
        }
    }   
}

void submit_history()
{
    signal(SIG_SUBMIT_HISTORY, submit_history);

    printf("\n--------------------------------------------------------------------------------------------------------\r\n");
    printf("Job_ID          Command                    Start_Time                 End_Time                 STATUS\r\n");
    printf("--------------------------------------------------------------------------------------------------------\r\n");

    for(int i=0;i<jobs_number;i++)
    {
        if(jobs[i].status == COMPLETED)
        {
            printf("%d", jobs[i].id);
            for(int j=0; j<16-len(jobs[i].id); j++)
                printf(" ");

            printf("%s", jobs[i].cmd);
            for(int j=0; j<27-strlen(jobs[i].cmd); j++)
                printf(" ");

            printf("%s", jobs[i].start_time);
            for(int j=0; j<27-strlen(jobs[i].start_time); j++)
                printf(" ");

            printf("%s", jobs[i].end_time);
            for(int j=0; j<25-strlen(jobs[i].end_time); j++)
                printf(" ");

            printf("%s\r\n", status_array[jobs[i].status]);
        }
    }
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
            open_write_to_file(inputs, inputs_size); 

            kill(child_pid, SIG_SUBMIT);
        }
        else if(strcmp(inputs[0], SHOW_JOBS) == 0)
        {
            if(inputs_size > 1)
                print_invalid_command();
            else 
                kill(child_pid, SIG_SHOW_JOB);
        }
        else if(strcmp(inputs[0], SUBMIT_HISTORY) == 0)
        {
            if(inputs_size > 1)
                print_invalid_command();
            else 
                kill(child_pid, SIG_SUBMIT_HISTORY);
        }
        else
        {
            print_invalid_command();
        }
    }

    sleep(1);
    print_command();
}

bool is_file_created(char* file_name)
{
    file = fopen(file_name, "r");

    bool is_exist = false;
    
    if (file != NULL)
    {
        is_exist = true;
        fclose(file); // close the file
    }

    return is_exist;
}

char* add_two_string(char* name1, char* name2)
{
    char* final_string = (char *) malloc(strlen(name1) + strlen(name2) + 1);
    strcpy(final_string, name1);
    strcat(final_string, name2);

    return final_string;
    
}

void check_running_job_status()
{
    running_numbers = 0;
    for(int i=0; i<jobs_number; i++)
    {
        if(jobs[i].status == RUNNING)
        {
            char* name = int_to_string(jobs[i].id);

            char* out_file_name = add_two_string(name, ".out");
            char* err_file_name = add_two_string(name, ".err");
            
            if(is_file_created(out_file_name) &&
               is_file_created(err_file_name))
            {
                jobs[i].status = COMPLETED;
                jobs[i].end_time = get_current_time();
            }
            else
            {
                running_numbers++;
            }
        }
    }
}

void create_write_file(char* file_name, char* text)
{
    file = fopen(file_name, "w"); // "w" defines "writing mode"

    for (int i = 0; text[i] != '\0'; i++) {
        /* write to file using fputc() function */
        fputc(text[i], file);
    }

    fclose(file);
}
void run_job()
{
    int job_id = queue_delete(waiting_jobs);

    if(job_id != -1)
    {
        pid_t current_pid = getpid();

        jobs[job_id].start_time = get_current_time();
        jobs[job_id].status = RUNNING;

        jobs[job_id].process_id = fork();
        if(getpid() != current_pid)
        {
            jobs[job_id].process_id = getpid();

            sleep(10);

            char* name = int_to_string(job_id);
            char* out_file_name = add_two_string(name, ".out");
            char* err_file_name = add_two_string(name, ".err");

            create_write_file(out_file_name, "");
            create_write_file(err_file_name, "");
        }
    }
}

int main(int argc, char **argv)
{
    int opt;

    waiting_jobs = queue_init(MAX_LEN);

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

    pid_t main_pid = getpid();
    
    child_pid = fork();
    if(getpid() != main_pid)
    {
        child_pid = getpid();

        signal(SIG_SUBMIT, submit);
        signal(SIG_SHOW_JOB, show_jobs);
        signal(SIG_SUBMIT_HISTORY, submit_history);

        while(true)
        {
            check_running_job_status();

            if(running_numbers < core_numbers)
                run_job();

            sleep(1);
        }
    }

    print_command();

    return 0;
}