//
// Created by projector on 10/13/22.
//
#include <iostream>
#include <string.h>
#include <zip.h>
#include <map>
#include <vector>

#include <string>
#include "ZipRead.h"

extern "C"{
#include "pd_uthread.h"
#include "pd_log.h"
}

using namespace std;
#define LVAR(var)   " " << #var << "="<< var
#define LOG_COUT cout << __FILE__ << ":" << __LINE__ <<  " at " << __FUNCTION__ << " "
#define LOG_ENDL_ERR      LVAR(errno) << " err=" << strerror(errno) << endl
#define LOG_ENDL " " << endl;

struct buffer_t {
    char *data;
    size_t size;
};


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


int main(int argc, char **argv) {

    zip_t *zip = zip_open(argv[1], 0, 'r');
    if (zip == NULL) {
        LOG_COUT << "err" << LOG_ENDL;
        return -1;
    }
    int n = zip_entries_total(zip);
    for (int i = 0; i < n; ++i) {
        zip_entry_openbyindex(zip, i);
        {
            const char *name = zip_entry_name(zip);
            int isdir = zip_entry_isdir(zip);
            unsigned long long size = zip_entry_size(zip);
            unsigned int crc32 = zip_entry_crc32(zip);
            LOG_COUT << LVAR(name) << LVAR(isdir) << LVAR(size) << LVAR(zip_entry_comp_size(zip)) << LVAR(crc32)
                     << LVAR(zip_entry_comp_size(zip) * 1.0 / size) << LOG_ENDL;
        }
        zip_entry_close(zip);
    }
    struct buffer_t buf;

    buf.data = NULL;
    buf.size = 0;

    memset((void *) &buf, 0, sizeof(struct buffer_t));

    int ret = zip_entry_open(zip, argv[2]);
    LOG_COUT << LVAR(ret) << LOG_ENDL;

    ZipInfo *zipInfo  = new ZipInfo();
    zipInfo->zip = zip;
    zipInfo->fileName = "data/1.txt";


    const int64_t uthread_num = 1;
    struct PdUThreadMgr *uthread_mgr;
    struct PdUThreadNode *uthread_node[uthread_num];

    int64_t i = 0;
    uthread_mgr = pd_uthread_init_mgr();
    for (i = 0; i < uthread_num; i++)
    {
        uthread_node[i] = pd_uthread_init_node(uthread_mgr, uthread_func, (void*)zipInfo);
    }
    zipInfo->req_size = 1;
//  pd_uthread_schedule(uthread_mgr);
    for (int j = 0; j < 8; ++j) {
        //set args
        LOG_COUT << "begin" << LVAR(zipInfo->req_offset) << LVAR(zipInfo->req_size) << LOG_ENDL;
        zipInfo->req_offset = 0;

        int ret = pd_uthread_scheduleII(uthread_mgr);
        //get result
        LOG_COUT << "end" << LVAR(zipInfo->req_offset) << LVAR(zipInfo->req_size) << LVAR(zipInfo->data_len) << LVAR(zipInfo->cur_read_offset) << LOG_ENDL;
        zipInfo->req_size = zipInfo->cur_read_offset+1;
        if (ret == 0) {
            break;
        }

    }
    for (int j = 0; j < 8; ++j) {
        //set args
        LOG_COUT << "begin" << LVAR(zipInfo->req_offset) << LVAR(zipInfo->req_size) << LOG_ENDL;
        zipInfo->req_offset = 0;

        int ret = pd_uthread_scheduleII(uthread_mgr);
        //get result
        LOG_COUT << "end" << LVAR(zipInfo->req_offset) << LVAR(zipInfo->req_size) << LVAR(zipInfo->data_len) << LVAR(zipInfo->cur_read_offset) << LOG_ENDL;
        zipInfo->req_size = zipInfo->cur_read_offset-1;
        if (ret == 0) {
            break;
        }

    }


    for (i = 0; i < uthread_num; i++)
    {
        pd_uthread_destroy_node(uthread_node[i]);
    }
    pd_uthread_destroy_mgr(uthread_mgr);


}