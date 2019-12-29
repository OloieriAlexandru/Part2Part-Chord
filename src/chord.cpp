#include "chord.h"

bool between(uint id, uint l, uint r) {
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
    for (int i=SHA_HASH_BITS-1;i>=0;--i){
        if (between(info.fTable.fingers[i].key, info.me.key, id)){
            return info.fTable.fingers[i];
        }
    }
    return info.me;
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