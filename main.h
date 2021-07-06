#ifndef GLOBALH
#define GLOBALH

#define _GNU_SOURCE
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define CONANS 5
#define LIBRARIANS 3
#define UNIFORMS 3
#define WASHING_MACHINES 2

#define TRUE 1
#define FALSE 0

#define LIB_REQS_PROB 50
#define STATE_CHANGE_PROB 50
#define SEC_IN_STATE 2

#define ROOT 0

// message types
#define REQ 1
#define REQ_UNIFORM 2
#define REQ_LAUNDRY 3
#define ACK 4
#define ACK_UNIFORM 5
#define ACK_LAUNDRY 6
#define NEG 7
#define RELEASE 8
#define RELEASE_UNIFORM 9

typedef struct {
    int ts;
    int src;
    int jobId;
    int libRank;
    int conans[CONANS];
} packet_t;

extern MPI_Datatype MPI_PACKET_T;
extern int rank, size;

typedef enum {
    InLookingForConan,
    InWaitingForBooks,
    InLibrarianJobDone
} librarian_state;

typedef enum {
    InLookingForJob,
    InGettingUniform,
    InPursuit,
    InConanJobDone
} conan_state;

extern librarian_state lib_state;
extern conan_state con_state;

extern pthread_mutex_t lamportMutex;

struct libQueueElement
{
    int rank;
    int lamportTimer;
    int tag;
};
extern struct libQueueElement* libQueue;
extern int libQueueLength;
extern int libQueueReceivedMessagesNumber;
extern int employeeRank;

extern int lamportTimer;
extern int jobId;

#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [tid %d][ts %d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamportTimer, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

#define P_WHITE printf("%c[%d;%dm",27,1,37);
#define P_BLACK printf("%c[%d;%dm",27,1,30);
#define P_RED printf("%c[%d;%dm",27,1,31);
#define P_GREEN printf("%c[%d;%dm",27,1,33);
#define P_BLUE printf("%c[%d;%dm",27,1,34);
#define P_MAGENTA printf("%c[%d;%dm",27,1,35);
#define P_CYAN printf("%c[%d;%d;%dm",27,1,36);
#define P_SET(X) printf("%c[%d;%dm",27,1,31+(6+X)%7);
#define P_CLR printf("%c[%d;%dm",27,0,37);

#define println(FORMAT, ...) printf("%c[%d;%dm [tid %d][ts %d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamportTimer, ##__VA_ARGS__, 27,0,37);

void init(int*, char***);
void waitInLibrarianState(librarian_state);
void changeLibrarianState(librarian_state);

void waitInConanState(conan_state);
void changeConanState(conan_state);

void sendPacket(packet_t*, int, int);

#endif //GLOBALH