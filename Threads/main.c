#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

int runner = 1;

struct MY_NODE_STRUCT
{
    char* functionid;
    int index;
    struct MY_NODE_STRUCT* next;
};
typedef struct MY_NODE_STRUCT MY_NODE;
static MY_NODE* NODE_TOP = NULL;

char* getHead()
{
    return NODE_TOP->functionid;
}

void insertIntoList(char* toInsert)
{
    char* copy = malloc(sizeof(char) * 25);
    strcpy(copy, toInsert);
    MY_NODE* temp = (MY_NODE *) malloc(sizeof(MY_NODE));
    temp->functionid = copy;
    if(NODE_TOP == NULL){  temp->next = NULL; temp->index = 0;temp->index = 0; NODE_TOP = temp; }
    else
    {
        temp->index = 1 + NODE_TOP->index;
        temp->next = NODE_TOP;
        NODE_TOP = temp;
    }
}
void deallocateList()
{
    MY_NODE*temp = NODE_TOP;
    while(temp != NULL)
    {
        //printf("%s", temp->functionid);
        MY_NODE* del = temp;
        free(del->functionid);
        free(del);
        temp = temp->next;
    }
    NODE_TOP = NULL;
}

void printList()
{
    MY_NODE*temp = NODE_TOP;
    while(temp != NULL)
    {
        printf("%s", temp->functionid);
        temp = temp->next;
    }
}
//thread mutex lock for access to the log index
pthread_mutex_t tlock1 = PTHREAD_MUTEX_INITIALIZER;
//thread mutex lock for critical sections of allocating THREADDATA
pthread_mutex_t tlock2 = PTHREAD_MUTEX_INITIALIZER;
//reaok lock
pthread_mutex_t tlock3= PTHREAD_MUTEX_INITIALIZER;

void* thread_runner(void*);
pthread_t tid1, tid2;
struct THREADDATA_STRUCT
{
    pthread_t creator;
};
typedef struct THREADDATA_STRUCT THREADDATA;

THREADDATA* p=NULL;


//variable for indexing of messages by the logging function
int logindex=0;
int *logip = &logindex;
volatile int readok = 0;


//A flag to indicate if the reading of input is complete,
//so the other thread knows when to stop
bool is_reading_complete = false;

void sig_interrupt_handler(int signal)
{
    runner = 0;
    exit(0);
}


/*********************************************************
// function main  -------------------------------------------------
*********************************************************/
int main()
{

    pid_t child;
    child = fork();
    printf("%d\n",child);

    return 0;
    /*
    //check
    int* z = 6;
    printf("%d\n", ++(*z));
    char* s = "hello ";
    char* q = s;
    printf("%p\t%p", q, s);
    printf("%d", 100 + 200 / 10 - 3 * 10);
        printf("create first thread\n");
        pthread_create(&tid1, NULL, thread_runner, NULL);

        printf("create second thread\n");
        pthread_create(&tid2, NULL, thread_runner, NULL);

        printf("wait for first thread to exit\n");
        pthread_join(tid1, NULL);
        printf("first thread exited\n");

        printf("wait for second thread to exit\n");
        pthread_join(tid2, NULL);
        printf("second thread exited\n");
    exit(0);
*/
}//end main

void printBoilerPlate(pthread_t me, char* message)  //Gets the print message, also locks the log counter
{

    int hours, minutes, seconds, day, month, year;
    time_t now;
    time(&now);
    char am[] = "am";
    char pm[] = "pm";
    struct tm *local = localtime(&now);
    seconds = local->tm_sec;
    minutes = local->tm_min;
    hours = local->tm_hour;
    day = local->tm_mday;
    month = local->tm_mon + 1;
    year = local->tm_year + 1900;
    char* nd;
    nd = am;
    if(hours <12)
    {
        nd = pm;
    }
    pthread_mutex_lock(&tlock1);
    logindex++;
    printf("Logindex %d, thread %ld (p=%p), PID %d, ", logindex, (long)me, p, getpid());
    printf("%02d:%02d:%02d %02d:%02d:%02d %s: ",day, month, year, hours, minutes,seconds, nd);
    printf("%s \n", message);
    pthread_mutex_unlock(&tlock1);
}
/**********************************************************************
// function thread_runner runs inside each thread --------------------------------------------------
**********************************************************************/
void* thread_runner(void* x)
{
    signal(SIGINT, sig_interrupt_handler);
    pthread_t me;
    me = pthread_self();
    printBoilerPlate(me, "ENTERED THREAD RUNNER");

    pthread_mutex_lock(&tlock2); // critical section starts
    if (p==NULL)
    {
        p = (THREADDATA *) malloc(sizeof(THREADDATA));
        p->creator = me;
        printBoilerPlate(me, "INITIALIZED THREADDATA");
    }
    pthread_mutex_unlock(&tlock2);  // critical section ends
    char input[25];
    if(p->creator == me)
    {
        while(runner)
        {

            fgets(input, 20, stdin);
            if(feof(stdin))
            {
                runner = 0;
            }
            else if(strcmp(input, "\n") == 0)
            {
                runner = 0;
            }
            else
            {  //Indented to more easily see what is between the lock
                pthread_mutex_lock(&tlock3); //locking before inserting into the linked list
                    insertIntoList(input);
                    readok = 1;//tell the other thread thread there is a new value to read
                    printBoilerPlate(me, "CREATED AND INSERTED NODE");
                pthread_mutex_unlock(&tlock3);//unlock!

            }
        }
    }
    else
    {
        while(runner)
        {
            if(readok && runner)
            {
                char ok[100] = "HEAD OF LINKED LIST CONTAINS LINE: ";
                pthread_mutex_lock(&tlock3); //lock before reading the Node head
                if(runner)
                {
                    readok = 0;//Set flag to zero, so thread y knows there are no new values
                    strcat(ok, getHead()); //concatenate message and result
                    ok[strlen(ok)-1] = '\0'; //get rid of that peasky newline character
                    printBoilerPlate(me, ok); //print out the message with the head node value
                    pthread_mutex_unlock(&tlock3); //all done!

                }
            }
        }
    }
    if(p->creator == me)
    {
        pthread_mutex_lock(&tlock3);
        deallocateList();
        pthread_mutex_unlock(&tlock3);
        printBoilerPlate(me, "DEALLOCATED LINKED LIST");
        printf("runner");

    }
    printBoilerPlate(me, "EXITING THREAD RUNNER");
    return NULL;
}//end thread_runner