#include "main.h"
#include "conan.h"
#include "librarian.h"

#include <pthread.h>

librarian_state lib_state = InLookingForConan;
conan_state con_state = InLookingForJob;

int rank, size;

MPI_Datatype MPI_PACKET_T;
pthread_t threadLib, threadCon;

pthread_mutex_t stateMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waitingCond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t lamportMutex = PTHREAD_MUTEX_INITIALIZER;

struct libQueueElement* libQueue;
int libQueueLength = 0;
int libQueueReceivedMessagesNumber = 0;
int employeeRank = -1;
int lamportTimer = 0;
int jobId = -1;

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
            /* Nie ma co, trzeba wychodzić */
	    fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	    break;
        case MPI_THREAD_MULTIPLE: printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
	    break;
        default: printf("Nikt nic nie wie\n");
    }
}

void init(int* argc, char*** argv) {
    int provided;
    MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);
    const int nitems=5;
    int blocklengths[5] = {1,1,1,1,CONANS};
    MPI_Datatype types[5] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint     offsets[5]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, jobId);
    offsets[3] = offsetof(packet_t, libRank);
    offsets[4] = offsetof(packet_t, conans);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_PACKET_T);
    MPI_Type_commit(&MPI_PACKET_T);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(rank);

    if (rank < CONANS) {
        pthread_create( &threadCon, NULL, comThreadConan, 0 );
        debug("I'm Conan %d (process %d)", rank, rank);
    } else {
        pthread_create( &threadLib, NULL, comThreadLibrarian, 0 );
        debug("I'm Librarian %d (process %d)", rank-CONANS, rank);
    }
}

void finish() {

    MPI_Type_free(&MPI_PACKET_T);
    MPI_Finalize();
}

void waitInLibrarianState(librarian_state state) {
    pthread_mutex_lock(&stateMutex);
    if (state == lib_state)
        pthread_cond_wait(&waitingCond, &stateMutex);
    pthread_mutex_unlock(&stateMutex);
}

void changeLibrarianState(librarian_state state) {
    pthread_mutex_lock(&stateMutex);
    lib_state = state;
    pthread_mutex_unlock(&stateMutex);
    pthread_cond_signal(&waitingCond);
}

void waitInConanState(conan_state state) {
    pthread_mutex_lock(&stateMutex);
    if (state == con_state)
        pthread_cond_wait(&waitingCond, &stateMutex);
    pthread_mutex_unlock(&stateMutex);
}

void changeConanState(conan_state state) {
    pthread_mutex_lock(&stateMutex);
    con_state = state;
    pthread_mutex_unlock(&stateMutex);
    pthread_cond_signal(&waitingCond);
}

void sendPacket(packet_t *pkt, int destination, int tag)
{
    int freepkt=0;
    if (pkt==0) {
        pkt = malloc(sizeof(packet_t));
        freepkt = 1;
    }
    MPI_Send( pkt, 1, MPI_PACKET_T, destination, tag, MPI_COMM_WORLD);
    if (freepkt)
        free(pkt);
}

int main(int argc, char ** argv) {
    init(&argc, &argv);

    if (rank < CONANS) {
        mainLoopConan();
    } else {
        mainLoopLibrarian();
    }

    finish();
    return 0;
}