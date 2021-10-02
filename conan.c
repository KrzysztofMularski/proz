#include "main.h"
#include "conan.h"

pthread_mutex_t workPlacesMutex = PTHREAD_MUTEX_INITIALIZER;
int bossRank = -1;

void changeWorkStatus(int src, int jobId, int status, int timestamp, int conans[]) {
    pthread_mutex_lock(&workPlacesMutex);
    for (int i=0; i<LIBRARIANS; i++) {
        if (workPlaces[i].libRank == src) {
            workPlaces[i].jobId = jobId;
            workPlaces[i].status = status;
            workPlaces[i].req_timestamp = timestamp;
            for (int j=0; j<CONANS; j++) {
                workPlaces[i].conans[j] = conans[j];
            }
        }
    }
    pthread_mutex_unlock(&workPlacesMutex);
}

void newResponseHandling(int libRank, int conanRank, int jobId, int tag, int timestamp) {
    pthread_mutex_lock(&workPlacesMutex);
    for (int i=0; i<LIBRARIANS; i++) {
        if (workPlaces[i].libRank == libRank) {
            if (jobId < workPlaces[i].jobId)
                return;
            else if (jobId > workPlaces[i].jobId) {
                for (int j=0; j<CONANS; j++) {
                    workPlaces[i].responses[j].res_status = null;
                    workPlaces[i].responses[j].res_timestamp = -1;
                }
                workPlaces[i].jobId = jobId;
            }
            if (tag == ACK) {
                workPlaces[i].responses[conanRank].res_status = ack;
            } else if (tag == NEG) {
                workPlaces[i].responses[conanRank].res_status = neg;
            }
            // todo porównywanie timestamp żądania z timestamp odpowiedzi
            workPlaces[i].responses[conanRank].res_timestamp = timestamp;
            break;
        }
    }
    pthread_mutex_unlock(&workPlacesMutex);
}

int allResArrived(int libRank) {
    pthread_mutex_lock(&workPlacesMutex);
    for (int i=0; i<LIBRARIANS; i++) {
        if (workPlaces[i].libRank == libRank) {
            for (int j=0; j<CONANS; j++) {
                if (workPlaces[i].responses[j].res_status == null) {
                    pthread_mutex_unlock(&workPlacesMutex);
                    return FALSE;
                }
            }
            pthread_mutex_unlock(&workPlacesMutex);
            return TRUE;
        }
    }
    pthread_mutex_unlock(&workPlacesMutex);
}

int findWinnerRank(int libRank) {
    int winnerRank = -1;
    int lowestTs = -1; // biggest priority
    pthread_mutex_lock(&workPlacesMutex);
    for (int i=0; i<LIBRARIANS; i++) {
        if (workPlaces[i].libRank == libRank) {
            debug("67 %d", workPlaces[i].responses[0].res_status);
            // debug("68 %d", libRank)
            for (int j=0; j<CONANS; j++) {
                if (workPlaces[i].responses[j].res_status == ack) {
                    debug("71 %d", lowestTs);
                    debug("72 %d", winnerRank);
                    if (lowestTs == -1 || lowestTs > workPlaces[i].responses[j].res_timestamp || (lowestTs == workPlaces[i].responses[j].res_timestamp && winnerRank > j)) {
                        winnerRank = j;
                        lowestTs = workPlaces[i].responses[j].res_timestamp;
                    }
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(&workPlacesMutex);
    return winnerRank;
}

void *comThreadConan(void *ptr) {
    MPI_Status status;
    packet_t recvPacket;
    for (int i=0; i<LIBRARIANS; i++) {
        // workPlaces[i].status = unavailable;
        workPlaces[i].libRank = CONANS + i;
        workPlaces[i].jobId = -1;
        for (int j=0; j<CONANS; j++) {
            workPlaces[i].conans[j] = -1;
            workPlaces[i].responses[j].res_status = null;
            workPlaces[i].responses[j].res_timestamp = -1;
        }
    }
    while(1) {
        MPI_Recv(&recvPacket, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch(status.MPI_TAG) {
            case REQ: ;// req with job from librarian
                packet_t* toSendPkt = malloc(sizeof(packet_t));
                toSendPkt->jobId = recvPacket.jobId;
                toSendPkt->libRank = recvPacket.src;
                toSendPkt->src = rank;
                // toSendPkt->ts = 
                changeWorkStatus(recvPacket.src, recvPacket.jobId, recruiting, recvPacket.ts, recvPacket.conans);
                int response = -1;
                if (jobStatus == null) {
                    response = ACK;
                    jobStatus = status_ACK_sent;
                } else {
                    response = NEG;
                }
                sendPacket(toSendPkt, recvPacket.src, response);
                for (int i=0; i<CONANS; i++) {
                    if (recvPacket.conans[i] == 1)
                        sendPacket(toSendPkt, i, response);
                }
                free(toSendPkt);
            break;
            case ACK:
            case NEG:
                newResponseHandling(recvPacket.libRank, recvPacket.src, recvPacket.jobId, status.MPI_TAG, recvPacket.ts);
                //debug("%d %d %d", recvPacket.libRank, recvPacket.src, status.MPI_TAG);
                debug("Got Message %d from conan %d, libRank %d", status.MPI_TAG, recvPacket.src, recvPacket.libRank);
                debug("allResArrived: %d", allResArrived(recvPacket.libRank));
                debug("findWinnerRank: %d", findWinnerRank(recvPacket.libRank));
                if (allResArrived(recvPacket.libRank) == TRUE) {
                    if (findWinnerRank(recvPacket.libRank) == rank) {
                        bossRank = recvPacket.libRank;
                        debug("Hired by librarian: %d", bossRank);
                        changeConanState(InGettingUniform);
                    }
                }
            break;
            default:
            break;
        }
    }
    
}

void mainLoopConan() {
    srandom(rank);
    while(1) {
        switch(con_state) {
            case InLookingForJob:
                debug("Looking for Job");
                waitInConanState(InLookingForJob);
                // packet_t *pkt = malloc(sizeof(packet_t));
                // sleeping
                sleep(SEC_IN_STATE);
            break;
            case InGettingUniform:
                sleep(5);    //temp
                changeConanState(InConanJobDone); // temp
                packet_t* toSendPkt = malloc(sizeof(packet_t));//temp
                toSendPkt->src = rank;
                debug("Job Done!");
                sendPacket(toSendPkt, bossRank, RELEASE);//temp
                bossRank = -1;

                free(toSendPkt);//temp
            break;
            default:
            break;
        }
    }
}