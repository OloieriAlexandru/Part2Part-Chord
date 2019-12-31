#ifndef CHORD_H
#define CHORD_H

#include <unistd.h>

#include <string>
#include <vector>
#include <algorithm>
#include <ostream>

#include "constants.h"

#define     uint    unsigned int

struct node {
    uint            key;
    uint            port;
    std::string     address;
    node() {
        key = 0;
        port = 0;
    }
    friend bool             operator==(const node& n1, const node& n2);
    friend bool             operator!=(const node& n1, const node& n2);
    friend std::ostream&    operator<<(std::ostream& out, const node& nd);
};

struct fingersTable {
    node            predecessor;
    node            fingers[SHA_HASH_BITS];
};

struct nodeInterval{
    int             start, end;
};

struct nodeInfo {
    fingersTable    fTable;
    node            me;
    nodeInterval    intervals[SHA_HASH_BITS];
};

extern nodeInfo info;
extern node     serverFindSucc(uint id);
extern node     serverFindPred(uint id);

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

void    printThisChordNodeInfo();
void    initFirstChordNode();
void    initMyIntervals();

int     normalizeValue(int value);
bool    between(int id, int l, int r);
node    closestPrecedingFinger(uint id);

bool    sendNodeInfo(int sd, const uint key, const uint port, const std::string& address);
bool    sendNodeInfo(int sd, const node& nd);
bool    readNodeInfo(int sd, uint& key, uint& port, std::string& address);
bool    readNodeInfo(int sd, node& nd);

bool    getNodesInClockwiseOrder(std::vector<int>& clock);
void    printNodesInClockwiseOrder();
bool    getNodesInCounterClockwiseOrder(std::vector<int>& cclock);
void    printNodesInCounterClockwiseOrder();
void    checkSuccPredPointers();

#endif // CHORD_H
