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

#ifndef LINE_MAX
#define LINE_MAX 4096
#endif

/*
 * LIST_SORT	( listelm, entry_type, field, cmp_func )
 * 		
 *		- list heap sort by 'data_name'
 * 		@ listelm		:  list
 * 		@ entry_type	:  the type of the entry structure
 * 		@ field			:  the name of the list entry
 * 		@ cmp_func		:  the name of the compare function to be used in sort
 *					: 		
 * 					:  int cmp_func( entry_type *lhs, entry_type *rhs )
 * 					:  The comparison function must return a negative value if
 *					:  @a should sort before @rhs, and a positive value if @lhs should sort
 *					:  after @rhs. If @lhs and @rhs are equivalent, and their original
 *					:  relative ordering is to be preserved, @cmp_func must return 0.
 */

#define __LIST_SORT( listelm, first_elem, elem_type, field, cmp_for_sort ) do{\
elem_type	*p; \
elem_type	*q;\
elem_type	*e;\
elem_type 	*tail;\
int			insize; \
int 		nmerges;\
int			psize; \
int			qsize;\
int			i;\
if (first_elem == NULL){\
    (listelm)->lh_first = NULL;\
}\
else{\
    insize = 1;\
    while (1) {\
        p = first_elem;\
        first_elem = NULL;\
        tail = NULL;\
        nmerges = 0;\
        while (p) {\
            nmerges++;\
            q = p;\
            psize = 0;\
            for (i = 0; i < insize; i++) {\
                psize++;\
                q = q->field.le_next;\
                if (!q) break;\
            }\
            qsize = insize;\
            while (psize > 0 || (qsize > 0 && q)) {\
                if (psize == 0) {\
                    e = q; q = q->field.le_next; qsize--;\
                } else if (qsize == 0 || !q) {\
                    e = p; p = p->field.le_next; psize--;\
                } else if (cmp_for_sort( p, q ) <= 0) {\
                    e = p; p = p->field.le_next; psize--;\
                } else {\
                    e = q; q = q->field.le_next; qsize--;\
                }\
                if (tail) {\
                    tail->field.le_next = e;\
                } else {\
                    first_elem = e;\
                }\
                e->field.le_prev = &tail->field.le_next;\
                tail = e;\
            }\
            p = q;\
        }\
        tail->field.le_next = NULL;\
        if (nmerges <= 1){\
            (listelm)->lh_first = first_elem;\
            break;\
        }\
        insize *= 2;\
    }\
}\
} while(0)

#define LIST_SORT( listelm, entry_type, field, data_name )	{\
__LIST_SORT( listelm, (listelm)->lh_first, entry_type, field, data_name );\
(listelm)->lh_first->field.le_prev = &(listelm)->lh_first;\
}

int globalID = 0;
pid_t old_pid;

typedef struct _entry entry;
struct _entry {
    int     _id;
    char**  _argv;
    int     _status;
    int     _ret;
    pid_t   _pid;
    unsigned long _start;
    unsigned long _stop;
    LIST_ENTRY(_entry) entries;
};

typedef void (*func_ptr)(char **argv);
typedef LIST_HEAD(tailhead, _entry) equeue;

typedef struct _command command;
struct _command{
    char* _command;
    func_ptr _f;
};

enum JSTATUS{
    WAITING     = 0,
    RUNNING     = 1,
    COMPLETED   = 2
};

const char* JSTATUS_str[] = {
    "WAITING",
    "RUNNING",
    "COMPLETED"
};

static inline const char* jstatus_string(int js){
    return JSTATUS_str[js];
}

static inline int id_cmp(entry* lhs, entry *rhs){
    return lhs->_id - rhs->_id;
}

unsigned long hash(unsigned char *str);

char** str_split(char* a_str, const char a_delim);

void submit(char **argv);

void show(char **argv);

void jobhistory(char **argv);

void hello(char **argv);

void fibonacci(char** argv);

void list(char** argv);

void cd(char** argv);

void help(char **argv);

void quit(char **argv);

void history(char **argv);

command Commands[] = {
    { ._command = "submit",          ._f = submit       },
    { ._command = "showjobs",        ._f = show         },
    { ._command = "submithistory",   ._f = jobhistory   },
    { ._command = "hello",           ._f = hello        },
    { ._command = "fibonacci",       ._f = fibonacci    },
    { ._command = "list",            ._f = list         },
    { ._command = "cd",              ._f = cd           },
    { ._command = "help",            ._f = help         },
    { ._command = "quit",            ._f = quit         },
    { ._command = "history",         ._f = history      }
};

equeue head;

int main(int argc, char **argv){
    int opt;
    int conc = 2;
    FILE *in;
    old_pid = getpid();
    ssize_t linen;
    char *buffer = 0;
    int i = 0;
    FILE* fh;
    
    LIST_INIT(&head); //initialize queue

    //getopt to scan arguments from command line
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt){
            case 'p':
                conc = atoi(optarg);
                break;
            
            default: /* '?' */
                fprintf(stderr, "Usage: %s [directory]\n",argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    if( fork() < 0){
        perror("fork failed");
    }else{
        if(getpid() != old_pid){
            int running = 0;
            pid_t sched = getpid();
            for(;;){
                entry *e;
                entry *p;
                
                //scheduling loop
                if(running < conc){
                    LIST_FOREACH(e, &head, entries) {
                        if(e->_status != WAITING){
                            continue;
                        }
                        
                        if( fork() < 0){
                            perror("fork failed");
                        }else{
                            p = e;
                            if(getpid() != sched){
                                char buffer[4096];
                                p->_pid = getpid();
                                
                                sprintf(buffer, "./%d.out", p->_id);
                                int fd_out = open(buffer, O_WRONLY, 0666);
                                dup2(fd_out, STDOUT_FILENO);
                                
                                sprintf(buffer, "./%d.err", p->_id);
                                int fd_err = open(buffer, O_WRONLY, 0666);
                                dup2(fd_err, STDERR_FILENO);
                                p->_start = time(NULL);
                                
                                if(execvp(p->_argv[0], p->_argv) < 0){
                                    printf("Command not found\r\n");
                                    exit(-1);
                                }
                            }
                        }
                    }
                }
                
                if(running > 0){
                    int status = 0;
                    pid_t ep = waitpid(-1,&status,WNOHANG);
                    LIST_FOREACH(e, &head, entries) {
                        if(ep == e->_pid){
                            --running;
                            e->_status = COMPLETED;
                            e->_stop = time(NULL);
                            e->_ret = WIFEXITED(status);
                            break;
                        }
                    }
                }
                
                
                sleep(1); //scheduler resolution 1s
                
            }
        }
    }
    
    if(!(in = fdopen(0, "r"))){
        fprintf(stderr, "Failed to open input stream");
        exit(EXIT_FAILURE);
    }
    
    if(!(fh = fopen("./uab_sh.log","a+"))){
        fprintf(stderr, "Failed to open history log stream");
        exit(EXIT_FAILURE);
    }
    
    while(!feof(in)){
        printf("uab_sh>");
        size_t rest = 0;
        ssize_t chars = getline(&buffer,&rest,in);
        buffer[strlen(buffer)-1] = 0;
        
        if(strlen(buffer) == 0){
            goto NEXT_INPUT;
        }
        
        fprintf(fh,"%s\r\n",buffer);
        fflush(fh);
        
        char **splitted = str_split(buffer,' ');
        for(i = 0; i < sizeof(Commands)/sizeof(command); ++i){
            if(strcmp(Commands[i]._command,splitted[0]) == 0){
                Commands[i]._f(splitted);
                goto NEXT_INPUT;
            }
        }
        
       
        if( fork() < 0){
            perror("fork failed");
        }else{
            if(getpid() != old_pid){
                if(execvp(splitted[0], splitted) < 0){
                    printf("Command not found\r\n");
                    exit(-1);
                }
            }
        }
            
        int status = 0;
        wait(&status);
        
        NEXT_INPUT:
    }
    
    fclose(in);
    fclose(fh);
    printf("\r\n");
    
    return 0;
}

void submit(char **argv){
    entry *elem;
    
    //prepare new element allocate memory and set
    if((elem = malloc(sizeof(entry)))){
        memset(elem,0,sizeof(entry));
        elem->_argv = argv;
        elem->_status = WAITING;
        elem->_id = globalID++;
    }
    
    LIST_INSERT_HEAD(&head, (struct _entry*) elem, entries);
    LIST_SORT( &head, struct _entry, entries, id_cmp );
}

void show(char **argv){
    entry *e;
    
    printf("jobid\t\tCOMMAND\t\t\tSTATUS\r\n");
    LIST_FOREACH(e, &head, entries) {
        if(e->_status != COMPLETED){
            printf("%d\t\t",e->_id);
            char **ptr;
            for( ptr = argv; *ptr != NULL; ++ptr){
                printf("%s",*ptr);
            }
            printf("\t\t%s\r\n",jstatus_string(e->_status));
        }
    }
}

void jobhistory(char **argv){
    entry *e;
    char buffer[256];
    struct tm* tm_info;
    
    printf("jobid\t\tCOMMAND\t\t\tSTART\t\tSTOP\t\tSTATUS\r\n");
    LIST_FOREACH(e, &head, entries) {
        if(e->_status == COMPLETED){
            printf("%d\t\t",e->_id);
            char **ptr;
            for( ptr = argv; *ptr != NULL; ++ptr){
                printf("%s ",*ptr);
            }
            
            tm_info = localtime(&e->_start);
            strftime(buffer, 256, "%Y-%m-%d %H:%M:%S", tm_info);
            printf("\t\t%s\t\t",buffer);
            
            tm_info = localtime(&e->_stop);
            strftime(buffer, 256, "%Y-%m-%d %H:%M:%S", tm_info);
            printf("%s\t\t",buffer);

            printf("%s\r\n",e->_ret ? "Success" : "Failed");
        }
    }
}

void fibonacci(char** argv){
    int i, n;
    
    // initialize first and second terms
    int t1 = 0, t2 = 1;
    
    // initialize the next term (3rd term)
    int nextTerm = t1 + t2;
    
    if(argv[1] != NULL){
        n = atoi(argv[1]);
    }else{
        // get no. of terms from user
        printf("Enter the number of terms: ");
        scanf("%d", &n);
    }
    
    // print the first two terms t1 and t2
    printf("Fibonacci Series: %d, %d, ", t1, t2);
    
    // print 3rd to nth terms
    for (i = 3; i <= n; ++i) {
        printf("%d, ", nextTerm);
        t1 = t2;
        t2 = nextTerm;
        nextTerm = t1 + t2;
    }
    
    printf("\r\n");
}

void hello(char **argv){
    printf("Hello world\r\n");
}

void list(char** argv){
    if( fork() < 0){
        perror("fork failed");
    }else{
        if(getpid() != old_pid){
            char *args[] = { "ls", 0 };
            if(execvp("ls",args) < 0){
                printf("Command not found\r\n");
                exit(-1);
            }
        }
    }
    
    int status = 0;
    wait(&status);
}

void cd(char** argv){
    chdir(argv[0]);
}

void help(char **argv){
    printf("Commands:\r\n");
    printf("hello\t\t\tprint hello world\r\n");
    printf("list\t\t\tshow files in current directory\r\n");
    printf("cd\t\t\tchange current directory\r\n");
    printf("fibonacci [N]\t\tprint N element of fibonacci series\r\n");
    printf("quit\t\t\tquit\r\n");
    printf("history\t\t\tprint command history\r\n");
    printf("submit command args ...\tsubmit background command\r\n");
    printf("showjobs\t\tshow background jobs\r\n");
    printf("submithistory\t\tshow background jobs history\r\n");
}

void quit(char **argv){
    exit(0);
}

void history(char **argv){
    if( fork() < 0){
        perror("fork failed");
    }else{
        if(getpid() != old_pid){
            char *args[] = { "cat", "./uab_sh.log", 0 };
            if(execvp("cat",args) < 0){
                printf("Command not found\r\n");
                exit(-1);
            }
        }
    }
    
    int status = 0;
    wait(&status);
}

char** str_split(char* a_str, const char a_delim){
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    
    /* Count how many elements will be extracted. */
    while (*tmp){
        if (a_delim == *tmp){
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    
    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);
    
    /* Add space for terminating null string so caller
     *      knows where the list of returned strings ends. */
    count++;
    
    result = malloc(sizeof(char*) * count);
    
    if (result){
        size_t idx  = 0;
        char* token = strtok(a_str, delim);
        
        while (token){
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        *(result + idx) = 0;
    }
    
    return result;
}
