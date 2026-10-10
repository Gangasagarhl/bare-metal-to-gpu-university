// dfs.h - DS403 cluster kernel: DFS, a small remote file service (F5-46).
// The message set is modelled on 9P (attach, walk, create, open, read, write, stat, clunk);
// the encoding is our own fixed layout, NOT the 9P wire format (see F5-45).
#pragma once
#include <stdint.h>
#include "msg.h"

enum DfsOp : uint8_t { D_ATTACH = 1, D_WALK, D_CREATE, D_OPEN, D_READ, D_WRITE, D_STAT, D_CLUNK };

struct [[gnu::packed]] DfsT {          // request (client -> server), type M_DFS_T
    uint16_t tag;                      // matches the reply to the request
    uint8_t op;
    uint32_t fid;                      // the client's handle
    uint32_t newfid;                   // D_WALK, D_CREATE: the handle to create
    uint32_t offset;
    uint16_t count;
    char name[28];
    uint8_t data[512];
};

struct [[gnu::packed]] DfsR {          // reply (server -> client), type M_DFS_R
    uint16_t tag;
    uint8_t op;
    int8_t err;                        // 0 = ok, else a negative error number
    uint32_t qpath;                    // which file (unique, never reused)
    uint32_t qversion;                 // grows by one at every change of the file
    uint32_t length;
    uint16_t count;
    uint8_t data[512];
};

enum : int8_t { DE_OK = 0, DE_NOENT = -2, DE_IO = -5, DE_BADF = -9, DE_EXIST = -17,
                DE_NOSPC = -28, DE_TIMEDOUT = -110 };

// Server (one node).
void dfs_server_init();
void dfs_server_handle(const Msg& m);
void dfs_server_dump();                // print the file table

// Client (any node). Each call is a remote procedure call that waits for the reply,
// retrying up to 3 times; while it waits it keeps the membership heartbeats going.
void dfs_client_init(uint8_t server, bool naive_cache);
int8_t dfs_attach(uint32_t fid);
int8_t dfs_walk(uint32_t fid, uint32_t newfid, const char* name, DfsR* out);
int8_t dfs_create(uint32_t dirfid, uint32_t newfid, const char* name);
int dfs_read(uint32_t fid, const char* name, uint32_t offset, char* buf, uint16_t cap);
int dfs_write(uint32_t fid, uint32_t offset, const char* data, uint16_t len);
int8_t dfs_stat(uint32_t fid, DfsR* out);
int8_t dfs_clunk(uint32_t fid);
const char* dfs_error(int8_t e);
