#include <stdio.h>
#include <stdlib.h>
#include <zconf.h>

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        exit(0);
    }
    int fd_total[2];
    int fd_count[2];
    int index = 0;
    long int aggregate_count = 0;
    pid_t p=1;
    long double aggregate_total = 0;
    for (int i = 1; i < argc; i++)  //each loop produces a fork.  child process reads a file passes result back to parent
    {
        if (p > 0)
        {
            if (pipe(fd_total)==-1 || pipe(fd_count)==-1)
            {
                fprintf(stderr, "Pipe Failed" );
                return 1;
            }
            p = fork();
            index++;
            if(p == 0)
            {
                FILE *fp = fopen(argv[index], "r"); //read file
                if (fp == NULL) {
                    printf("avg: cannot open file\n");
                    exit(1);
                }
                fseek (fp, 0, SEEK_END); //check if file is empty or not
                long size = ftell(fp);
                long double total = 0;
                long int count = 0;
                if (0 == size) //if it is empty count and total are 0
                {
                   count = 0;
                   total = 0;
                }
                else {
                    fseek(fp, 0, SEEK_SET); //reset file pointer
                    char fileInt[1000];
                    char *convert;
                    while (!feof(fp)) { //read contents of file
                        fscanf(fp, "%s", fileInt);
                        double val = strtod(fileInt, &convert);
                        total += val;
                        count++;
                    }
                }
                fclose(fp);
                close(fd_total[0]);  //close reads
                close(fd_count[0]);
                write(fd_count[1], &count, sizeof(count)); //write to pipe
                write(fd_total[1], &total, sizeof(total));
                close(fd_total[1]); //close writes
                close(fd_count[1]);
                exit(0);  //child process exits so it does not do loop unnecessarily
            }
            wait(NULL);  //wait for child process to finish
            long double temp1 = 0;
            long int temp_count = 0;
            close(fd_total[1]);  //read child processes results
            close(fd_count[1]);
            read(fd_total[0], &temp1, sizeof(temp1));
            close(fd_total[0]);
            aggregate_total+=temp1;  //add results to total
            read(fd_count[0], &temp_count, sizeof(temp_count));
            aggregate_count+=temp_count;
            close(fd_total[0]);
        }
    }
    long double final = 0;
    if(aggregate_total > 0) {
       final = aggregate_total / aggregate_count;//compute average if not 0
    }
    printf("%Lf", final);
     return 0;
}



