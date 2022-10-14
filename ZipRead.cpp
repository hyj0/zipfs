//
// Created by projector on 10/14/22.
//

#include "ZipRead.h"
#include <string>
#include "pd_uthread.h"


static size_t on_extract(void *arg, unsigned long long offset, const void *data,
                         size_t size) {
    ZipInfo *zipInfo = (ZipInfo*)arg;


    LOG_COUT << LVAR(offset) << LVAR(size) << LOG_ENDL;
    if (offset+size > zipInfo->data_len) {
        zipInfo->pData = static_cast<char *>(realloc(zipInfo->pData, offset + size));
        zipInfo->data_len = offset + size;
    }
    memcpy(&zipInfo->pData[offset], data, size);
    zipInfo->cur_read_offset = offset + size;

    while (1) {
        if (offset+size >= zipInfo->req_size+zipInfo->req_offset) {
            pd_uthread_yield(zipInfo->uthread_node, 1);
        } else {
            break;
        }
    }

    return size;
}

void *uthread_func(struct PdUThreadNode *uthread_node, void *arg)
{
    ZipInfo *zipInfo = (ZipInfo*)arg;
    zipInfo->uthread_node = uthread_node;
    LOG_COUT << LVAR(zipInfo) << LOG_ENDL;
//    pd_uthread_yield(uthread_node, 1);

    while (1) {
        if (zipInfo->data_len >= zipInfo->req_offset+zipInfo->req_size) {
            //return
            pd_uthread_yield(uthread_node, 1);
            break;
        }
        if (zipInfo->pData == NULL){
            zipInfo->pData = (char *)malloc(zipInfo->req_size+zipInfo->req_offset);
            zipInfo->data_len = zipInfo->req_offset+zipInfo->req_size;
        }
        int ret = zip_entry_extract(zipInfo->zip, reinterpret_cast<size_t (*)(void *, uint64_t, const void *, size_t)>(on_extract),
                                    zipInfo);
        LOG_COUT << "end" << LVAR(ret) << LOG_ENDL;
        break;
    }
    return NULL;
}

long ZipRead::read(void *buff, unsigned long offset, unsigned long size) {
    MutexLock lock(&stMutex);
    //open if not open
    if (zipInfo == NULL) {
        zipInfo = new ZipInfo();
        zipInfo->zip = zip_open(zipfile.c_str(), 0, 'r');
        if (zipInfo->zip == NULL){

        }
        zipInfo->fileName = datafile;
        int ret = zip_entry_open(zipInfo->zip, datafile.c_str());
        if (ret != 0){

        }
        uthread_mgr = pd_uthread_init_mgr();
        uthread_node = pd_uthread_init_node(uthread_mgr, uthread_func, (void*)zipInfo);
    }

    //check
    if (offset > zipInfo->cur_read_offset) {
        LOG_COUT << LVAR(offset) << LVAR(zipInfo->cur_read_offset) << LOG_ENDL;
        return -1;
    }
    if (zipInfo->cur_read_offset >= offset+size){
        memcpy(buff, &zipInfo->pData[offset], size);
        return size;
    }
    if (zipInfo->cur_read_offset == fileSize) {
        long len = zipInfo->cur_read_offset-offset;
        memcpy(buff, &zipInfo->pData[offset], len);
        return len;
    }
    //uthread
    zipInfo->req_size = size;
    zipInfo->req_offset = offset;
    pd_uthread_scheduleII(uthread_mgr);
    //read
    if (zipInfo->cur_read_offset >= offset+size){
        memcpy(buff, &zipInfo->pData[offset], size);
        return size;
    }
    //has read all
    if (zipInfo->cur_read_offset == fileSize) {
        long len = zipInfo->cur_read_offset-offset;
        memcpy(buff, &zipInfo->pData[offset], len);
        return len;
    }
    LOG_COUT << "bug here" << LVAR(fileSize) << LVAR(zipInfo->cur_read_offset) << LVAR(offset+size) << LOG_ENDL;
    return -2;
}

