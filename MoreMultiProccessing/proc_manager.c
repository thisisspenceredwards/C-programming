#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>
/*
int createFileStuff(char fileName[100], int inOrErr, int index, char suffix[4])
{

    sprintf(fileName, "%d", index);
    strcat(fileName, suffix);
    int std= open(fileName, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    dup2(std, inOrErr);
    //setbuf(inOrErr, NULL);
    return std;
}
*/
int main(int argc, char **argv) {
    struct timespec start, finish; //initialize containers for holding child process info in lue of linked list
    char commandTable[100][100] = {0};
    char parameterTable[100][100] = {0};
    int indexTable[100] = {0};
    int pidTable[100] = {0};
    struct timespec timeTable[100] = {0};
    if (argc < 2) {  //check input file is valid
        printf("Missing arguments");
        exit(0);
    }
    FILE *fp = fopen(argv[1], "r"); //create file pointer
    if (fp == NULL) { //check file pointer is valid
        printf("Could not open file %s", argv[1]);
        return 1;
    }
    fseek(fp, 0, SEEK_END); //check size of file
    long fSize = ftell(fp);
    fseek(fp, 0, SEEK_SET); //reset pointer
    char *fileInput = malloc(fSize + 1);
    fread(fileInput, 1, fSize, fp);
    fileInput[fSize] = 0;
    fclose(fp);
    int standard;
    int standardErr;
    pid_t parent = getpid();
    char *pointy = strtok(fileInput, "\n");
    int numberOfProcesses = 0;
    while (pointy != NULL && getpid() == parent)  //read in a line of the file parse it
    {
        numberOfProcesses++;
        char *p = pointy;
        char firstString[100] = {0};
        char secondString[100] = {0};
        bool sentinel = false;
        int saveIndexForStartOfSecondWord = 0;
        for (int i = 0; i < strlen(pointy); i++) {
            if (*p == ' ') {
                sentinel = true;
                saveIndexForStartOfSecondWord = i + 1;
            } else {
                if (sentinel == false) {
                    firstString[i] = *p;
                } else {
                    secondString[i - saveIndexForStartOfSecondWord] = *p;
                }
            }
            p++;
        }
        char *add[] = {firstString};  //build arguments for file name
        char *args[] = {*add, secondString, NULL}; //build arguments for execvp
        char fileOut[100];
        char errOut[100];
        sprintf(fileOut, "%d", numberOfProcesses);
        strcat(fileOut, ".out");
        sprintf(errOut, "%d", numberOfProcesses);
        strcat(errOut, ".err");

        standard = open(fileOut, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        standardErr = open(errOut,O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        dup2(standard, 1);
        dup2(standardErr, 2);
        setbuf(stdout, NULL);
        setbuf(stderr, NULL);
        for (int a = 0; a < sizeof(pidTable); a++)
        {
            if (pidTable[a] == 0)
            {
                clock_gettime(CLOCK_MONOTONIC, &start);
                timeTable[a] = start;
                unsigned long max = 0;
                if(strlen(firstString) > strlen(secondString))
                {
                    max = strlen(firstString);
                }
                else
                    max = strlen(secondString);
                for(int q = 0; q< max; q++)
                {
                    indexTable[a] = numberOfProcesses;
                    commandTable[a][q] = firstString[q];
                    parameterTable[a][q] = secondString[q];
                }
                pid_t z = fork();
                if(z > 0)
                {
                    printf("Starting command %d: child %d pid of parent %d\n", numberOfProcesses, z, getpid());
                    fflush(stdout);
                    pidTable[a] = z;
                }
                else if( z == 0)
                {
                    execvp(*add, args);
                    exit(2);
                }
                close(standard);  //close file stuff
                close(standardErr);
                break;
            }
        }
        pointy = strtok(NULL, "\n"); //increment file pointer
    }
    while (numberOfProcesses > 0) {
        int status;
        pid_t p = wait(&status);
        if (p < 0) { //if p is less than 0, no children remain
            return 0;
        }
        for (int i = 0; i < sizeof(pidTable); i++)  //check the returned state of the child process
        {
            if (pidTable[i] == p) {  //find child process in pid array so we get the correct index for the associated arrays
                char fileOut[100]; //remake redirect for stdout and stderr, should have used the method, but i made it after this was written
                char errOut[100];
                sprintf(fileOut, "%d", indexTable[i]);
                strcat(fileOut, ".out");
                sprintf(errOut, "%d", indexTable[i]);
                strcat(errOut, ".err");
                standard = open(fileOut, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                standardErr = open(errOut, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                dup2(standard, 1);
                dup2(standardErr, 2);
                setbuf(stdout, NULL);
                setbuf(stderr, NULL);
                clock_gettime(CLOCK_MONOTONIC, &finish);
                printf("Finished at %ld, runtime duration %ld\n", finish.tv_sec, finish.tv_sec - timeTable[i].tv_sec); //time childprocess took
                long timeStarted = timeTable[i].tv_sec;
                for (int i = 0; i < sizeof(pidTable); i++)
                {
                    if (pidTable[i] == p)
                    {
                        //char fileOut[100];
                        //createFileStuff(fileOut, pidTable[i], indexTable[i], ".err");
                        if (WIFEXITED(status))
                        {
                            if(WEXITSTATUS(status) == 2)
                            {
                                fprintf(stderr, "Unable to execute command: %s ", commandTable[i]);
                            }
                            fprintf(stderr, "Exited with exitcode = %d\n", WEXITSTATUS(status));
                        }
                        else if (WIFSIGNALED(status))
                        {
                            fprintf(stderr, "Killed with signal %d\n", WTERMSIG(status));
                        }
                        break;
                    }
                }
                if (finish.tv_sec - timeStarted > 2) //restart process if it took too long
                {
                    char *params[] = {commandTable[i], parameterTable[i], NULL};
                    pid_t again = fork();
                    if (again > 0)
                    {
                        pidTable[i] = again;  //get values to print out whats happening
                        clock_gettime(CLOCK_MONOTONIC, &start);
                        timeTable[i] = start;
                        printf("Starting command %d: child %d pid of parent %d\n", numberOfProcesses, again, getpid());
                    } else if (again == 0)
                    {
                        execvp(commandTable[i], params); //execute command
                        exit(2); //shouldn't get here
                    }
                    } else
                    {
                        pidTable[i] = 0;  //remove entry in pidtable, so it can be overwritten
                        fprintf(stderr, "Spawning too fast\n");
                        numberOfProcesses--; //yep
                    }
                }
            }
        }
    return 0;
}


