#include "chord.h"

bool operator==(const node& n1, const node& n2){
    return n1.key == n2.key && n1.port == n2.port && n1.address == n2.address;
}

bool operator!=(const node& n1, const node& n2){
    return !(n1 == n2);
}

std::ostream& operator<<(std::ostream& out, const node& nd){
    out << "Key: " << nd.key << ", ip: " << nd.address << ", port: " << nd.port;
    return out;
}

void printThisChordNodeInfo() {
    printf("Chord node %u\n", info.me.key);
    printf("Successor: %u %u %s\n", info.fTable.fingers[0].key, info.fTable.fingers[0].port, info.fTable.fingers[0].address.c_str());
    printf("Predecessor: %u %u %s\n", info.fTable.predecessor.key, info.fTable.predecessor.port, info.fTable.predecessor.address.c_str());
    printf("Fingers table:\n");
    for (int i=0;i<SHA_HASH_BITS;++i){
        printf("%d. %u %u %s\n", i+1, info.fTable.fingers[i].key, info.fTable.fingers[i].port, info.fTable.fingers[i].address.c_str());
    }
}

void initFirstChordNode() {
    for (int i=0;i<SHA_HASH_BITS;++i){
        info.fTable.fingers[i] = info.me;
    }
    info.fTable.predecessor = info.me;
}

void initMyIntervals() {
    for (int i=0;i<SHA_HASH_BITS;++i){
        info.intervals[i].start = (info.me.key + (1u << i)) & SHA_HASH_MOD;
        info.intervals[i].end = (info.me.key + (1u << (i+1))-1) & SHA_HASH_MOD;
    }
}

int normalizeValue(int value) {
    value += SHA_HASH_VAL;
    value &= SHA_HASH_MOD;
    return value;
}

bool between(int id, int l, int r) {
    id = normalizeValue(id);
    l = normalizeValue(l);
    r = normalizeValue(r);

    if (l <= r && l <= id && id <= r){
        return true;
    }
    if (l > r){
        if (id >= l){
            return true;
        } else if(id <= r){
            return true;
        }
    }
    return false;
}

node closestPrecedingFinger(uint id){
    if (info.me.key == info.fTable.fingers[0].key){
        return info.me;
    }

    for (int i=SHA_HASH_BITS-1;i>0;--i){
        if (between(info.fTable.fingers[i].key, info.me.key, id)){
            return info.fTable.fingers[i];
        }
    }

    return info.fTable.fingers[0];
}

bool sendNodeInfo(int sd, const uint key, const uint port, const std::string& address) { 
    if (-1 == write(sd, &key, 4) || -1 == write(sd, &port, 4)){
        printf("[server]Error when sending info about a node to a client!\n");
        return false;
    }
    uint len = address.size();
    if (-1 == write(sd, &len, 4) || -1 == write(sd, address.c_str(), len)){
        printf("[server]Error when sending a node address to a client!\n");
        return false;
    }
    return true;
}

bool sendNodeInfo(int sd, const node& nd) {
    return sendNodeInfo(sd, nd.key, nd.port, nd.address);
}

bool readNodeInfo(int sd, uint& key, uint& port, std::string& address) {
    if (-1 == read(sd, &key, 4) || -1 == read(sd, &port, 4)){
        printf("[server]Error when reading info about a node!\n");
        return false;
    }
    uint len;
    if (-1 == read(sd, &len, 4)){
        printf("[server]Error when reading a node address length!\n");
        return false;
    }
    address.resize(len);
    for (int i=0;i<len;++i){
        if (-1 == read(sd, &address[i], 1)){
            printf("[server]Error when reading a node address byte number %d\n", i+1);
            return false;
        }
    }
    return true;
}

bool readNodeInfo(int sd, node& nd) {
    return readNodeInfo(sd, nd.key, nd.port, nd.address);
}