#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>

#include "command-line.h"
#include "helper.h"
#include "chord.h"
#include "defines.h"

#define umap std::unordered_map

extern int errno;

node    getFirstNodeInfo();
bool    isFirstChordNode(const std::string& address, uint port);
bool    check(int argc, char* argv[]);
void    shareFilesFromConfigFile();

nodeInfo                                        info;
umap<std::string,std::vector<sharedFileInfo>>   sharedFiles;

void          joinChordNetwork();
void          joinChordNetwork(const node& randomNode);
void          initFingerTable(const node& randomNode);
void          updateOthers();

SHA1          sha1;
uint          getHash(const char* str);

void          shareFile(const std::string& name, const std::string& path, bool atInit = false);
void          shareFile(const cmd::commandResult& command);
void          listSharedFiles();

void          serverSendErrorFlag(int sd);
void          serverDownloadFileLogic(int sd);
void          serverDownloadFileUpload(int sd, sharedFileInfo* sharedFile);
void          serverListFilesToDownloadLogic(int sd);
bool          serverListFilesToDownloadSendFile(int sd, sharedFileInfo* sharedFile);

node          serverFindSucc(const node& nd, uint id);
node          serverFindSucc(uint id);
void          serverFindSuccessor(int sd);
node          serverGetPred(const node& nd);
node          serverFindPred(uint id);
void          serverFindPredecessor(int sd);
node          serverSendSuccessorRequest(int sd);
void          serverSendSuccessor(int sd);
void          serverSendPredecessor(int sd);
void          serverUpdatePredecessorRequest(const node& nd);
void          serverUpdatePredecessor(int sd);
void          serverUpdateFingerTable(int sd);
void          serverUpdateFingerTable(const node& p, const node& nd, uint i);

static void*  serverLogic(void *);
int           createServer();

bool          executeClientCommand(const cmd::commandParser& parser, cmd::commandResult& command);
void          initCmd(cmd::commandParser& parser);

static void*  treat(void *);

void          clientDownloadFileLogic(const cmd::commandResult& command);
void          clientDownloadFileDownload(int sd, const std::string& fileName, uint fileId);
void          clientListFilesToDownloadLogic(const cmd::commandResult& command);

bool          initClient(int& sd, const char* ipAddress, int port);
void          clientLogic();

int main(int argc, char* argv[]) {
  if (!check(argc, argv)){
    return 1;
  }

  if (!configFileInit()) {
    fprintf(stderr, "[main]Error when creating/checking the config file!");
    return 1;
  }

  pthread_t serverThread;
  if (pthread_create(&serverThread, NULL, serverLogic, NULL)){
    perror("[main]Error when creating main thread for the server!");
    return 1;
  }

  shareFilesFromConfigFile();
  joinChordNetwork();

  clientLogic();
  return 0;
};

node getFirstNodeInfo(){
  node res;
  res.address = "127.0.0.1";
  res.port = CHORD_FIRST_PORT;
  return res;
}

bool isFirstChordNode(const std::string& address, uint port){
  return (address == "127.0.0.1" && port == CHORD_FIRST_PORT);
}

bool check(int argc, char* argv[]){
  if (argc != 2){
    fprintf(stderr, "Invalid number of arguments, expected: 1 - the port of this server!\n");
    return false;
  }
  if (!(argv[1][0] >= '0' && argv[1][0] <= '9') || strlen(argv[1]) > 4){
    fprintf(stderr, "Invalid argument, expected: an integer - the port of this server!\n");
    return false;
  }
  info.me.port = 0;
  int digitsNo = 0;
  while (argv[1][digitsNo] && argv[1][digitsNo] >= '0' && argv[1][digitsNo] <= '9'){
    info.me.port = info.me.port * 10 + (argv[1][digitsNo++] - '0');
  }
  info.me.address = std::string("127.0.0.1");
  std::string keyStr = info.me.address + std::to_string(info.me.port);
  info.me.key = getHash(keyStr.c_str());
  return true;
}

void shareFilesFromConfigFile() {
  if (!configFileGetFlagValue()){
    return;
  }
  std::ifstream fileIn(userConfigFilePath);
  if (!fileIn.is_open()){
    printf("Error when opening the config file for reading!\n");
    return;
  }
  int autoAdd;
  std::string name, path;
  fileIn >> autoAdd;
  while (fileIn >> name >> path){
    shareFile(name, path, true);
  }
  fileIn.close();
}

void joinChordNetwork() {
  initMyIntervals();
  if (isFirstChordNode(info.me.address, info.me.port)){
    initFirstChordNode();
  } else {
    node firstNode = getFirstNodeInfo();
    joinChordNetwork(firstNode);
  }
}

void joinChordNetwork(const node& randomNode) {
  initFingerTable(randomNode);
  updateOthers();
}

void initFingerTable(const node& randomNode) {
  int sk;
  node succ = serverFindSucc(randomNode, info.intervals[0].start);
  info.fTable.fingers[0] = succ;
  info.fTable.predecessor = serverGetPred(succ);
  serverUpdatePredecessorRequest(succ);
  for (int i=1;i<SHA_HASH_BITS;++i){
    if (between(info.intervals[i].start, info.me.key, info.fTable.fingers[i-1].key)){
      info.fTable.fingers[i] = info.fTable.fingers[i-1];
    } else {
      info.fTable.fingers[i] = serverFindSucc(randomNode, info.intervals[i].start);
    }
  }
}

void updateOthers() {
  for (int i=0;i<SHA_HASH_BITS;++i){
    int lookFor = normalizeValue((int)info.me.key - (1<<i));
    node p = serverFindPred(lookFor);
    serverUpdateFingerTable(p, info.me, i);
  }
}

uint getHash(const char* str) {
  return getHash(sha1, str);
}

void shareFile(const std::string& name, const std::string& path, bool atInit) {
  if (!fileExists(path.c_str())) {
    if (!atInit){
      std::cout<<"Invalid path! The file doesn't exist!\n";
    } else {
      std::cout<<"Error when loading the file "<<path<<", the file is not located there anymore!\n";
    }
    return;
  }
  sharedFiles[name].push_back(sharedFileInfo(name, path, getCustomHash(path.c_str())));
}

void shareFile(const cmd::commandResult& command) {
  std::string fileName = command.getStringOptionValue("-name");
  std::string filePath = command.getStringOptionValue("-path");
  if (fileName == "none" || filePath == "none"){
      printf("You have to specify the name and the path of the file! (-name and -path options)\n");
      return;
  }
  shareFile(fileName, filePath);
}

void listSharedFiles() {
  int fileNumber = 1;
  printf("Shared files: \n");
  for (auto bucket : sharedFiles){
    for (auto file : bucket.second) {
      printf("%d. name:%s path:%s id:%u\n", fileNumber++, file.name.c_str(), file.path.c_str(), file.customHash);
    }
  }
}

void serverSendErrorFlag(int sd){
  uint response = SRV_ERROR;
  if (-1 == write(sd, &response, 4)){
    printf("[server]Failed to send the error flag to the client!\n");
  }
}

void serverDownloadFileLogic(int sd) {
  uint fileId, fileNameLen, response;
  std::string fileName;
  if (-1 == read(sd, &fileNameLen, 4)){
    printf("[server]Failed to read the length of the file name!\n");
    serverSendErrorFlag(sd);
    return;
  }
  fileName.resize(fileNameLen);
  for (int i=0;i<fileNameLen;++i){
    if (-1 == read(sd, &fileName[i], 1)){
      printf("[server]Failed to read the %d th byte of the file name!\n", i+1);
      serverSendErrorFlag(sd);
      return;
    }
  }
  if (-1 == read(sd, &fileId, 4)){
    printf("[server]Failed to read the file id!\n");
    serverSendErrorFlag(sd);
    return;
  }
  sharedFileInfo* requestedFile = NULL;
  for (auto &file : sharedFiles[fileName]){
    if (file.customHash == fileId){
      requestedFile = &file;
      break;
    }
  }
  response = (requestedFile == NULL ? SRV_DOWNLOAD_FILE_NOT_EXISTS : SRV_DOWNLOAD_FILE_OK_BEGIN);
  if (response == SRV_DOWNLOAD_FILE_OK_BEGIN){
    if (!fileExists(requestedFile->path.c_str())){
      response = SRV_DOWNLOAD_FILE_NOT_AVAILABLE;
    }
  }
  if (-1 == write(sd, &response, 4)){
    printf("[server]Failed to send the response to the client!\n");
    return;
  }
  if (response == SRV_DOWNLOAD_FILE_OK_BEGIN){
    serverDownloadFileUpload(sd, requestedFile);
  }
}

void serverDownloadFileUpload(int sd, sharedFileInfo* sharedFile) {
  char package[PACKAGE_SIZE];
  uint fileSize = getFileSize(sharedFile->path.c_str());
  int fd;
  if (-1 == (fd = open(sharedFile->path.c_str(), O_RDONLY))){
    printf("[server]Failed to open the file to share!\n");
    return;
  }
  uint packages = fileSize / PACKAGE_SIZE + (fileSize % PACKAGE_SIZE ? 1 : 0);
  if (-1 == write(sd, &packages, 4)){
    printf("[server]Failed to sent the number of packages to the client!\n");
    return;
  }
  uint currentPackageSize = PACKAGE_SIZE;
  for (int i=1;i<=packages;++i){
    if (i == packages){
      currentPackageSize = ((fileSize % PACKAGE_SIZE != 0) ? fileSize % PACKAGE_SIZE : PACKAGE_SIZE);
    }
    if (-1 == read(fd, package, currentPackageSize)){
      printf("[server]Failed to read package number %d from the file!\n", i);
      return;
    }
    if (-1 == write(sd, &currentPackageSize, 4) || -1 == write(sd, package, currentPackageSize)){
      printf("[server]Failed to send information about the package number %d to the client!\n", i);
      return;
    }
  }
}

void serverListFilesToDownloadLogic(int sd) {
  uint fileNameLen;
  if (-1 == read(sd, &fileNameLen, 4)){
    printf("[server]Failed to read the length of the file name!\n");
    return;
  }
  if (!fileNameLen){
    uint filesNumber = 0;
    for (auto& bucket:sharedFiles){
      filesNumber += bucket.second.size();
    }
    if (-1 == write(sd, &filesNumber, 4)){
      printf("[server]Failed to send the number of files to the client!\n");
      return;
    }
    for (auto& bucket:sharedFiles){
      bool good = true;
      for (auto& sharedFiles:bucket.second){
        if (!serverListFilesToDownloadSendFile(sd, &sharedFiles)){
          good = false;
          break;
        }
      }
      if (!good){
        break;
      }
    }
  } else {
    std::string fileName;
    fileName.resize(fileNameLen);
    for (int i=0;i<fileNameLen;++i){
      if (-1 == read(sd, &fileName[i], 1)){
        printf("[server]Failed to read the %d th byte of the file name!\n", i+1);
        return;
      }
    }
    uint filesNumber = sharedFiles[fileName].size();
    if (-1 == write(sd, &filesNumber, 4)){
      printf("[server]Failed to send the number of files to the client!\n");
      return;
    }
    for (auto& sharedFile:sharedFiles[fileName]){
      if (!serverListFilesToDownloadSendFile(sd, &sharedFile)){
        break;
      }
    }
  }
}

bool serverListFilesToDownloadSendFile(int sd, sharedFileInfo* sharedFile) {
  if (!sharedFile){
    return false;
  }
  uint fileSize = sharedFile->name.size();
  if (-1 == write(sd, &fileSize, 4) || -1 == write(sd, sharedFile->name.c_str(), fileSize) ||
    -1 == write(sd, &sharedFile->customHash, 4)){
      printf("[server]Error when sending data about a file to the client!\n");
      return false;
    }
  return true;
}

node serverFindSucc(const node& nd, uint id) {
  int sd;
  if (!initClient(sd, nd.address.c_str(), nd.port)){
    return info.me;
  }

  debugMessage("I'm asking node at port %u to find the successor of id %u\n", nd.port, id);
  
  uint request = SRV_FIND_SUCCESSOR;
  if (-1 == write(sd, &request, 4) || -1 == write(sd, &id, 4)){
    printf("[server]Failed to ask a specific node to find a successor!\n");
    close(sd);
    return info.me;
  }
  node res;
  readNodeInfo(sd, res);

  close(sd);
  return res;
}

node serverFindSucc(uint id) {
  if (id == info.me.key){
    debugMessage("I'm trying to find the succesor of node %u, and it is me", id);
    return info.me;
  }
  node pred = serverFindPred(id);
  if (pred == info.me){
    debugMessage("I'm trying to find the succesor of node %u, and it is my successor", id);
    return info.fTable.fingers[0];
  }
  
  int sd;
  if (!initClient(sd, pred.address.c_str(), pred.port)){
    return info.me;
  }
  node res = serverSendSuccessorRequest(sd);
  close(sd);
  return res;
}

void serverFindSuccessor(int sd) {
  uint reqId;
  if (-1 == read(sd, &reqId, 4)){
    printf("[server]Failed to read an id for a \"findSuccessor\" request!\n");
    return;
  }
  debugMessage("Got a request: find the successor of id %d\n", reqId);
  node succ = serverFindSucc(reqId);
  sendNodeInfo(sd, succ);
}

node serverGetPred(const node& nd) {
  int sd;
  if (!initClient(sd, nd.address.c_str(), nd.port)){
    return info.me;
  }

  debugMessage("I'm asking node %u to send me its predecessor\n", nd.key);

  uint request = SRV_GET_PREDECESSOR;
  if (-1 == write(sd, &request, 4)){
    printf("[server]Failed to ask a specific node to send its predecessor!\n");
    close(sd);
    return info.me;
  }
  node res;
  readNodeInfo(sd, res);

  close(sd);
  return res;
}

node serverFindPred(uint id) {
  debugMessage("I'm trying to find the predecessor of node %u", id);

  if (between(id, info.me.key+1, info.fTable.fingers[0].key)){
    return info.me;
  }
  node closest = closestPrecedingFinger(id);
  int sd;
  if (!initClient(sd, closest.address.c_str(), closest.port)){
    return info.me;
  }

  int request = SRV_FIND_PREDECESSOR;
  if (-1 == write(sd, &request, 4) || -1 == write(sd, &id, 4)){
    printf("[server]Failed to send info during a \"serverFindPred\" operation!\n");
    close(sd);
    return info.me;
  }
  node res;
  readNodeInfo(sd, res);

  close(sd);
  return res;
}

void serverFindPredecessor(int sd) {
  uint reqId;
  if (-1 == read(sd, &reqId, 4)){
    printf("[server]Failed to read an id for a \"findPredeccessor\" request!\n");
    return;
    
  }
  debugMessage("Got a request: find the predecessor of id %d\n", reqId);
  node pred = serverFindPred(reqId);
  sendNodeInfo(sd, pred);
}

node serverSendSuccessorRequest(int sd){
  uint request = SRV_GET_SUCCESSOR;
  if (-1 == write(sd, &request, 4)){
    printf("[server]Failed to send SRV_GET_SUCCESSOR flag to a peer!\n");
    return info.me;
  }
  debugMessage("I send a request to get the successor of a node\n");
  node res;
  readNodeInfo(sd, res);
  return res;
}

void serverSendSuccessor(int sd) {
  sendNodeInfo(sd, info.fTable.fingers[0]);
  debugMessage("Got a request: send my successor\n");
}

void serverSendPredecessor(int sd) {
  sendNodeInfo(sd, info.fTable.predecessor);
  debugMessage("Got a request: send my predecessor\n");
}

void serverUpdatePredecessorRequest(const node& nd) {
  int sd;
  if (!initClient(sd, nd.address.c_str(), nd.port)){
    return;
  }
  uint request = SRV_UPDATE_PREDECESSOR;
  if (-1 == write(sd, &request, 4)){
    printf("[server]Failed to ask a peer to set this server as its predecessor!\n");
    close(sd);
    return;
  }

  debugMessage("I'm asking node %u to set me (node %u) as its predecessor\n", nd.key, info.me.key);

  sendNodeInfo(sd, info.me);

  close(sd);
}

void serverUpdatePredecessor(int sd){
  node newPred;
  if (!readNodeInfo(sd, newPred)){
    return;
  }
  debugMessage("Got a request: update my predecessor, the new one is id %u\n", newPred.key);
  info.fTable.predecessor = newPred;
}

void serverUpdateFingerTable(int sd) {
  node nd;
  uint fingerIndex;
  if (!readNodeInfo(sd, nd) || -1 == read(sd, &fingerIndex, 4)){
    printf("[server]Failed to read protocol data for a \"serverUpdateFingerTable\" operation!\n");
    return;
  }

  debugMessage("Got a request: node %u has me as its %u th finger", nd.key, fingerIndex);
  
  if (between(nd.key, info.me.key, info.fTable.fingers[fingerIndex].key-1)){
    info.fTable.fingers[fingerIndex] = nd;
    node p = info.fTable.predecessor;
    if (p != nd){
      serverUpdateFingerTable(p, nd, fingerIndex);
    }
  }
}

void serverUpdateFingerTable(const node& p, const node& nd, uint i) {
  int sd;
  if (!initClient(sd, p.address.c_str(), p.port)){
    return;
  }
  debugMessage("I'm notifying node %u that node %u has him as its %u th finger", p.key, nd.key, i);
  uint request = SRV_UPDATE_FINGERS_TABLE;
  if (-1 == write(sd, &request, 4) || !sendNodeInfo(sd, nd) || -1 == write(sd, &i, 4)){
    printf("Failed to notify another peer to update its fingers table!\n");
  }
  close(sd);
}

static void* serverLogic(void *) {
	pthread_detach(pthread_self());

  struct sockaddr_in from;
  int sd = createServer();
  bool running = true;

  if (listen(sd, 20) == -1){
    perror("[server]Error when calling listen() in server!");
  }

  pthread_t cv[100];
  int client[100];
  int curr = 0;

  while (running){
    memset(&from, 0, sizeof(from));
    uint length = sizeof(from);
    if ((client[curr] = accept (sd, (struct sockaddr *) &from, &length)) < 0){
      perror ("[server]Error when calling accept()!");
      continue;
    }
    pthread_create(&cv[curr], NULL, treat, &client[curr++]);
  }

  return NULL;
}

int createServer() {
  struct sockaddr_in server;
  int sd, on = 1;

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1){
    perror ("[server]Error when creating the socket for the server!");
    return errno;
  }
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

  bzero (&server, sizeof (server));

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr (info.me.address.c_str());
  server.sin_port = htons (info.me.port);

  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1) {
    perror ("[server]Error when binding the socket with the server!");
    return errno;
  }
  return sd;
}

static void* treat(void* arg) {
  if (arg == NULL){
    perror("[server]Internal error! The argument for treat function should be the descriptor of the client!");
    return NULL;
  }
  
  int sd = *((int*)arg), option;
  if (-1 == read(sd, &option, 4)) {
    perror("[server]Internal error! Error when reading the option number!");
    return NULL;
  }

  switch (option){
    // Chord functionalities:
    case SRV_FIND_SUCCESSOR:
      serverFindSuccessor(sd);
      break;
    case SRV_GET_SUCCESSOR:
      serverSendSuccessor(sd);
      break;
    case SRV_GET_PREDECESSOR:
      serverSendPredecessor(sd);
      break;
    case SRV_FIND_PREDECESSOR:
      serverFindPredecessor(sd);
      break;
    case SRV_UPDATE_PREDECESSOR:
      serverUpdatePredecessor(sd);
      break;
    case SRV_UPDATE_FINGERS_TABLE:
      serverUpdateFingerTable(sd);
      break;
    // Part2Part functionalities:
    case SRV_ADD_FILE:
      break;
    case SRV_DOWNLOAD_FILE:
      serverDownloadFileLogic(sd);
      break;
    case SRV_LIST_FILES:
      serverListFilesToDownloadLogic(sd);
      break;
    default:
      printf("[server]This should not happen, got an invalid request ID: %d!\n", option);
      return NULL;
  }

  if (-1 == close(sd)){
    std::string errorString = std::string("[server]Error when calling close() for a client socket descriptor!") + std::to_string(option);
    perror(errorString.c_str());
    return NULL;
  }
  return NULL;
};

void clientDownloadFileLogic(const cmd::commandResult& command) {
  std::string fileName  = command.getStringOptionValue("-name");
  if (fileName == "none"){
    printf("[client]You have to specify the name of the file you want to download!\n");
    return;
  }
  int fileId            = command.getNumberOptionValue("-id");
  if (fileId == -1){
    printf("[client]You have to specify the id of the file you want to download!\n");
    return;
  }
  std::string peerIp    = command.getStringOptionValue("-ip");
  if (peerIp == "none"){
    printf("[client]You have to specify the ip of the peer that shares the file you want to download!\n");
    return;
  }
  int peerPort          = command.getNumberOptionValue("-port");
  if (peerPort == -1){
    printf("[client]You have to specify the port of the peer that shares the file you want to download!\n");
    return;
  }
  int sd;
  if (!initClient(sd, peerIp.c_str(), peerPort)){
    return;
  }
  uint request = SRV_DOWNLOAD_FILE, fileNameLen = fileName.size(), toSendFileId = fileId;
  uint response;

  if (-1 == write(sd, &request, 4) || -1 == write(sd, &fileNameLen, 4) || -1 == write(sd, fileName.c_str(), fileNameLen)
    || -1 == write(sd, &toSendFileId, 4)){
    printf("[client]Failed to send protocol info to the server!\n");
    close(sd);
    return;
  }
  if (-1 == read(sd, &response, 4)){
    printf("[client]Failed to read the response from the server!\n");
    close(sd);
    return;
  }
  if (response == SRV_DOWNLOAD_FILE_OK_BEGIN){
    clientDownloadFileDownload(sd, fileName, fileId);
  } else if (response == SRV_DOWNLOAD_FILE_NOT_EXISTS) {
    printf("[client]The peer claims the entered fileName+fileId does not match any of its file!\n");
  } else if (response == SRV_DOWNLOAD_FILE_NOT_AVAILABLE) {
    printf("[client]The peer doesn't have the file on his PC anymore!\n");
  } else if (response == SRV_ERROR) {
    printf("[client]The server crashed!\n");
  } else {
    printf("[client]Internal error!\n");
  }
  close(sd);
}

void clientDownloadFileDownload(int sd, const std::string& fileName, uint fileId) {
  std::string fileIdAsString = std::to_string(fileId);
  std::string newFileName = std::string(userDownloadsFolder) + fileName + std::string("::") + fileIdAsString + std::string(userDownloadsExtension);
  if (!fileExists(newFileName.c_str())){
    if (!fileCreate(newFileName.c_str())){
      printf("[client]Failed to create new file named %s\n", newFileName.c_str());
      return;
    }
    int fd = open(newFileName.c_str(), O_WRONLY);
    if (-1 == fd){
      printf("[client]Failed to open the file where the information is supposed to be downloaded %s\n", newFileName.c_str());
      return;
    }
    char package[PACKAGE_SIZE];
    uint packagesNumber, currentPackageSize;
    if (-1 == read(sd, &packagesNumber, 4)){
      printf("[client]Failed to read the number of packages that were about to be sent!\n");
      return;
    }
    for (int i=1;i<=packagesNumber;++i){
      if (-1 == read(sd, &currentPackageSize, 4) || -1 == read(sd, package, currentPackageSize)){
        printf("[client]Error when reading package number %d from the network!\n", i);
        return;
      }
      if (-1 == write(fd, package, currentPackageSize)){
        printf("[client]Error when writing package number %d to the file!\n", i);
        return;
      }
    }
    close(fd);
    printf("Download complete!\n");
  } else {
    // to do?
  }
}

void clientListFilesToDownloadLogic(const cmd::commandResult& command) {
  std::string peerIp    = command.getStringOptionValue("-ip");
  if (peerIp == "none"){
    printf("[client]You have to specify the ip of the peer whose files names you want to get!\n");
    return;
  }
  int peerPort          = command.getNumberOptionValue("-port");
  if (peerPort == -1){
    printf("[client]You have to specify the port of the peer whose files names you want to get!\n");
    return;
  }
  int sd;
  if (!initClient(sd, peerIp.c_str(), peerPort)){
    return;
  }

  std::string fileName  = command.getStringOptionValue("-name");
  uint request = SRV_LIST_FILES, fileNameLen = fileName.size(), filesNumber, peerFileNameLen, peerFileId;
  std::string peerFileName;

  if (-1 == write(sd, &request, 4) || -1 == write(sd, &fileNameLen, 4)){    
    printf("[client]Failed to send protocol info to the server!\n");
    close(sd);
    return;
  }
  if (fileNameLen){
    if (-1 == write(sd, fileName.c_str(), fileNameLen)){
      printf("[client]Failed to send the name to the server!\n");
      close(sd);
      return;
    }
  }
  if (-1 == read(sd, &filesNumber, 4)){
    printf("[client]Failed to read the number of files the peer shares!\n");
    close(sd);
    return;
  }

  if (filesNumber){
    printf("The peer shares %d files", filesNumber);
    if (fileNameLen){
      printf(" that match the file name you entered");
    }
    printf(":\n");
    for (int i=1;i<=filesNumber;++i){
      if (-1 == read(sd, &peerFileNameLen, 4)){
        printf("[client]Failed to read the file name length of the %dth file from the list!\n", i);
        break;
      }
      peerFileName.resize(peerFileNameLen);
      int j;
      for (j=0;j<peerFileNameLen;++j){
        if (-1 == read(sd, &peerFileName[j], 1)){
          printf("[client]Failed to read the %dth character of the file name number %d from the list!\n", j+1, i);
          break;
        }
      }
      if (j < peerFileNameLen){
        break;
      }
      if (-1 == read(sd, &peerFileId, 4)){
        printf("[client]Failed to read the file id of the %dth file from the list!", i);
        break;
      }
      printf("%d. File name: %s, File id: %d\n", i, peerFileName.c_str(), peerFileId);
    }
  } else {
    printf("[client]The peer you sent the request doesn't share any files at the moment that match your criteria!\n");
  }

  close(sd);
}

bool initClient(int& sd, const char* ipAddress, int port) {
  struct sockaddr_in client;
  if (-1 == (sd = socket(AF_INET, SOCK_STREAM, 0))){
    perror("[client]Error when creating a socket!");
    return false;
  }
  client.sin_family = AF_INET;
  client.sin_addr.s_addr = inet_addr(ipAddress);
  client.sin_port = htons (port);
  if (-1 == connect (sd, (struct sockaddr *) &client,sizeof (struct sockaddr))){
    std::string errorMsg = std::string("[client]Error when connecting to peer!") + std::string(ipAddress) + std::string(", ") + std::to_string(port);
    perror(errorMsg.c_str());
    close(sd);
    return false;
  }
  return true;
}

void clientLogic() {
  bool clientRunning = true;

  cmd::commandParser parser;
  initCmd(parser);

  while (clientRunning){
    std::cout<<"node"<<info.me.key<<"@chord: ";
    std::string inputLine;
    std::getline(std::cin,inputLine);

    cmd::commandResult info = parser.parse(inputLine);
    clientRunning = executeClientCommand(parser, info);
  }
}

bool executeClientCommand(const cmd::commandParser& parser, cmd::commandResult& command){
  switch (command.id){
    case cmd::commandId::ADD_FILE:
      shareFile(command);
      break;
    case cmd::commandId::SEARCH_FILE:
      break;
    case cmd::commandId::DOWNLOAD_FILE:
      clientDownloadFileLogic(command);
      break;
    case cmd::commandId::LIST_FILES_TO_DOWNLOAD:
      clientListFilesToDownloadLogic(command);
      break;
    case cmd::commandId::LIST_FILES:
      listSharedFiles();
      break;
    case cmd::commandId::CONFIG_ADD_FILE:
      configFileAddEntry(command);
      break;
    case cmd::commandId::CONFIG_REMOVE_FILE:
      configFileRemoveEntry(command);
      break;
    case cmd::commandId::CONFIG_REMOVE_ALL:
      configFileRemoveAll(command);
      break;
    case cmd::commandId::CONFIG_LIST_FILES:
      configFileListAll(command);
      break;
    case cmd::commandId::CONFIG_AUTO_ADD:
      configFileAutoAdd(command);
      break;
    case cmd::commandId::CHORD_NODE_INFO:
      printThisChordNodeInfo();
      break;
    case cmd::commandId::LISTALL:
      std::cout<<parser;
      break;
    case cmd::commandId::CLOSE:
      return false;
    case cmd::commandId::WOC:
      std::cout<<"Invalid command!\n";
      break;
    case cmd::commandId::WOCOPT:
      std::cout<<"Invalid options for command!\n";
      break;
    default: 
      break;
  }
  return true;
}

void initCmd(cmd::commandParser& parser){
  parser.addCommand(cmd::commandId::ADD_FILE, "add", "adds a file to the network");
  parser.addCommandOptionString(cmd::commandId::ADD_FILE, "-name:<fileNameInChord>", "none");
  parser.addCommandOptionString(cmd::commandId::ADD_FILE, "-path:<filePath>", "none");

  parser.addCommand(cmd::commandId::SEARCH_FILE, "search", "searches for a file in the network");
  parser.addCommandOptionString(cmd::commandId::SEARCH_FILE, "-name:<fileName>", "none");

  parser.addCommand(cmd::commandId::DOWNLOAD_FILE, "download", "downloads a file from a peer");
  parser.addCommandOptionString(cmd::commandId::DOWNLOAD_FILE, "-name:<fileName>", "none");
  parser.addCommandOptionString(cmd::commandId::DOWNLOAD_FILE, "-ip:<peerIP>", "none");
  parser.addCommandOptionNumber(cmd::commandId::DOWNLOAD_FILE, "-id:<fileId>", -1);
  parser.addCommandOptionNumber(cmd::commandId::DOWNLOAD_FILE, "-port:<peerPort>", -1);

  parser.addCommand(cmd::commandId::LIST_FILES_TO_DOWNLOAD, "list", "requests a peer to send information about all the file it shares");
  parser.addCommandOptionString(cmd::commandId::LIST_FILES_TO_DOWNLOAD, "-ip:<peerIP>", "none");
  parser.addCommandOptionString(cmd::commandId::LIST_FILES_TO_DOWNLOAD, "-name:<fileName>", "");
  parser.addCommandOptionNumber(cmd::commandId::LIST_FILES_TO_DOWNLOAD, "-port:<peerPort>", -1);

  parser.addCommand(cmd::commandId::LIST_FILES, "list-files", "lists all my shared files");

  parser.addCommand(cmd::commandId::CONFIG_ADD_FILE, "config-add-file", "adds an entry to the auto-upload list file");
  parser.addCommandOptionString(cmd::commandId::CONFIG_ADD_FILE, "-name:<fileName>", "none");
  parser.addCommandOptionString(cmd::commandId::CONFIG_ADD_FILE, "-path:<filePath>", "none");

  parser.addCommand(cmd::commandId::CONFIG_REMOVE_FILE, "config-rm-file", "removes an entry from the auto-upload list file");
  parser.addCommandOptionString(cmd::commandId::CONFIG_REMOVE_FILE, "-name:<fileName>", "none");

  parser.addCommand(cmd::commandId::CONFIG_REMOVE_ALL, "config-rm-all", "removes all the entries from the auto-upload list file");

  parser.addCommand(cmd::commandId::CONFIG_LIST_FILES, "config-list", "prints all the files that will be shared when the server is starts");

  parser.addCommand(cmd::commandId::CONFIG_AUTO_ADD, "config-auto-add", "sets whether the entries from the auto-upload list have to be shared when the server starts");
  parser.addCommandOptionBoolean(cmd::commandId::CONFIG_AUTO_ADD, "-enable", false);
  parser.addCommandOptionBoolean(cmd::commandId::CONFIG_AUTO_ADD, "-disable", false);

  parser.addCommand(cmd::commandId::CHORD_NODE_INFO, "chord-info", "prints info about this chord node");

  parser.addCommand(cmd::commandId::LISTALL, "list-cmd" ,"displays information about all the commands");

  parser.addCommand(cmd::commandId::CLOSE, "close", "closes the application");
}