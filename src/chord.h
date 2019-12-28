#ifndef CHORD_H
#define CHORD_H

#include <string>

#include "constants.h"

#define     uint    unsigned int

struct node {
    int             key;
    uint            port;
    std::string     address;
};

struct fingersTable {
    node            predecessor;
    node            fingers[SHA_HASH_BITS];
};

struct nodeInfo {
    fingersTable    fTable;
    node            me;
};

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

#endif // CHORD_H
