//
// Created by projector on 10/13/22.
//
#include <iostream>
#include <string.h>
#include <zip.h>
#include <map>
#include <vector>

#include <string>
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

    struct buffer_t *buf = (struct buffer_t *)arg;

    LOG_COUT << LVAR(offset) << LVAR(size) << LOG_ENDL;

    buf->data = static_cast<char *>(realloc(buf->data, buf->size + size + 1));

    memcpy(&(buf->data[buf->size]), data, size);
    buf->size += size;
    buf->data[buf->size] = 0;

    return size;
}


int main(int argc, char **argv) {

    zip_t *zip = zip_open("test/data.zip", 0, 'r');
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
                     << LVAR(zip_entry_comp_size(zip)*1.0/size)<< LOG_ENDL;
        }
    }
    struct buffer_t buf;

    buf.data = NULL;
    buf.size = 0;

    memset((void *)&buf, 0, sizeof(struct buffer_t));

    int ret = zip_entry_open(zip, "data/1.txt");
    LOG_COUT << LVAR(ret) << LOG_ENDL;
    ret = zip_entry_extract(zip, reinterpret_cast<size_t (*)(void *, uint64_t, const void *, size_t)>(on_extract), &buf);
    LOG_COUT << LVAR(ret) << LOG_ENDL;

}