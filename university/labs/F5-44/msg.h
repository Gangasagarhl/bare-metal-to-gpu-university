// msg.h - DS403 cluster kernel: cluster messages carried in raw Ethernet frames.
// Frame = destination MAC, source MAC, EtherType 0x88B5, then Hdr, then payload.
// Every frame is sent to the Ethernet broadcast address; Hdr.to says who should act on it.
#pragma once
#include <stdint.h>

enum MsgType : uint8_t {
    M_HEARTBEAT = 1,                 // membership (F5-44)
    M_PS_REQ = 2, M_PS_REP = 3,      // cluster-wide process list (F5-44)
    M_RUN_REQ = 4, M_RUN_REP = 5,    // run a job on another node (F5-44)
    M_DFS_T = 10, M_DFS_R = 11,      // remote file service request / reply (F5-46)
    M_VOTE_REQ = 20, M_VOTE_REP = 21,   // consensus: request vote / vote (F5-47)
    M_APPEND = 22, M_APPEND_REP = 23,   // consensus: append entries / reply (F5-47)
    M_CLIENT = 24, M_CLIENT_REP = 25,   // a client command for the leader (F5-47)
    M_JOB_DONE = 26,                    // scheduler: a job finished (F5-47)
};

struct [[gnu::packed]] Hdr {
    uint8_t magic;       // 'D'
    uint8_t version;     // 1
    uint8_t from;        // sender node id
    uint8_t to;          // receiver node id, 0 = every node
    uint8_t type;        // MsgType
    uint8_t flags;
    uint16_t len;        // payload bytes
    uint32_t term;       // consensus term or membership incarnation (0 if unused)
};

constexpr uint16_t ETHERTYPE_DS403 = 0x88B5;
constexpr int MAX_PAYLOAD = 1400;
constexpr int MAX_NODES = 8;

struct Msg {
    uint8_t from, to, type;
    uint32_t term;
    uint16_t len;
    uint8_t data[MAX_PAYLOAD];
};

bool net_init();                                    // e1000 up; prints the MAC
bool net_send(uint8_t to, uint8_t type, uint32_t term, const void* payload, uint16_t len);
bool net_poll(Msg& m);                              // next message for this node, if any
