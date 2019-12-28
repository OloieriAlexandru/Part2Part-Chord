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
    node            fingers[HASH_BITS];
};

struct nodeInfo {
    fingersTable    fTable;
    node            me;
};

struct sharedFileInfo {
    std::string name;
    std::string path;
    uint        hsh;
    sharedFileInfo(const std::string& fileName, const std::string& filePath, uint h = 0){
        name = fileName;
        path = filePath;
        hsh = h;
    }
};

#endif // CHORD_H
