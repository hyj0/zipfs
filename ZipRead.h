//
// Created by projector on 10/14/22.
//

#ifndef ZIPFS_ZIPREAD_H
#define ZIPFS_ZIPREAD_H

#include <string>
#include <iostream>
#include "zip.h"

using namespace std;
#define LVAR(var)   " " << #var << "="<< var
#define LOG_COUT cout << __FILE__ << ":" << __LINE__ <<  " at " << __FUNCTION__ << " "
#define LOG_ENDL_ERR      LVAR(errno) << " err=" << strerror(errno) << endl
#define LOG_ENDL " " << endl;

class MutexLock{
public:
    MutexLock(pthread_mutex_t *mutex) {
        this->mutex = mutex;
        pthread_mutex_lock(mutex);
    }
    ~MutexLock() {
        pthread_mutex_unlock(mutex);
    }

private:
    pthread_mutex_t *mutex;
};

class ZipInfo {
public:
    ZipInfo(){
        req_offset = 0;
        req_size = 0;
        cur_read_offset = 0;
        pData = NULL;
        data_len = 0;
        uthread_node = NULL;
        zip = NULL;
    }
    zip_t *zip;
    int req_offset;
    int req_size;
    int cur_read_offset; //µ±Ç°ÒÑ¶Á
    char *pData;
    int data_len;
    std::string fileName;
    struct PdUThreadNode *uthread_node;
};



class ZipRead {
public:
    ZipRead(const string &zipfile, const string &datafile, long fileSize) : zipfile(zipfile), datafile(datafile), fileSize(fileSize) {
        stMutex = PTHREAD_MUTEX_INITIALIZER;
        zipInfo = NULL;
        uthread_mgr = NULL;
        uthread_node = NULL;
    }

    long read(void *buff, unsigned long offset, unsigned long size);

private:
    string zipfile;
    string datafile;
    long fileSize;
    ZipInfo *zipInfo;
    pthread_mutex_t stMutex;
    struct PdUThreadMgr *uthread_mgr;
    struct PdUThreadNode *uthread_node;
};


#endif //ZIPFS_ZIPREAD_H
