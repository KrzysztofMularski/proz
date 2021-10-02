#ifndef CONAN_H
#define CONAN_H

extern pthread_mutex_t workPlacesMutex;

struct workPlace {
    int libRank;
    int jobId;
    enum {unavailable, recruiting, currentJob} status;
    int req_timestamp;
    int conans[CONANS];
    struct {
        enum {null, ack, neg} res_status; //null - not yet delivered
        int res_timestamp;
    } responses[CONANS];
};

enum {status_null, status_ACK_sent, status_currentJob} jobStatus;

struct workPlace workPlaces[LIBRARIANS];

void changeWorkStatus(int, int, int, int, int[]);
void newResponseHandling(int, int, int, int, int);

void *comThreadConan(void *ptr);
void mainLoopConan();

#endif // CONAN_H