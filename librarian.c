#include "main.h"
#include "librarian.h"

void libQueueReceived(int src, int ts, int tag) {
    for (int i=0; i<libQueueLength; i++) {
        if (libQueue[i].rank == src) {
            libQueue[i].lamportTimer = ts;
            libQueue[i].tag = tag;
        }
    }
    libQueueReceivedMessagesNumber++;
}

int pickConan() {
    int pickedConanRank = -1;
    int lowestTs = -1; // biggest priority
    for (int i=0; i<libQueueLength; i++) {
        if (libQueue[i].tag == ACK) {
            if (lowestTs == -1 || libQueue[i].lamportTimer < lowestTs) {
                lowestTs = libQueue[i].lamportTimer;
                pickedConanRank = libQueue[i].rank;
            } 
        }
    }
    return pickedConanRank;
}

void *comThreadLibrarian(void *ptr) {
    MPI_Status status;
    packet_t packet;
    while(1) {
        MPI_Recv(&packet, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch (status.MPI_TAG) {
            case ACK:
            case NEG:
                if (lib_state != InLookingForConan) continue;
                if (packet.jobId != jobId) continue;
                libQueueReceived(packet.src, packet.ts, status.MPI_TAG);
                if (libQueueReceivedMessagesNumber >= libQueueLength) {
                    employeeRank = pickConan();
                    if (employeeRank != -1) {
                        debug("Picked Conan: %d", employeeRank);
                        changeLibrarianState(InWaitingForBooks);
                    }
                }
            break;
            case RELEASE:
                changeLibrarianState(InLibrarianJobDone);
            break;
            default:
            break;
        }

    }
}

void mainLoopLibrarian() {
    srandom(rank);
    packet_t* pkt;
    jobId = rank;
    while(1) {
        switch(lib_state) {
            case InLookingForConan:
                debug("Sending new request..");
                pkt = malloc(sizeof(packet_t));
                // sleeping
                sleep(SEC_IN_STATE);
                // sending errand to random conans
                int conans_array[CONANS];
                memset(conans_array, 0, CONANS);
                int packetsToSend = 0;
                employeeRank = -1;
                // selecting random conans
                for(int i=0; i<CONANS; i++) {
                    if (random()%100 < LIB_REQS_PROB) {
                        conans_array[i] = 1;
                        packetsToSend++;
                    }
                }
                if (packetsToSend == 0) {
                    int rand = random() % CONANS;
                    conans_array[rand] = 1;
                    packetsToSend++;
                }
                libQueueLength = packetsToSend;
                libQueue = malloc(sizeof(struct libQueueElement)*libQueueLength);
                libQueueReceivedMessagesNumber = 0;
                int j = 0;
                for (int i=0; i<CONANS; i++) {
                    if (conans_array[i] == 1) {
                        libQueue[j].rank = i;
                        j++;
                    }
                }
                pthread_mutex_lock(&lamportMutex);
                lamportTimer++;
                pthread_mutex_unlock(&lamportMutex);
                pkt->src = rank;
                pkt->ts = lamportTimer;
                pkt->jobId = jobId;
                for (int i=0; i<CONANS; i++) {
                    pkt->conans[i] = conans_array[i];
                }
                for (int i=0; i<CONANS; i++) {
                    if (conans_array[i] == 1) {
                        sendPacket(pkt, i, REQ);
                    }
                }
                debug("Waiting for applications");
                free(pkt);
                waitInLibrarianState(InLookingForConan);
            break;
            case InWaitingForBooks:
                debug("Waiting for Conan %d to finish work", employeeRank);
                waitInLibrarianState(InWaitingForBooks);
            break;
            case InLibrarianJobDone:
                debug("Cycle finished. Everything is done");
                free(libQueue);
                // sleeping
                sleep(SEC_IN_STATE);

                //finishing
                changeLibrarianState(InLookingForConan);
                jobId += LIBRARIANS;
            break;
            default:
            break;
        }
    }
}