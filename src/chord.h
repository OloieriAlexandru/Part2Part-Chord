#ifndef CHORD_H
#define CHORD_H

#include <unistd.h>

#include <string>

#include "constants.h"

#define     uint    unsigned int

struct node {
    uint            key;
    uint            port;
    std::string     address;
};

struct fingersTable {
    node            predecessor;
    node            fingers[SHA_HASH_BITS];
};

struct nodeInterval{

};

struct nodeInfo {
    fingersTable    fTable;
    node            me;
};

extern nodeInfo info;

struct sharedFileInfo {
    std::string name;
    std::string path;
    uint        shaHash, customHash;
    sharedFileInfo(const std::string& fileName, const std::string& filePath, uint h1 = 0, uint h2 = 0){
        name = fileName;
        path = filePath;
        customHash = h1;
        shaHash = h2;
    }
};

bool    between(uint id, uint l, uint r);
node    closestPrecedingFinger(uint id);

bool    sendNodeInfo(int sd, const uint key, const uint port, const std::string& address);
bool    sendNodeInfo(int sd, const node& nd);
bool    readNodeInfo(int sd, uint& key, uint& port, std::string& address);
bool    readNodeInfo(int sd, node& nd);

#endif // CHORD_H
