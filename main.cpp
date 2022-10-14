#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 31

#include <iostream>
#include <string.h>
#include <fuse3/fuse.h>
#include <zip.h>
#include <map>
#include <vector>

#include <string>
#include "ZipRead.h"

using namespace std;
#define LVAR(var)   " " << #var << "="<< var
#define LOG_COUT cout << __FILE__ << ":" << __LINE__ <<  " at " << __FUNCTION__ << " "
#define LOG_ENDL_ERR      LVAR(errno) << " err=" << strerror(errno) << endl
#define LOG_ENDL " " << endl;

void SplitString(const std::string s, const std::string c, std::vector<std::string> &v) {
    std::string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while (std::string::npos != pos2) {
        string item = s.substr(pos1, pos2 - pos1);
        if (item.length() > 0) {
            v.push_back(item);
        }

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if (pos1 != s.length())
        v.push_back(s.substr(pos1));
}


class Node {
public:
    Node() {
        stMutex = PTHREAD_MUTEX_INITIALIZER;
        zipRead = NULL;
    }
    Node *getNode(const char *path) {
        if (strcmp(path, "/") == 0) {
            return this;
        } else {
            vector<std::string> vPath;
            SplitString(path, "/", vPath);
            Node *cur_node = this;
            for (int i = 0; i < vPath.size(); ++i) {
                string &one_path = vPath[i];
                auto it = cur_node->subNode.find(one_path);
                if (it == cur_node->subNode.end()) {
                    LOG_COUT << "no found" << LVAR(path) << LOG_ENDL;
                    return NULL;
                }
                cur_node = cur_node->subNode[one_path];
            }
            return cur_node;
        }
    }

    string name;
    int isDir;
    long fileSize;
    map<string, Node *> subNode;
    pthread_mutex_t stMutex;
    ZipRead *zipRead;
};


Node g_node;

string strZipFileName;

zip_t *g_pZip = NULL;

static void *zip_init(struct fuse_conn_info *conn,
                      struct fuse_config *cfg) {
    (void) conn;
    cfg->use_ino = 1;
//  cfg->nullpath_ok = 1;
    /* Pick up changes from lower filesystem right away. This is
       also necessary for better hardlink support. When the kernel
       calls the unlink() handler, it does not know the inode of
       the to-be-removed entry and can therefore not invalidate
       the cache of the associated inode - resulting in an
       incorrect st_nlink value being reported for any remaining
       hardlinks to this inode. */
    cfg->entry_timeout = 0;
    cfg->attr_timeout = 0;
    cfg->negative_timeout = 0;


    return NULL;
}

static int zip_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi) {
    (void) fi;
    int res = 0;

    LOG_COUT << LVAR(path) << LOG_ENDL;
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = g_node.subNode.size();
    } else {
        vector<std::string> vPath;
        SplitString(path, "/", vPath);
        Node *cur_node = &g_node;
        for (int i = 0; i < vPath.size(); ++i) {
            string &one_path = vPath[i];
            auto it = cur_node->subNode.find(one_path);
            if (it == cur_node->subNode.end()) {
                LOG_COUT << "no found" << LVAR(path) << LOG_ENDL;
                return -ENOENT;
            }
            cur_node = cur_node->subNode[one_path];
        }
        if (cur_node->isDir) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = cur_node->subNode.size();
        } else {
            stbuf->st_mode = S_IFREG | 0755;
            stbuf->st_nlink = 1;
            stbuf->st_size = cur_node->fileSize;
        }

    }

    return res;
}

static int zip_open(const char *path, struct fuse_file_info *fi) {
    LOG_COUT << LVAR(path) << LOG_ENDL;

    Node *node = g_node.getNode(path);
    if (node == NULL) {
        LOG_COUT << "no found" << LVAR(path) << LOG_ENDL;
        return -ENOENT;
    }

    return 0;
}

static int zip_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    LOG_COUT << LVAR(path) << LVAR(offset) << LVAR(size) << LOG_ENDL;
    Node *node = g_node.getNode(path);
    if (node == NULL) {
        LOG_COUT << "no found" << LVAR(path) << LOG_ENDL;
        return -ENOENT;
    }
    if (node->isDir) {
        LOG_COUT << "no a file" << LVAR(path) << LOG_ENDL;
        return -ENOENT;
    }

    MutexLock lock(&node->stMutex);
    if (node->zipRead == NULL) {
        node->zipRead = new ZipRead(strZipFileName, &path[1], node->fileSize);
    }
    long retLen = node->zipRead->read(buf, offset, size);
    return retLen;

    /*int ret = zip_entry_open(g_pZip, &path[1]);
    if (ret) {
        LOG_COUT << LVAR(ret) << LVAR(path) << LOG_ENDL;
        return -ENOENT;
    }

    int len = 0;
    char *pBuff = new char[node->fileSize + 1];
    len = zip_entry_noallocread(g_pZip, pBuff, node->fileSize);
    if (len != node->fileSize) {
        LOG_COUT << LVAR(len) << LVAR(node->fileSize) << LVAR(path) << LOG_ENDL;
        zip_entry_close(g_pZip);
        return len > 0 ? -len : len;
    }
    int res = 0;
    if (len - offset < size) {
        LOG_COUT << LVAR(len - offset) << LVAR(size) << LVAR(path) << LOG_ENDL;
        res = len - offset;
    } else {
        res = size;
    }
    memcpy(buf, &pBuff[offset], res);
    delete[] pBuff;
    zip_entry_close(g_pZip);
    return res;*/
}


static int zip_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    LOG_COUT << LVAR(path) << LOG_ENDL;

    if (strcmp(path, "/") == 0) {
        Node *cur_node = &g_node;
        auto it = cur_node->subNode.begin();
        for (; it != cur_node->subNode.end(); ++it) {
            filler(buf, it->first.c_str(), NULL, 0, static_cast<fuse_fill_dir_flags>(0));
            LOG_COUT << LVAR(path) << LVAR(it->first.c_str()) << LOG_ENDL;
        }
    } else {
        vector<std::string> vPath;
        SplitString(path, "/", vPath);
        Node *cur_node = &g_node;
        for (int i = 0; i < vPath.size(); ++i) {
            string &one_path = vPath[i];
            auto it = cur_node->subNode.find(one_path);
            if (it == cur_node->subNode.end()) {
                return -ENOENT;
            }
            cur_node = cur_node->subNode[one_path];
        }
        auto it = cur_node->subNode.begin();
        for (; it != cur_node->subNode.end(); ++it) {
            filler(buf, it->first.c_str(), NULL, 0, static_cast<fuse_fill_dir_flags>(0));
            LOG_COUT << LVAR(path) << LVAR(it->first.c_str()) << LOG_ENDL;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    static struct fuse_operations zip_oper;
    memset(&zip_oper, 0, sizeof(zip_oper));
    {
        zip_oper.init = zip_init;
        zip_oper.getattr = zip_getattr;
//        zip_oper.access = zip_access;
//        zip_oper.opendir = zip_opendir;
        zip_oper.readdir = zip_readdir;
//        zip_oper.releasedir = zip_releasedir,
//
        zip_oper.open = zip_open;
        zip_oper.read = zip_read;
//        zip_oper.read_buf = zip_read_buf,
//        zip_oper.write = zip_write,
//        zip_oper.write_buf = zip_write_buf,
//        zip_oper.statfs = zip_statfs,
//        zip_oper.flush = zip_flush,
//        zip_oper.release = zip_release
    };

    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--zipfile") == 0) {
            if (i + 1 == argc - 1) {
                strZipFileName = argv[i + 1];
            } else {
                LOG_COUT << "err args" << LOG_ENDL;
                return -1;
            }
        }
    }
    if (strZipFileName.empty()) {
        LOG_COUT << "err args" << LOG_ENDL;
        return -2;
    }

    g_pZip = zip_open(strZipFileName.c_str(), 0, 'r');
    if (g_pZip == NULL) {
        LOG_COUT << "zip_open err" << LVAR(strZipFileName) << LOG_ENDL;
        exit(-1);
    }

    g_node.name = "/";
    g_node.isDir = 1;

    int n = zip_entries_total(g_pZip);
    for (int i = 0; i < n; ++i) {
        zip_entry_openbyindex(g_pZip, i);
        {
            const char *name = zip_entry_name(g_pZip);
            int isdir = zip_entry_isdir(g_pZip);
            unsigned long long size = zip_entry_size(g_pZip);
            unsigned int crc32 = zip_entry_crc32(g_pZip);
            LOG_COUT << LVAR(name) << LVAR(isdir) << LVAR(size) << LVAR(zip_entry_comp_size(g_pZip)) << LVAR(crc32)
                     << LOG_ENDL;
            vector<std::string> vPath;
            SplitString(name, "/", vPath);
            Node *cur_node = &g_node;
            for (int j = 0; j < vPath.size(); ++j) {
                string &path = vPath[j];
                auto it = cur_node->subNode.find(path);
                if (it == cur_node->subNode.end()) {
                    Node *pnode = new Node();
                    pnode->name = path;
                    pnode->isDir = 1;
                    pnode->fileSize = 0;
                    if (!isdir && j == vPath.size() - 1) {
                        pnode->isDir = 0;
                        pnode->fileSize = size;
                    }
                    cur_node->subNode.insert(make_pair(path, pnode));
                } else { ;
                }
                cur_node = cur_node->subNode[path];
            }
        }
        zip_entry_close(g_pZip);
    }

    return fuse_main(argc - 2, argv, &zip_oper, NULL);
}
