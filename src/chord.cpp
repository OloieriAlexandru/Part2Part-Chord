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

std::ostream& operator<<(std::ostream& out, const sharedFileInfo& file){
    out<<"Name: "<<file.name<<", path: "<<file.path<<", SHA1: "<<file.shaHash<<", b131: "<<file.customHash;
    return out;
}

bool operator ==(const chordFileInfo& f1, const chordFileInfo& f2) {
    return (f1.id == f2.id && f1.port == f2.port && f1.address == f2.address && f1.name == f2.name);    
}

bool operator !=(const chordFileInfo& f1, const chordFileInfo& f2) {
    return !(f1 == f2);
}

std::ostream& operator<<(std::ostream& out, const chordFileInfo& file) {
    out << "Name: " << file.name << ", chordId: " << file.chordId  << ", id: " << file.id << ", ownerAddress: " << file.address << ", ownerPort: " << file.port;
    return out;
}

void printThisChordNodeInfo() {
    printf("Chord node %u\n", info.me.key);
    printf("Successor: %u %u %s\n", info.fTable.fingers[0].key, info.fTable.fingers[0].port, info.fTable.fingers[0].address.c_str());
    printf("Predecessor: %u %u %s\n", info.fTable.predecessor.key, info.fTable.predecessor.port, info.fTable.predecessor.address.c_str());
    printf("Fingers table:\n");
    for (int i=0;i<SHA_HASH_BITS;++i){
        printf("%d. %u %u %s [%d, %d)\n", i+1, info.fTable.fingers[i].key, info.fTable.fingers[i].port, info.fTable.fingers[i].address.c_str(),
            info.intervals[i].start, info.intervals[i].end+1);
    }
}

void printThisChordNodeFiles() {
    int index = 1;
    for (auto& bucket : chordFiles){
        for (auto& file : bucket.second){
            std::cout<<(index++)<<". "<<file<<'\n';
        }
    }
    if (index == 1){
        printf("This node isn't responsible for any files!\n");
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
        if (between(info.fTable.fingers[i].key, info.me.key+1, id-1)){
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

bool sendChordFileInfo(int sd, const std::string& fileName, const uint chordFileId, const uint fileId, const std::string& ownerAddress, const uint ownerPort) {
    uint len = fileName.size();
    if (-1 == write(sd,&len,4) || -1 == write(sd,fileName.c_str(),len) || -1 == write(sd,&chordFileId,4) || -1 == write(sd,&fileId,4)) {
        printf("[server]Error when sending info about a shared file to a peer!\n");
        return false;
    }
    len = ownerAddress.size();
    if (-1 == write(sd,&len,4) || -1 == write(sd,ownerAddress.c_str(),len) || -1 == write(sd,&ownerPort,4)) {
        printf("[server]Error when sending info about the owner of a shared file to a peer!\n");
        return false;
    }
    return true;
}

bool sendChordFileInfo(int sd, const sharedFileInfo& file) {
    return sendChordFileInfo(sd, file.name, file.shaHash, file.customHash, info.me.address, info.me.port);
}

bool readChordFileInfo(int sd, std::string& fileName, uint& chordFileId, uint& fileId, std::string& ownerAddress, uint& ownerPort) {
    uint len;
    if (-1 == read(sd,&len,4)){
        printf("[server]Error when reading the length of the name of a shared file!\n");
        return false;
    }
    fileName.resize(len);
    for (int i=0;i<len;++i){
        if (-1 == read(sd,&fileName[i],1)){
            printf("[server]Error when reading the %d th byte of the name of a shared file!\n",i);
            return false;
        }
    }
    if (-1 == read(sd,&chordFileId,4) || -1 == read(sd,&fileId,4)){
        printf("[server]Error when reading the ids of a shared file!\n");
        return false;
    }
    len = ownerAddress.size();
    if (-1 == read(sd,&len,4)){
        printf("[server]Error when reading the length of the address of a peer that shared a file!\n");
        return false;
    }
    ownerAddress.resize(len);
    for (int i=0;i<len;++i){
        if (-1 == read(sd,&ownerAddress[i],1)){
            printf("[server]Error when reading the %d th byte of the address of a peer that shared a file!\n",i);
            return false;
        }
    }
    if (-1 == read(sd,&ownerPort,4)){
        printf("[server]Error when reading the port of a peer that shared a file!\n");
        return false;
    }
    return true;
}

bool readChordFileInfo(int sd, chordFileInfo& fileInfo) {
    return readChordFileInfo(sd, fileInfo.name, fileInfo.chordId, fileInfo.id, fileInfo.address, fileInfo.port);
}

bool getNodesInClockwiseOrder(std::vector<int>& clock) {
    clock.clear();
    int mine = info.me.key, currId = mine + 1, steps = 1;
    clock.push_back(mine);
    node currentNode = serverFindSucc(currId);
    while (currentNode.key != mine && steps < SHA_HASH_VAL){
        clock.push_back(currentNode.key);
        currId = currentNode.key + 1;
        currentNode = serverFindSucc(currId);
        ++steps;
    }
    if (steps == SHA_HASH_VAL){
        return false;
    }
    return true;
}

void printNodesInClockwiseOrder() {
    std::vector<int> clock;
    if (!getNodesInClockwiseOrder(clock)){
        return;
    }
    printf("Chord nodes: ");
    for (int i=0;i<clock.size();++i){
        printf("%d ", clock[i]);
    }
    printf("\n");
}

bool getNodesInCounterClockwiseOrder(std::vector<int>& cclock) {
    cclock.clear();
    int mine = info.me.key, currId = mine, steps = 1;
    cclock.push_back(mine);
    node currentNode = serverFindPred(currId);
    while (currentNode.key != mine && steps < SHA_HASH_VAL){
        cclock.push_back(currentNode.key);
        currId = currentNode.key;
        currentNode = serverFindPred(currId);
        ++steps;
    }
    if (steps == SHA_HASH_VAL){
        return false;
    }
    return true;
}

void printNodesInCounterClockwiseOrder() {
    std::vector<int> cclock;
    if (!getNodesInCounterClockwiseOrder(cclock)){
        return;
    }
    printf("Chord nodes: ");
    for (int i=0;i<cclock.size();++i){
        printf("%d ", cclock[i]);
    }
    printf("\n");
}

void checkSuccPredPointers() {
    std::vector<int> clock, cclock;
    if (!getNodesInClockwiseOrder(clock) || !getNodesInCounterClockwiseOrder(cclock)){
        return;
    }
    std::reverse(cclock.begin()+1, cclock.end());
    if (clock == cclock){
        printf("Success! All successor and predecessor fields are correct in the network!\n");
    } else {
        printf("Error! Not all successor and predecessor fields are correct in the network!\n");
    }
}
