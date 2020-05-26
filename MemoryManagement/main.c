
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
// TRACE_NODE_STRUCT is a linked list of
// pointers to function identifiers
// TRACE_TOP is the head of the list is the top of the stack
//GLOBALS
int defaultWordLength = 5;
int gIndex = 0;

struct MY_NODE_STRUCT
{
    char* functionid;
    int index;
    struct MY_NODE_STRUCT* next;
};
typedef struct MY_NODE_STRUCT MY_NODE;
static MY_NODE* NODE_TOP = NULL;

struct TRACE_NODE_STRUCT
{
    char* functionid; //ptr to function identifier
    struct TRACE_NODE_STRUCT* next; // ptr to next frama
};
typedef struct TRACE_NODE_STRUCT TRACE_NODE;
static TRACE_NODE* TRACE_TOP = NULL; // ptr to the top of the stack
/* --------------------------------*/
/* function PUSH_TRACE */
void PUSH_TRACE(char* p)          // push p on the stack
{
    TRACE_NODE* tnode;
    static char glob[]="global";
    if (TRACE_TOP==NULL) {
        // initialize the stack with "global" identifier
        TRACE_TOP=(TRACE_NODE*) malloc(sizeof(TRACE_NODE));
        // no recovery needed if allocation failed, this is only
        // used in debugging, not in production
        if (TRACE_TOP==NULL) {
            printf("PUSH_TRACE: memory allocation error\n");
            exit(1);
        }
        TRACE_TOP->functionid = glob;
        TRACE_TOP->next=NULL;
    }
    // create the node for p
    tnode = (TRACE_NODE*) malloc(sizeof(TRACE_NODE));
    // no recovery needed if allocation failed, this is only
    // used in debugging, not in production
    if (tnode==NULL) {
        printf("PUSH_TRACE: memory allocation error\n");
        exit(1);
    }
    tnode->functionid=p;
    tnode->next = TRACE_TOP;  // insert fnode as the first in the list
    TRACE_TOP=tnode;          // point TRACE_TOP to the first node
}/*end PUSH_TRACE*/
/* --------------------------------*/
/* function POP_TRACE */
void POP_TRACE()    // remove the op of the stack
{
    TRACE_NODE* tnode;
    tnode = TRACE_TOP;
    TRACE_TOP = tnode->next;
    free(tnode);
}/*end POP_TRACE*/
/* ---------------------------------------------- */
/* function PRINT_TRACE prints out the sequence of function calls that are on the
stack at this instance */
/* For example, it returns a string that looks like: funcA:funcB:funcC.  */
char* PRINT_TRACE()
{
    int depth = 50; //A max of 50 levels in the stack will be combined in a string
    int i, length, j;
    TRACE_NODE* tnode;
    static char buf[100];
    if (TRACE_TOP==NULL)
    {
        // stack not initialized yet, so we are
        strcpy(buf,"global");   // still in the `global' area
        return buf;
    }
    /* peek at the depth top entries on the stack, but do not
     * go over 100 chars and do not go over the bottom of the
     * stack */
    sprintf(buf,"%s",TRACE_TOP->functionid);
    length = strlen(buf);                  // length of the string so far
    for(i=1, tnode=TRACE_TOP->next; tnode!=NULL && i < depth;  i++,tnode=tnode->next)
    {
        j = strlen(tnode->functionid);             // length of what we want to add
        if (length+j+1 < 100) {              // total length is ok
            sprintf(buf+length,":%s",tnode->functionid);
            length += j+1;
        }else                                // it would be too long
            break;
    }
    return buf;
}
// -----------------------------------------
// function REALLOC calls realloc
// TODO REALLOC should also print info about memory usage.// For instance, example of print out:
// "File tracemem.c, line X, function F reallocated the memory segment at address A to a new size S"
// Information about the function F should be printed by printing the stack (use PRINT_TRACE)
void* REALLOC(void* p,int t,char* file,int line)
{
    ///testtt
    printf("File %s, line %d, function=%s, reallocated the memory segment at address %p to a new size %d\n", file, line, PRINT_TRACE(), p, t);
    p = realloc(p, t);
    printf("File %s, line %d, function=%s, reallocated the memory segment at address %p to a new size %d\n", file, line, PRINT_TRACE(), p, t);
    return p;
}
// -------------------------------------------// function MALLOC calls malloc
// TODO MALLOC should also print info about memory usage.// For instance, example of print out:
// "File tracemem.c, line X, function F allocated new memory segment at address A to size S"
// Information about the function F should be printed by printing the stack (use PRINT_TRACE)

void* MALLOC(int t,char* file,int line)
{

    void* p;
    p = malloc(t);
    printf("File %s, line %d, function=%s, allocated new memory segment at address %p to size %d\n", file, line, PRINT_TRACE(), p, t);
    return p;
}

// ----------------------------------------------
// function FREE calls free
// TODO FREE should also print info about memory usage.// For instance, example of print out:// "File tracemem.c, line X, function F deallocated the memory segment at address A"
// Information about the function F should be printed by printing the stack (use PRINT_TRACE)

void FREE(void* p,char* file,int line)
{
    printf("File %s, line %d, function=%s, deallocated the memory segment at address %p\n", file, line, PRINT_TRACE(), p);

    free(p);
}
#define realloc(a,b) REALLOC(a,b,__FILE__,__LINE__)
#define malloc(a) MALLOC(a,__FILE__,__LINE__)
#define free(a) FREE(a,__FILE__,__LINE__)

// -----------------------------------------
//reallocate array
void add_column(char** array,int index, unsigned int columns)
{
    PUSH_TRACE("add_column");
    array[index]=(char*) realloc(array[index],sizeof(char)*(columns+1));
    POP_TRACE();
}
// end add_column

// ------------------------------------------
// function make_extend_array
// Example of how the memory trace is done
// This function is intended to demonstrate how memory usage tracing of malloc and free is done

void make_extend_array(char **array)
{
    PUSH_TRACE("make_extend_array");
    int i, j;
    // and a new column
    add_column(array,4,3);

    // now display the array again
    for(i=0; i<4; i++)
        for(j=0; j<4; j++)
            printf("array[%d][%d]=%d\n",i,j,array[i][j]);

    //now deallocate it
    for(i=0; i<4; i++)
        free((void*)array[i]);
    free((void*)array);


    POP_TRACE();
}//end make_extend_array

// ----------------------------------------------
// function main
char**  setArray(char* pointy, char** arrPointer, int currentSize)
{

    while (pointy != NULL)  //read in a line of the file parse it
    {
        if(gIndex >= currentSize)
        {
            int newSize = currentSize + 1;
            arrPointer = realloc(arrPointer, sizeof(char*) * newSize);
            for(int i=currentSize; i<newSize; i++)
            {
                arrPointer[i] = (char *) malloc(sizeof(char) * defaultWordLength + 1);
            }
            currentSize = newSize;
        }
        if(strlen(pointy) > defaultWordLength+1)
        {
            add_column(arrPointer, gIndex, strlen(pointy));
        }
        strcpy(arrPointer[gIndex], pointy);
        gIndex++;
        pointy = strtok(NULL, "\n"); //increment file pointer
    }
    return arrPointer;
}
char* getFilePointer(char **argv)
{
    FILE *fp = fopen(argv[1], "r"); //create file pointer
    if (fp == NULL) { //check file pointer is valid
        printf("Could not open file %s", argv[1]);
        exit(1);
    }
    fseek(fp, 0, SEEK_END); //check size of file
    long fSize = ftell(fp);
    fseek(fp, 0, SEEK_SET); //reset pointer
    char *fileInput = malloc(fSize + 1);
    fread(fileInput, 1, fSize, fp);
    fileInput[fSize] = 0;
    fclose(fp);
    char *pointy = strtok(fileInput, "\n");
    return pointy;

}
char** allocateArray(int currentMaxSize)
{                                                                                   //what i think im doing
                                                                //0 1 2 3 4 5 6 7 8 9
    char** arrPointer = (char**) malloc(sizeof(char*)*currentMaxSize);  //                0 x x x x x x x x x x
    for(int i=0; i<currentMaxSize; i++)                                             //1 x x x x x x x x x x
    {                                                                               //2 x x x x x x x x x x
        arrPointer[i] = (char *) malloc(sizeof(char) * defaultWordLength + 1);   //3 x x x x x x x x x x
                                                                                    //4 x x x x x x x x x x
                                                                                    //5 x x x x x x x x x x
          // 5 rows for a 5 character string + 1 row for the null character
    }
    return arrPointer;
}
void deallocateArray(char** arr)
{
    for(int i = 0; i < gIndex; i++)
    {
        free(arr[i]);
    }
}
void printArray(char** arr)
{
    for(int i = 0; i < gIndex; i++)
    {
        printf("array[%d]=%s\n",i,arr[i]);
    }
}
void deallocateLinkedList()
{
    MY_NODE* temp = NODE_TOP;
    while(temp != NULL)
    {
        MY_NODE* temp2 = temp;
        temp = temp->next;
        free(temp2);
    }
}
void linkedListInsert(char* pointy)
{
    int index = 0;
    while(pointy != NULL)
    {
        MY_NODE* temp = (MY_NODE *) malloc(sizeof(MY_NODE));
        if (temp == NULL)
        {
            printf("PUSH_TRACE: memory allocation error\n");
            exit(1);
        }
        //For string length: per requirements of the assignment(?) first malloc a set amount.  if it is too small realloc
        char* pTemp = (char*) malloc(sizeof(char)*defaultWordLength+1); //5 characters plus null character
        if(strlen(pointy) > defaultWordLength+1)
        {
            pTemp = realloc(pTemp, strlen(pointy));
        }
        strcpy(pTemp, pointy);
        temp->next = NODE_TOP;
        temp->index = index;
        temp->functionid = pTemp;
        NODE_TOP = temp;
        index++;
        pointy = strtok(NULL, "\n");  //increment file pointer
    }
}
void printNodes(MY_NODE* temp)
{
    if(temp!=NULL)
    {
        printf("Node index: %d, function ID: %s\n", temp->index, temp->functionid);
        printNodes(temp->next);
    }
}
void filestuff()
{
    char fileOut[] = "memtrace.out";
    int standard;
    standard = open(fileOut, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    dup2(standard, 1);
}
void setArray2(char* pointy, char*** arrPointer, int currentSize)
{

    while (pointy != NULL)  //read in a line of the file parse it
    {
        if(gIndex >= currentSize)
        {
            int newSize = currentSize + 1;
            *arrPointer = realloc(*arrPointer, sizeof(char*) * newSize);
            for(int i=currentSize; i<newSize; i++)
            {
                (*arrPointer)[i] = (char *) malloc(sizeof(char) * defaultWordLength + 1);
            }
            currentSize = newSize;
        }
        if(strlen(pointy) > defaultWordLength+1)
        {
            add_column(*arrPointer, gIndex, strlen(pointy));
        }
        strcpy((*arrPointer)[gIndex], pointy);
        gIndex++;
        pointy = strtok(NULL, "\n"); //increment file pointer
    }
}
int main(int argc, char **argv)
{
    if (argc < 2) {  //check input file is valid
        printf("Missing arguments");
        exit(0);
    }
    filestuff();
    int currentMaxSizeArray = 10;

    PUSH_TRACE("main");
    //ARRAY STUFF
    char* arrPointy = getFilePointer(argv);
    char** arrPointer = allocateArray(currentMaxSizeArray);
    setArray2(arrPointy, &arrPointer, currentMaxSizeArray);
    //arrPointer = *pointerToArrPointer;
    //arrPointer = setArray(arrPointy, arrPointer, currentMaxSizeArray);
    printArray(arrPointer);
    deallocateArray(arrPointer);
    free(arrPointy);
    //LINKED LIST STUFF

    char* llPointy = getFilePointer(argv);
    linkedListInsert(llPointy);
    printNodes(NODE_TOP);
    deallocateLinkedList();
    free(llPointy);

    POP_TRACE();
    return(0);
}// end main