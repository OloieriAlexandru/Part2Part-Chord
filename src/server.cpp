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
#include <time.h>

#include <unordered_map>
#include <iostream>
#include <vector>
#include <string>

#include "command-line.h"
#include "helper.h"
#include "chord.h"
#include "defines.h"

#define umap std::unordered_map

extern int                                      errno;

node          getFirstNodeInfo();
bool          isFirstChordNode(const std::string& address, uint port);
bool          check(int argc, char* argv[]);
void          shareFilesFromConfigFile();

nodeInfo                                        info;
umap<std::string,std::vector<sharedFileInfo>>   sharedFiles;
std::vector<std::string>                        fileCategories;

umap<std::string,std::vector<chordFileInfo>>    chordFiles;

bool          chordIdIsTaken(const node& randomNode);
void          joinChordNetwork();
void          joinChordNetwork(const node& randomNode);
void          initFingerTable(const node& randomNode);
void          updateOthers();
void          getFilesFromPredecessor();
void          printWelcomeToChordMessage();
void          leaveChordNetwork();
void          removeMyFilesFromTheNetwork();
void          moveFilesToSuccessor();
void          updatePredecessorOfSuccessor();
void          replaceMeInFingersTable();

SHA1          sha1;
uint          getHash(const char* str);

uint          addFileToNetworkToPeer(const node& nd, const sharedFileInfo& file);
bool          addFileToNetwork(const sharedFileInfo& file);
uint          removeFileFromNetworkFromPeer(const node& nd, const sharedFileInfo file);
bool          removeFileFromNetwork(const sharedFileInfo& file);
void          shareFile(const std::string& name, const std::string& path, const std::string& description, const std::string& category, bool atInit = false);
void          listSharedFiles();

void          serverSendErrorFlag(int sd);
uint          serverAddSharedFileLogic(const chordFileInfo& newFile);
void          serverAddSharedFile(int sd);
void          serverDownloadFileLogic(int sd);
void          serverDownloadFileUpload(int sd, sharedFileInfo* sharedFile);
void          serverListFilesToDownloadLogic(int sd);
bool          serverListFilesToDownloadSendFile(int sd, sharedFileInfo* sharedFile);
void          serverSearchFiles(int sd);
uint          serverRemoveFileLogic(const chordFileInfo& removedFile);
void          serverRemoveFile(int sd);

node          serverFindSucc(const node& nd, uint id);
node          serverFindSucc(uint id);
void          serverFindSuccessor(int sd);
node          serverGetPred(const node& nd);
node          serverFindPred(uint id);
void          serverFindPredecessor(int sd);
node          serverSendSuccessorRequest(int sd);
void          serverSendSuccessor(int sd);
void          serverSendPredecessor(int sd);
void          serverUpdateSuccessorRequest(const node& nd);
void          serverUpdateSuccessor(int sd);
void          serverUpdatePredecessorRequest(const node& nd, const node& pred);
void          serverUpdatePredecessorRequest(const node& nd);
void          serverUpdatePredecessor(int sd);
void          serverUpdateFingerTable(int sd);
void          serverUpdateFingerTable(const node& p, const node& nd, uint i);
void          serverChordGetKeysFromPredecessor(int sd);
void          serverAddFilesFromPredecessor(int sd);
void          serverReplaceNodeFromFingersTable(int sd);
uint          serverCountChordNodesRequest();
void          serverCountChordNodes(int sd);

void          serverChordStabilization(int sd);
void          chordStabilizationNotify(const node& nd);
void          chordStabilize();
void          chordFixFingers();

static void*  chordStabilization(void *);
static void*  serverLogic(void *);

pthread_t     serverThreads[THREADS_COUNT];
threadInfo    serverClients[THREADS_COUNT];

int           createServer();

bool          executeClientCommand(const cmd::commandParser& parser, cmd::commandResult& command);
void          initCmd(cmd::commandParser& parser);

static void*  treat(void *);

void          clientAddFileLogic(const cmd::commandResult& command);
void          clientSearchFileLogicAnotherPeer(const node& nd, const std::string& fileName);
void          clientSearchFileLogic(const cmd::commandResult& command);
void          clientDownloadFileLogic(const cmd::commandResult& command);
void          clientDownloadFileDownload(int sd, const std::string& fileName, uint fileId);
void          clientListFilesToDownloadLogic(const cmd::commandResult& command);
void          clientRemoveFileLogic(const cmd::commandResult& command);

bool          checkId(int id);
void          printChordSucc(const cmd::commandResult& command);
void          printChordPred(const cmd::commandResult& command);

bool          initClient(int& sd, const char* ipAddress, int port);
void          clientLogic();

void          serverConcurrentFunction(int sd);
void          clientConcurrentFunction(const cmd::commandResult& command);

int main(int argc, char* argv[]) {
  if (!check(argc, argv)){
    return 1;
  }

  initFileSharingInfo();
  
  if (!configFileInit()) {
    fprintf(stderr, "[main]Error when creating/checking the config file!");
    return 1;
  }

  pthread_t serverThread, stabilizationThread;
  if (pthread_create(&serverThread, NULL, serverLogic, NULL)){
    perror("[main]Error when creating main thread for the server!");
    return 1;
  }

  joinChordNetwork();
  printWelcomeToChordMessage();
  shareFilesFromConfigFile();

  if (pthread_create(&stabilizationThread, NULL, chordStabilization, NULL)){
    perror("[main]Error when creating a thread for the stabilization operations!");
    return 1;
  }

  clientLogic();
  leaveChordNetwork();
  return 0;
};

node getFirstNodeInfo(){
  node res;
  res.address = myAddress;
  res.port = CHORD_FIRST_PORT;
  return res;
}

bool isFirstChordNode(const std::string& address, uint port){
  return (address == myAddress && port == CHORD_FIRST_PORT);
}

bool check(int argc, char* argv[]){
  if (argc != 2){
    fprintf(stderr, "Invalid number of arguments, expected: 1 - the port of this server!\n");
    return false;
  }
  if (!(argv[1][0] >= '0' && argv[1][0] <= '9') || strlen(argv[1]) > 5){
    fprintf(stderr, "Invalid argument, expected: an integer - the port of this server!\n");
    return false;
  }
  info.me.port = 0;
  int digitsNo = 0;
  while (argv[1][digitsNo] && argv[1][digitsNo] >= '0' && argv[1][digitsNo] <= '9'){
    info.me.port = info.me.port * 10 + (argv[1][digitsNo++] - '0');
  }
  if (!(info.me.port < 1<<16)){
    fprintf(stderr, "Invalid argument, expected: an integer - the port of this server!\n");
    return false;
  }
  
  info.me.address = myAddress;
  std::string keyStr = info.me.address + std::to_string(info.me.port);
  info.me.key = getHash(keyStr.c_str());

  if (info.me.port != 3500){
    node firstNode = getFirstNodeInfo();
    if (chordIdIsTaken(firstNode)){
      printf("You were asign chord id %u, but this id already exists in the network! Please choose another port!\n", info.me.key);
      return false;
    }
  }

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
  std::string name, path, description, category;
  fileIn >> autoAdd;
  while (fileIn >> name >> path){
    readConfigFileDescription(fileIn, description);
    fileIn >> category;
    if (description == fileEmptyDescription){
      description.clear();
    }
    notifyMessage("File %s: ", name.c_str());
    shareFile(name, path, description, category, true);
  }
  fileIn.close();
}

bool chordIdIsTaken(const node& randomNode) {
  node succ = serverFindSucc(randomNode, info.me.key);
  if (succ.key == info.me.key){
    return true;
  }
  return false;
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
  getFilesFromPredecessor();
}

void initFingerTable(const node& randomNode) {
  int sk;
  node succ = serverFindSucc(randomNode, info.intervals[0].start);
  info.fTable.fingers[0] = succ;
  info.fTable.predecessor = serverGetPred(succ);
  serverUpdatePredecessorRequest(succ);
  serverUpdateSuccessorRequest(info.fTable.predecessor);
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
    if (p != info.me){
      serverUpdateFingerTable(p, info.me, i);
    }
  }
}

void getFilesFromPredecessor() {
  if (info.fTable.predecessor == info.me){
    return;
  }
  int sd;
  if (!initClient(sd, info.fTable.predecessor.address.c_str(), info.fTable.predecessor.port)){
    return;
  }
  uint request = SRV_GET_MY_KEYS, filesCount, response;
  chordFileInfo myFile;
  if (-1 == write(sd,&request,4) || -1 == write(sd,&info.me.key,4)){
    printf("Failed to send protocol data to my predecessor in order to get the keys I'm responsible for!\n");
    close(sd);
    return;
  }
  if (-1 == read(sd,&response,4)){
    printf("Failed to read the response flag from my predecessor!\n");
    close(sd);
    return;
  }
  if (!(response == SRV_GET_MY_KEYS_OK)){
    return;
  }
  if (-1 == read(sd,&filesCount,4)){
    printf("Failed to read the number of files I'm responbile for that my predecessor sent me!\n");
    close(sd);
    return;
  }
  for (int i=0;i<filesCount;++i){
    if (!readChordFileInfo(sd,myFile)){
      printf("Failed to read the %d th file I'm responsible for!\n", i);
      close(sd);
      return;
    }
    chordFiles[myFile.name].push_back(myFile);
  }
  close(sd);
}

void printWelcomeToChordMessage() {
  notifyMessage("You have successfully joined the Chord network!\n");
  uint chordNodesCount = serverCountChordNodesRequest();
  notifyMessage("Currently, there %s %u node%s in the network, and you were assigned id %u!\n", (chordNodesCount == 1 ? "is" : "are"), 
    chordNodesCount, (chordNodesCount == 1 ? "" : "s"), info.me.key);
  notifyMessage("To see a list of all the commands, type \"list-cmd\"!\n");
}

void leaveChordNetwork() {
  if (info.me == info.fTable.fingers[0]){
    return;
  }
  removeMyFilesFromTheNetwork();
  moveFilesToSuccessor();
  updatePredecessorOfSuccessor();
  replaceMeInFingersTable();
}

void removeMyFilesFromTheNetwork() {
  for (auto& bucket:sharedFiles){
    for (auto& file:bucket.second){
      if (!removeFileFromNetwork(file)){
        printf("[!!!]Failed to remove one of my files from the network! (%s, %s)", file.name.c_str(), file.path.c_str());
      }
    }
  }
}

void moveFilesToSuccessor() {
  int sd;
  uint request = SRV_ADD_PREDECESSOR_FILES, response;
  if (!initClient(sd,info.fTable.fingers[0].address.c_str(),info.fTable.fingers[0].port)){
    return;
  }
  if (-1 == write(sd,&request,4)){
    printf("[client]Failed to send protocol data to my successor!\n");
    close(sd);
    return;
  }
  if (!sendNodeInfo(sd,info.me)){
    printf("[client]Failed to send info about this node to successor for a \"moveFilesToSuccessor\" operation!\n");
    close(sd);
    return;
  }
  if (-1 == read(sd,&response,4)){
    printf("[client]Failed to read the response from peer for a \"moveFilesToSuccessor\" operation!\n");
    close(sd);
    return;
  }
  if (response == SRV_ADD_PREDECESSOR_FILES_NO){
    return;
  }
  uint fileIndex = 0, filesCount = 0;
  for (auto& bucket:chordFiles){
    filesCount += bucket.second.size();
  }
  if (-1 == write(sd,&filesCount,4)){
    printf("[client]Failed to send the number of files I have to send to my successor!\n");
    close(sd);
    return;
  }
  for (auto& bucket:chordFiles){
    for (auto& file:bucket.second){
      ++fileIndex;
      if (!sendChordFileInfo(sd,file)){
        printf("[client]Failed to send the %d th file to my successor!\n", fileIndex);
        close(sd);
        return;
      }
    }
    bucket.second.clear();
  }
  close(sd);
}

void updatePredecessorOfSuccessor() {
  serverUpdatePredecessorRequest(info.fTable.fingers[0], info.fTable.predecessor);
}

void replaceMeInFingersTable() {
  uint request = SRV_NODE_LEFT;
  int sd;
  if (!initClient(sd, info.fTable.fingers[0].address.c_str(), info.fTable.fingers[0].port)){
    return;
  }
  if (-1 == write(sd,&request,4) || !sendNodeInfo(sd,info.me) || !sendNodeInfo(sd,info.fTable.fingers[0])){
    close(sd);
    printf("Failed to send protocol data for a \"replaceMeInFingersTable\" operation!\n");
    return;
  }
  close(sd);
}

uint getHash(const char* str) {
  return getHash(sha1, str);
}

uint addFileToNetworkToPeer(const node& nd, const sharedFileInfo& file) {
  uint response;
  if (info.me == nd){
    chordFileInfo newFile(file.name, file.description, file.category, file.size, file.shaHash, file.customHash, nd.address, nd.port);
    response = serverAddSharedFileLogic(newFile);
  } else {
    int sd;
    if (!initClient(sd, nd.address.c_str(), nd.port)){
      return SRV_ERROR;
    }
    uint request = SRV_ADD_FILE;
    if (-1 == write(sd,&request,4)){
      close(sd);
      printf("[!!!]Failed to send protocol data to peer!\n");
      return SRV_ERROR;
    }
    if (!sendChordFileInfo(sd, file)){
      close(sd);
      return SRV_ERROR;
    }
    if (-1 == read(sd,&response,4)){
      close(sd);
      printf("[!!!]Failed to read the server response for a \"addFileToNetwork\" operation!\n");
      return SRV_ERROR;
    }
    close(sd);
  }
  if (response == SRV_ADD_FILE_ALREADY_EXISTS){
    printf("The server claims that the file already exists!\n");
  } 
  return response;
}

bool addFileToNetwork(const sharedFileInfo& file){
  node responsibleNode;
  if (file.shaHash == info.me.key){
    responsibleNode = info.me;
  } else {
    responsibleNode = serverFindSucc(file.shaHash);
  }
  uint response = addFileToNetworkToPeer(responsibleNode, file);
  if (response == SRV_ADD_FILE_OK_ADDED){
    std::cout<<"File added successfully at node: "<<responsibleNode<<'\n';
  } else if (response == SRV_ERROR) {
    std::cout<<"Server error!\n";
  }
  return response == (SRV_ADD_FILE_OK_ADDED);
}

uint removeFileFromNetworkFromPeer(const node& nd, const sharedFileInfo file) {
  uint response;
  if (info.me == nd){
    chordFileInfo newFile(file.name, file.shaHash, file.customHash, nd.address, nd.port);
    response = serverRemoveFileLogic(newFile);
  } else {
    int sd;
    if (!initClient(sd, nd.address.c_str(), nd.port)){
      return SRV_ERROR;
    }
    uint request = SRV_REMOVE_FILE;
    if (-1 == write(sd, &request, 4)){
      printf("[!!!]Failed to send protocol data to peer!\n");
      close(sd);
      return SRV_ERROR;
    }
    if (!sendChordFileInfo(sd, file)){
      close(sd);
      return SRV_ERROR;
    }
    if (-1 == read(sd,&response,4)){
      printf("[!!!]Failed to read the server response for a \"removeFileFromNetwork\" operation!\n");
      close(sd);
      return SRV_ERROR;
    }
    close(sd);
  }
  if (response == SRV_REMOVE_FILE_NOT_FOUND){
    printf("The server claims that the file doesn't exist in the network!\n");
  }
  return response;
}

bool removeFileFromNetwork(const sharedFileInfo& file) {
  node responsibleNode;
  if (file.shaHash == info.me.key){
    responsibleNode = info.me;
  } else {
    responsibleNode = serverFindSucc(file.shaHash);
  }
  uint response = removeFileFromNetworkFromPeer(responsibleNode, file);
  if (response == SRV_REMOVE_FILE_OK_REMOVED){
    std::cout<<"File removed successfully from node: "<<responsibleNode<<'\n';
  } else if (response == SRV_ERROR) {
    std::cout<<"Server error!\n";
  }
  return (response == SRV_REMOVE_FILE_OK_REMOVED);
}

void shareFile(const std::string& name, const std::string& path, const std::string& description, const std::string& category, bool atInit) {
  if (!fileExists(path.c_str())) {
    if (!atInit){
      std::cout<<"Invalid path! The file doesn't exist!\n";
    } else {
      std::cout<<"Error when loading the file "<<path<<", the file is not located there anymore!\n";
    }
    return;
  }
  if (description.size() > FILE_DESC_MAX_LEN) {
    if (!atInit){
      std::cout<<"The description is too long! Max number of characters: "<<FILE_DESC_MAX_LEN<<"!\n";
      return;
    } else {
      std::cout<<"The description for file "<<path<<" is too long! Max number of characters: "<<FILE_DESC_MAX_LEN<<"!\n";
      return;
    }
  }
  uchar categoryId = getCategoryId(category);
  if (isAnInvalidCategory(categoryId)){
    if (!atInit){
      std::cout<<"Invalid file category!\n";
      return;
    } else {
      std::cout<<"The category for file"<<path<<" is invalid!\n";
      return;
    }
  }
  uint size = getFileSize(path.c_str());
  sharedFileInfo newFile(sharedFileInfo(name, path, description, categoryId, size, getCustomHash(path.c_str()), getHash(name.c_str())));
  if (addFileToNetwork(newFile)){
      sharedFiles[name].push_back(newFile);
  }
}

void listSharedFiles() {
  int fileNumber = 1;
  printf("Shared files: \n");
  for (auto bucket : sharedFiles){
    for (auto file : bucket.second) {
      printf("%d. name:%s path:%s id:%u\n", fileNumber++, file.name.c_str(), file.path.c_str(), file.customHash);
    }
  }
  if (fileNumber == 1){
    printf("None!\n");
  }
}

void serverSendErrorFlag(int sd){
  uint response = SRV_ERROR;
  if (-1 == write(sd, &response, 4)){
    printf("[server]Failed to send the error flag to the client!\n");
  }
}

uint serverAddSharedFileLogic(const chordFileInfo& newFile){
  bool found = false;
  for (auto file : chordFiles[newFile.name]){
    if (file == newFile){
      found = true;
      break;
    }
  }
  if (found){
    return SRV_ADD_FILE_ALREADY_EXISTS;
  } 
  chordFiles[newFile.name].push_back(newFile);
  return SRV_ADD_FILE_OK_ADDED;
}

void serverAddSharedFile(int sd) {
  chordFileInfo newFile;
  if (!readChordFileInfo(sd, newFile)){
    return;
  }
  uint response = serverAddSharedFileLogic(newFile);
  if (-1 == write(sd,&response,4)) {
    printf("[server]Error when sending the response back to the client (\"serverAddSharedFile\")!\n");
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

void serverSearchFiles(int sd) { 
  uint fileNameLen;
  std::string fileName;
  if (-1 == read(sd,&fileNameLen,4)){
    printf("[server]Failed to read the length of the name of the searched file!\n");
    return;
  }
  fileName.resize(fileNameLen);
  for (int i=0;i<fileNameLen;++i){
    if (-1 == read(sd,&fileName[i],1)){
      printf("[server]Failed to read the %d th byte of the searched file name!\n", i+1);
      return;
    }
  }
  uint filesCount = chordFiles[fileName].size();
  if (-1 == write(sd,&filesCount,4)){
    printf("[server]Failed to send back to the client the number of found files!\n");
    return;
  }
  if (!filesCount){
    return;
  }
  uint fileAddressLen, fileDescriptionLen;
  for (int i=0;i<filesCount;++i){
    fileAddressLen = chordFiles[fileName][i].address.size();
    fileDescriptionLen = chordFiles[fileName][i].description.size();
    if (-1 == write(sd,&fileDescriptionLen,4) || -1 == write(sd,chordFiles[fileName][i].description.c_str(),fileDescriptionLen)
      || -1 == write(sd,&chordFiles[fileName][i].category,1) || -1 == write(sd,&chordFiles[fileName][i].size,4)){
      printf("Failed to send back information about the %d th found file!\n", i+1);
      return;
    }
    if (-1 == write(sd,&chordFiles[fileName][i].id,4) || -1 == write(sd,&fileAddressLen,4) 
      || -1 == write(sd,chordFiles[fileName][i].address.c_str(),fileAddressLen) || -1 == write(sd,&chordFiles[fileName][i].port,4)) {
      printf("Failed to send back information about the %d th found file!\n", i+1);
      return;
    }
  }
}

uint serverRemoveFileLogic(const chordFileInfo& removedFile) {
  if (!chordFiles[removedFile.name].size()){
    return SRV_REMOVE_FILE_NOT_FOUND;
  }
  int position = -1;
  for (int i=0;i<chordFiles[removedFile.name].size();++i){
    if (removedFile == chordFiles[removedFile.name][i]){
      position = i;
      break;
    }
  }
  if (position == -1){
    return SRV_REMOVE_FILE_NOT_FOUND;
  }
  chordFiles[removedFile.name].erase(chordFiles[removedFile.name].begin()+position);
  return SRV_REMOVE_FILE_OK_REMOVED;
}

void serverRemoveFile(int sd) {
  chordFileInfo removedFile;
   if (!readChordFileInfo(sd, removedFile)){
    return;
  }
  uint response = serverRemoveFileLogic(removedFile);
  if (-1 == write(sd,&response,4)) {
    printf("[server]Error when sending the response back to the client (\"serverRemoveFile\")!\n");
  }
}

node serverFindSucc(const node& nd, uint id) {
  int sd;
  if (!initClient(sd, nd.address.c_str(), nd.port)){
    info.me.key = SHA_HASH_VAL;
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
  node succ = serverFindSucc(reqId);
  debugMessage("Got a request: find the successor of id %d, found: %d\n", reqId, succ.key);
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
  if (between(id, info.me.key+1, info.fTable.fingers[0].key)){
    debugMessage("I'm trying to find the predecessor of node %u, and it's me\n", id);
    return info.me;
  }
  node closest = closestPrecedingFinger(id);
  if (closest == info.me){    
    debugMessage("I'm trying to find the predecessor of node %u, and here?\n", id);
    printThisChordNodeInfo();
    return info.me;
  }

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
  debugMessage("Got a request: send my successor, sent: %d\n", info.fTable.fingers[0].key);
}

void serverSendPredecessor(int sd) {
  sendNodeInfo(sd, info.fTable.predecessor);
  debugMessage("Got a request: send my predecessor\n");
}

void serverUpdateSuccessorRequest(const node& nd) {
  int sd;
  if (!initClient(sd, nd.address.c_str(), nd.port)){
    return;
  }
  uint request = SRV_UPDATE_SUCCESSOR;
  if (-1 == write(sd, &request, 4)){
    printf("[server]Failed to ask a peer to set this server as its successor!\n");
    close(sd);
    return;
  }

  debugMessage("I'm asking node %u to set me (node %u) as its successor\n", nd.key, info.me.key);

  sendNodeInfo(sd, info.me);
  close(sd);
}

void serverUpdateSuccessor(int sd) {
  node newSucc;
  if (!readNodeInfo(sd, newSucc)){
    return;
  }
  debugMessage("Got a request: update my successor, the new one is id %u\n", newSucc.key);
  info.fTable.fingers[0] = newSucc;
}

void serverUpdatePredecessorRequest(const node& nd, const node& pred) {
  int sd;
  if (!initClient(sd, nd.address.c_str(), nd.port)){
    return;
  }
  uint request = SRV_UPDATE_PREDECESSOR;
  if (-1 == write(sd, &request, 4)){
    printf("[server]Failed to ask a peer to change its predecessor!\n");
    close(sd);
    return;
  }

  debugMessage("I'm asking node %u to chage its predecessor to %u\n", nd.key, pred.key);
  
  sendNodeInfo(sd, pred);
  close(sd);
}

void serverUpdatePredecessorRequest(const node& nd) {
  debugMessage("I'm asking node %u to set me (node %u) as its predecessor\n", nd.key, info.me.key);
  serverUpdatePredecessorRequest(nd, info.me);
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

  debugMessage("Got a request: node %u has me as its %u th finger\n", nd.key, fingerIndex);

  if (between(nd.key, info.me.key, (int)info.fTable.fingers[fingerIndex].key-1)){
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
  debugMessage("I'm notifying node %u that node %u has him as its %u th finger\n", p.key, nd.key, i);
  uint request = SRV_UPDATE_FINGERS_TABLE;
  if (-1 == write(sd, &request, 4) || !sendNodeInfo(sd, nd) || -1 == write(sd, &i, 4)){
    printf("Failed to notify another peer to update its fingers table!\n");
  }
  close(sd);
}

void serverChordGetKeysFromPredecessor(int sd) {
  uint successorKey;
  if (-1 == read(sd,&successorKey,4)){
    printf("[server]Failed to read the key of the peer that claims is my successor in order for me to send its files!\n");
    return;
  }
  if (successorKey != info.fTable.fingers[0].key){
    printf("[server]Peer %u told me it is my successor, but I have a node that has a lower id, %u!\n", successorKey, info.fTable.fingers[0].key);
    serverSendErrorFlag(sd);
    return;
  }
  uint response = SRV_GET_MY_KEYS_OK;
  if (-1 == write(sd,&response,4)){
    printf("[server]Failed to send a flag to my successor for a \"getKeysFromPredecessor\" operation!\n");
    return;
  }
  uint filesCount = 0, fileSentIndex = 0;
  for (auto& bucket:chordFiles){
    if (!bucket.second.size()){
      continue;
    }
    if (between(bucket.second[0].chordId, info.me.key+1, successorKey)){
      filesCount += bucket.second.size();
    }
  }
  if (-1 == write(sd,&filesCount,4)){
    printf("[server]Failed to send the number of files that my successor is responsible for!\n");
    return;
  }
  for (auto& bucket:chordFiles){
    if (!bucket.second.size()){
      continue;
    }
    if (!between(bucket.second[0].chordId, info.me.key+1, successorKey)){
      continue;
    }
    for (int i=bucket.second.size()-1;i>=0;--i){
      ++fileSentIndex;
      if (!sendChordFileInfo(sd,bucket.second[i])){
        printf("[server]Error when sending info about the %d th file to my successor!\n", fileSentIndex);
        return;
      }
    }
    bucket.second.clear();
  }
}

void serverAddFilesFromPredecessor(int sd){
  node pred;
  uint response;
  if (!readNodeInfo(sd,pred)){
    printf("[server]Failed to read info about the node that wants to send me its files!\n");
    return;
  }
  if (pred != info.fTable.predecessor){
    response = SRV_ADD_PREDECESSOR_FILES_NO;
  } else {
    response = SRV_ADD_PREDECESSOR_FILES_OK;
  }
  if (-1 == write(sd, &response, 4)){
    printf("[server]Failed to send protocol data for a \"serverAddFilesFromPredecessor\" operation!\n");
    return;
  }
  if (response == SRV_ADD_PREDECESSOR_FILES_NO){
    return;
  }
  uint filesCount = 0;
  chordFileInfo predFile;
  if (-1 == read(sd,&filesCount,4)){
    printf("[server]Failed to read the number of files my predecessor has to send me!\n");
    return;
  }
  for (int i=0;i<filesCount;++i){
    if (!readChordFileInfo(sd,predFile)){
      printf("[server]Failed to read the %d th file from my predecessor!\n", i);
      return;
    }
    chordFiles[predFile.name].push_back(predFile);
  }
}

void serverReplaceNodeFromFingersTable(int sd) {
  node nodeThatLeft, nodeReplacer, succ;
  if (!readNodeInfo(sd,nodeThatLeft) || !readNodeInfo(sd,nodeReplacer)){
    printf("[server]Error when reading protocol data for a \"replaceNodeFromFingersTable\" operation!\n");
    return;
  }
  if (nodeThatLeft == info.me){
    return;
  }
  succ = info.fTable.fingers[0];
  for (int i=0;i<SHA_HASH_BITS;++i){
    if (info.fTable.fingers[i] == nodeThatLeft){
      info.fTable.fingers[i] = nodeReplacer;
    }
  }
  if (nodeThatLeft == succ){
    return;
  }
  int newSd;
  uint request = SRV_NODE_LEFT;
  if (!initClient(newSd, succ.address.c_str(), succ.port)){
    return;
  }
  if (-1 == write(newSd,&request,4) || !sendNodeInfo(newSd,nodeThatLeft) || !sendNodeInfo(newSd,nodeReplacer)){
    close(newSd);
    printf("Failed to send protocol data for a \"replaceMeInFingersTable\" operation!\n");
    return;
  }
  close(newSd);
}

uint serverCountChordNodesRequest() {
  if (info.me == info.fTable.fingers[0]){
    return 1;
  }
  uint response = 1, request = SRV_COUNT_NODES;
  int sd;
  if (!initClient(sd,info.fTable.fingers[0].address.c_str(),info.fTable.fingers[0].port)){
    printf("[client]Failed to connect to my succesor in order to count the number of nodes in the network!\n");
    return 0;
  }
  if (-1 == write(sd,&request,4) || !sendNodeInfo(sd,info.me) || -1 == write(sd,&response,4)){
    printf("[client]Failed to sent protocol data to my successor for a \"countChordNodes\" operation!\n");
    close(sd);
    return 0;
  }
  if (-1 == read(sd,&response,4)){
    printf("[client]Failed to read the number of nodes in the network!\n");
    close(sd);
    return 0;
  }
  close(sd);
  return response;
}

void serverCountChordNodes(int sd) {
  node nd;
  uint response, request = SRV_COUNT_NODES;
  if (!readNodeInfo(sd,nd) || -1 == read(sd,&response,4)){
    printf("[server]Failed to read protocol data for a \"countChordNodes\" operation!\n");
    return;
  }
  ++response;
  if (nd != info.fTable.fingers[0]){
    int newSd;
    if (!initClient(newSd,info.fTable.fingers[0].address.c_str(),info.fTable.fingers[0].port)){
      printf("[server]Failed to connect to my succesor in order to count the number of nodes in the network!\n");
      return;
    }
    if (-1 == write(newSd,&request,4) || !sendNodeInfo(newSd,nd) || -1 == write(newSd,&response,4)){
      printf("[server]Failed to sent protocol data to my successor for a \"countChordNodes\" operation!\n");
      close(newSd);
      return;
    }
    if (-1 == read(newSd,&response,4)){
      printf("[server]Failed to read the number of nodes in the network!\n");
      close(newSd);
      return;
    }
    close(newSd);
  }
  if (-1 == write(sd,&response,4)){
    printf("[server]Failed to send the number of nodes in the network to the client!\n");
    return;
  }
}

void serverChordStabilization(int sd) {
  node np;
  debugMessage("A node sent me a stabilization flag, I'm reading info about that node!\n");
  if (!readNodeInfo(sd, np)){
    return;
  }
  if (between(np.key, info.fTable.predecessor.key+1, (int)info.me.key-1)){
    info.fTable.predecessor = np;
  }
}

void chordStabilizationNotify(const node& nd){
  int sd;
  if (!initClient(sd, nd.address.c_str(), nd.port)){
    return;
  }
  uint request = SRV_STABILIZATION;
  if (-1 == write(sd, &request, 4)){
    printf("Failed to send stabilization flag to a peer!\n");
    close(sd);
    return;
  }

  debugMessage("I'm sending a stabilization message to my successor, node %d\n", nd.key);

  sendNodeInfo(sd, nd);
  close(sd);
}

void chordStabilize() {
  node succPred = serverGetPred(info.fTable.fingers[0]);
  if (succPred == info.me){
    return;
  }
  if (between(succPred.key, info.me.key+1, (int)info.fTable.fingers[0].key-1)){
    info.fTable.fingers[0] = succPred;
  }
  chordStabilizationNotify(info.fTable.fingers[0]);
}

void chordFixFingers() {
  int finger = (rand() % (SHA_HASH_BITS - 1)) + 1, i;
  for (i=1;i<5;++i){
    if (info.fTable.fingers[finger] == info.me){
      finger = (rand() % (SHA_HASH_BITS - 1)) + 1;
    }
  }
  if (i == 5){
    return;
  }
  info.fTable.fingers[finger] = serverFindSucc(info.intervals[finger].start);
}

static void* chordStabilization(void *) {
  srand(time(NULL));
  while (true){
    sleep(1);
    chordStabilize();
    chordFixFingers();
  }
}

static void* serverLogic(void *) {
	pthread_detach(pthread_self());

  struct sockaddr_in from;
  int sd = createServer(), currentThreadIndex, client;

  if (sd == errno) {
    printf("Failed to create the server! Probably the chosen port is already used!\n");
    exit(1);
  }

  bool running = true;

  if (listen(sd, 20) == -1){
    perror("[server]Error when calling listen() in server!");
  }

  for (int i=0;i<THREADS_COUNT;++i){
    serverClients[i].sd = THREAD_EXIT;
    serverClients[i].threadNo = i;
  }

  while (running){
    memset(&from, 0, sizeof(from));
    uint length = sizeof(from);
    if ((client = accept (sd, (struct sockaddr *) &from, &length)) < 0){
      perror ("[server]Error when calling accept()!");
      continue;
    }
    currentThreadIndex = -1;
    for (int i=0;i<THREADS_COUNT;++i){
      if (serverClients[i].sd == THREAD_EXIT){
        currentThreadIndex = i;
        break;
      }
    }
    if (currentThreadIndex != -1){
      serverClients[currentThreadIndex].sd = client;
      pthread_create(&serverThreads[currentThreadIndex], NULL, treat, &serverClients[currentThreadIndex]);
    } else {
      printf("Error! Could not find a free thread!\n");
    }
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
  
  threadInfo* thisThreadInfo = (threadInfo*)arg;
  int sd = thisThreadInfo->sd, option;
  if (-1 == read(sd, &option, 4)) {
    serverClients[thisThreadInfo->threadNo].sd = THREAD_EXIT;
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
    case SRV_UPDATE_SUCCESSOR:
      serverUpdateSuccessor(sd);
      break;
    case SRV_UPDATE_FINGERS_TABLE:
      serverUpdateFingerTable(sd);
      break;
    case SRV_STABILIZATION:
      serverChordStabilization(sd);
      break;
    case SRV_GET_MY_KEYS:
      serverChordGetKeysFromPredecessor(sd);
      break;
    case SRV_ADD_PREDECESSOR_FILES:
      serverAddFilesFromPredecessor(sd);
      break;
    case SRV_NODE_LEFT:
      serverReplaceNodeFromFingersTable(sd);
      break;
    case SRV_COUNT_NODES:
      serverCountChordNodes(sd);
      break;
    // Part2Part functionalities:
    case SRV_ADD_FILE:
      serverAddSharedFile(sd);
      break;
    case SRV_DOWNLOAD_FILE:
      serverDownloadFileLogic(sd);
      break;
    case SRV_LIST_FILES:
      serverListFilesToDownloadLogic(sd);
      break;
    case SRV_SEARCH_FILE:
      serverSearchFiles(sd);
      break;
    case SRV_REMOVE_FILE:
      serverRemoveFile(sd);
      break;
    // Other:
    case SRV_CONCURRENT:
      serverConcurrentFunction(sd);
      break;
    default:
      serverClients[thisThreadInfo->threadNo].sd = THREAD_EXIT;
      printf("[server]This should not happen, got an invalid request ID: %d!\n", option);
      return NULL;
  }

  if (-1 == close(sd)){
    std::string errorString = std::string("[server]Error when calling close() for a client socket descriptor!") + std::to_string(option);
    perror(errorString.c_str());
    serverClients[thisThreadInfo->threadNo].sd = THREAD_EXIT;
    return NULL;
  }
  serverClients[thisThreadInfo->threadNo].sd = THREAD_EXIT;
  return NULL;
};

void clientAddFileLogic(const cmd::commandResult& command) {
  std::string fileName = command.getStringArgumentValue("file-name");
  std::string filePath = command.getStringArgumentValue("file-path");
  if (fileName == "" || filePath == ""){
    printf("You have to specify the name and the path of the file! (-name and -path options)\n");
    return;
  }
  std::string fileDescription = command.getStringOptionValue("-description");
  if (fileDescription.size() > FILE_DESC_MAX_LEN){
    printf("The file description is too long! Max. number of characters: %d\n", FILE_DESC_MAX_LEN);
    return;
  }
  std::string fileCategory = command.getStringOptionValue("-category");
  shareFile(fileName, filePath, fileDescription, fileCategory);
}

void clientSearchFileLogicAnotherPeer(const node& nd, const std::string& fileName){
  int sd;
  if (!initClient(sd, nd.address.c_str(), nd.port)){
    return;
  }
  uint request = SRV_SEARCH_FILE, fileNameLen = fileName.size(), filesCount;
  uint fileId, fileOwnerPort, fileOwnerIpLen, fileSize, fileDescriptionLen;
  uchar fileCategory;
  std::string fileDescription, fileOwnerIp, fileCategoryStr;
  if (-1 == write(sd,&request,4) || -1 == write(sd,&fileNameLen,4) || -1 == write(sd,fileName.c_str(),fileNameLen)){
    close(sd);
    printf("Error when sending protocol data to the peer that stores information about the searched files!\n");
    return;
  }
  if (-1 == read(sd,&filesCount,4)){
    close(sd);
    printf("Error when reading the number of files for the search operation!\n");
    return;
  }
  if (!filesCount){
    close(sd);
    printf("There are no files in the network with the specified name!\n");
    return;
  }
  printf("There are %d files in the network with the specified name: \n", filesCount);
  for (int i=0;i<filesCount;++i){
    if (-1 == read(sd,&fileDescriptionLen,4)){
      close(sd);
      printf("Error when reading the %d th file description length!\n", i+1);
      return;
    }
    fileDescription.resize(fileDescriptionLen);
    for (int j=0;j<fileDescriptionLen;++j){
      if (-1 == read(sd,&fileDescription[j],1)){
        printf("Error when reading the %d th byte of the %d th file's description!\n", j+1, i+1);
        close(sd);
        return;
      }
    }
    if (-1 == read(sd,&fileCategory,1)){
      printf("Error when reading the category of the %d th file!\n", i+1);
      close(sd);
      return;
    }
    if (-1 == read(sd,&fileSize,4)){
      printf("Error when reading the size of the %d th file!\n", i+1);
      close(sd);
      return;
    }
    if (-1 == read(sd,&fileId,4)){
      close(sd);
      printf("Error when reading the %d th file id!\n", i+1);
      return;
    }
    if (-1 == read(sd,&fileOwnerIpLen,4)){
      close(sd);
      printf("Error when reading the %d th file owner address len!\n",i+1);
      return;
    }
    fileOwnerIp.resize(fileOwnerIpLen);
    for (int j=0;j<fileOwnerIpLen;++j){
      if (-1 == read(sd,&fileOwnerIp[j],1)){
        printf("Error when reading the %d th byte of the %d th file's owner address!\n",j+1,i+1);
        close(sd);
        return;
      }
    }
    if (-1 == read(sd,&fileOwnerPort,4)){
      close(sd);
      printf("Error when reading the port of the %d th file's owner!\n",i+1);
      return;
    }
    if (isAnInvalidCategory(fileCategory)){
      fileCategoryStr = std::string("error");
    } else {
      fileCategoryStr = getCategoryString(fileCategory);
    }
    printf("%d. Name: %s, description: \"%s\", category: %s, size: %d, id: %d, address: %s, port: %d\n", 
      (i+1), fileName.c_str(), fileDescription.c_str(), fileCategoryStr.c_str(), fileSize, fileId, fileOwnerIp.c_str(), fileOwnerPort);
  }
  close(sd);
}

void clientSearchFileLogic(const cmd::commandResult& command) {
  std::string fileName = command.getStringArgumentValue("file-name"), categoryStr;
  if (fileName == "none") {
    printf("You have to specify the name and the path of the file! (-name option)\n");
    return;
  }
  uint fileChordHash = getHash(fileName.c_str());
  node responsibleNode = serverFindSucc(fileChordHash);
  if (info.me == responsibleNode){
    uint filesCount = chordFiles[fileName].size();
    if (!filesCount){
      printf("There are no files in the network with the specified name!\n");
    } else {
      printf("There are %d files in the network with the specified name: \n", filesCount);
      for (int i=0;i<filesCount;++i){
        categoryStr = getCategoryString(chordFiles[fileName][i].category);
        printf("%d. Name: %s, description: \"%s\", category: %s, size: %d, id: %d, address: %s, port: %d\n", 
          (i+1), fileName.c_str(), chordFiles[fileName][i].description.c_str(), categoryStr.c_str(), 
          chordFiles[fileName][i].size, chordFiles[fileName][i].id, chordFiles[fileName][i].address.c_str(), chordFiles[fileName][i].port);
      }
    }
  } else {
    clientSearchFileLogicAnotherPeer(responsibleNode, fileName);
  }
}

void clientDownloadFileLogic(const cmd::commandResult& command) {
  std::string fileName  = command.getStringArgumentValue("file-name");
  if (fileName == ""){
    printf("[client]You have to specify the name of the file you want to download!\n");
    return;
  }
  int fileId            = command.getNumberArgumentValue("file-id");
  if (fileId == NUM_ARG_MISSING){
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
    printf("The peer shares %d file%s", filesNumber, (filesNumber == 1 ? "" : "s"));
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

void clientRemoveFileLogic(const cmd::commandResult& command) {
  std::string fileName = command.getStringArgumentValue("file-name");
  std::string filePath = command.getStringArgumentValue("file-path");
  if (fileName == "" || filePath == ""){
    printf("You have to specify the name and the path of the file you want to remove!\n");
    return;
  }
  if (!sharedFiles.count(fileName)){
    printf("You don't share any file that has the name %s!\n", fileName.c_str());
    return;
  }
  sharedFileInfo *toRem = NULL;
  int position = -1;
  for (int i=0;i<sharedFiles[fileName].size();++i){
    if (sharedFiles[fileName][i].path == filePath){
      toRem = &sharedFiles[fileName][i];
      position = i;
      break;
    }
  }
  if (!toRem){
    printf("You don't share any file that has the name %s and the path %s!\n", fileName.c_str(), filePath.c_str());
    return;
  }
  if (removeFileFromNetwork(*toRem)){
    sharedFiles[fileName].erase(sharedFiles[fileName].begin()+position);
  }
}

bool checkId(int id){
  return id >= 0 && id <= SHA_HASH_MOD;
}

void printChordSucc(const cmd::commandResult& command) {
  int id = command.getNumberArgumentValue("id");
  if (id == NUM_ARG_MISSING){
    printf("You have to specify an id!\n");
    return;
  }
  node res = serverFindSucc(id);
  std::cout<<res<<'\n';
}

void printChordPred(const cmd::commandResult& command) {
  int id = command.getNumberArgumentValue("id");
  if (id == NUM_ARG_MISSING){
    printf("You have to specify an id!\n");
    return;
  }
  node res = serverFindPred(id);
  std::cout<<res<<'\n';
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
    //std::string errorMsg = std::string("[client]Error when connecting to peer!") + std::string(ipAddress) + std::string(", ") + std::to_string(port);
    //perror(errorMsg.c_str());
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
    notifyMessage("node%u@chord: ", info.me.key);
    std::string inputLine;
    std::getline(std::cin,inputLine);

    cmd::commandResult info = parser.parse(inputLine);
    clientRunning = executeClientCommand(parser, info);
  }
}

bool executeClientCommand(const cmd::commandParser& parser, cmd::commandResult& command){
  switch (command.id){
    case cmd::commandId::ADD_FILE:
      clientAddFileLogic(command);
      break;
    case cmd::commandId::SEARCH_FILE:
      clientSearchFileLogic(command);
      break;
    case cmd::commandId::DOWNLOAD_FILE:
      clientDownloadFileLogic(command);
      break;
    case cmd::commandId::REMOVE_FILE:
      clientRemoveFileLogic(command);
      break;
    case cmd::commandId::LIST_FILES_TO_DOWNLOAD:
      clientListFilesToDownloadLogic(command);
      break;
    case cmd::commandId::LIST_FILES:
      listSharedFiles();
      break;
    case cmd::commandId::LIST_CATEGORIES:
      printFileCategories(command);
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
    case cmd::commandId::CHORD_LIST:
      printThisChordNodeFiles();
      break;
    case cmd::commandId::CHORD_SUCC:
      printChordSucc(command);
      break;
    case cmd::commandId::CHORD_PRED:
      printChordPred(command);
      break;
    case cmd::commandId::CHORD_CLOCKWISE:
      printNodesInClockwiseOrder();
      break;
    case cmd::commandId::CHORD_CCLOCKWISE:
      printNodesInCounterClockwiseOrder();
      break;
    case cmd::commandId::CHORD_CHECK:
      checkSuccPredPointers();
      break;
    case cmd::commandId::LISTALL:
      std::cout<<parser;
      break;
    case cmd::commandId::CLOSE:
      return false;
    case cmd::commandId::CONCURRENT:
      clientConcurrentFunction(command);
      break;
    case cmd::commandId::WOC:
      std::cout<<"Invalid command!\n";
      break;
    case cmd::commandId::WARG:
      std::cout<<"Invalid argument!\n";
      break;
    case cmd::commandId::WCARG:
      std::cout<<"Too few arguments!\n";
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
  parser.addCommandArgumentString(cmd::commandId::ADD_FILE, "file-name");
  parser.addCommandArgumentString(cmd::commandId::ADD_FILE, "file-path");
  parser.addCommandOptionString(cmd::commandId::ADD_FILE, "-description:<fileDescripton>", "");
  parser.addCommandOptionString(cmd::commandId::ADD_FILE, "-category:<fileCategory>", "");

  parser.addCommand(cmd::commandId::SEARCH_FILE, "search", "searches for a file in the network");
  parser.addCommandArgumentString(cmd::commandId::SEARCH_FILE, "file-name");

  parser.addCommand(cmd::commandId::DOWNLOAD_FILE, "download", "downloads a file from a peer");
  parser.addCommandArgumentString(cmd::commandId::DOWNLOAD_FILE, "file-name");
  parser.addCommandArgumentNumber(cmd::commandId::DOWNLOAD_FILE, "file-id");
  parser.addCommandOptionString(cmd::commandId::DOWNLOAD_FILE, "-ip:<peerIP>", "none");
  parser.addCommandOptionNumber(cmd::commandId::DOWNLOAD_FILE, "-port:<peerPort>", -1);

  parser.addCommand(cmd::commandId::REMOVE_FILE, "rm", "removes a file from the network");
  parser.addCommandArgumentString(cmd::commandId::REMOVE_FILE, "file-name");
  parser.addCommandArgumentString(cmd::commandId::REMOVE_FILE, "file-path");

  parser.addCommand(cmd::commandId::LIST_FILES_TO_DOWNLOAD, "list", "requests a peer to send information about all the file it shares");
  parser.addCommandOptionString(cmd::commandId::LIST_FILES_TO_DOWNLOAD, "-ip:<peerIP>", "none");
  parser.addCommandOptionString(cmd::commandId::LIST_FILES_TO_DOWNLOAD, "-name:<fileName>", "");
  parser.addCommandOptionNumber(cmd::commandId::LIST_FILES_TO_DOWNLOAD, "-port:<peerPort>", -1);

  parser.addCommand(cmd::commandId::LIST_FILES, "list-files", "lists all my shared files");
  parser.addCommand(cmd::commandId::LIST_CATEGORIES, "list-categories", "lists the available file categories");

  parser.addCommand(cmd::commandId::CONFIG_ADD_FILE, "config-add-file", "adds an entry to the auto-upload list file");
  parser.addCommandArgumentString(cmd::commandId::CONFIG_ADD_FILE, "file-name");
  parser.addCommandArgumentString(cmd::commandId::CONFIG_ADD_FILE, "file-path");
  parser.addCommandOptionString(cmd::commandId::CONFIG_ADD_FILE, "-description:<fileDescripton>", "");
  parser.addCommandOptionString(cmd::commandId::CONFIG_ADD_FILE, "-category:<fileCategory>", "");

  parser.addCommand(cmd::commandId::CONFIG_REMOVE_FILE, "config-rm-file", "removes an entry from the auto-upload list file");
  parser.addCommandArgumentString(cmd::commandId::CONFIG_REMOVE_FILE, "file-name");

  parser.addCommand(cmd::commandId::CONFIG_REMOVE_ALL, "config-rm-all", "removes all the entries from the auto-upload list file");

  parser.addCommand(cmd::commandId::CONFIG_LIST_FILES, "config-list", "prints all the files that will be shared when the server is starts");

  parser.addCommand(cmd::commandId::CONFIG_AUTO_ADD, "config-auto-add", "sets whether the entries from the auto-upload list have to be shared when the server starts");
  parser.addCommandOptionBoolean(cmd::commandId::CONFIG_AUTO_ADD, "-enable", false);
  parser.addCommandOptionBoolean(cmd::commandId::CONFIG_AUTO_ADD, "-disable", false);

  parser.addCommand(cmd::commandId::CHORD_NODE_INFO, "chord-info", "prints info about this chord node");
  parser.addCommand(cmd::commandId::CHORD_LIST, "chord-list", "prints all the files this chord node is responsible for");

  parser.addCommand(cmd::commandId::CHORD_SUCC, "chord-succ", "prints successor(id) in the Chord network");
  parser.addCommandArgumentNumber(cmd::commandId::CHORD_SUCC, "id");
  
  parser.addCommand(cmd::commandId::CHORD_PRED, "chord-pred", "prints predecessor(id) in the Chord network");
  parser.addCommandArgumentNumber(cmd::commandId::CHORD_PRED, "id"); 

  parser.addCommand(cmd::commandId::CHORD_CLOCKWISE, "chord-clock", "prints the nodes in the Chord network in clockwise order");
  parser.addCommand(cmd::commandId::CHORD_CCLOCKWISE, "chord-cclock", "prints the nodes in the Chord network in counter-clockwise order");
  parser.addCommand(cmd::commandId::CHORD_CHECK, "chord-check", "checks if successor and predecessor pointers are correct for all the nodes in the network");

  parser.addCommand(cmd::commandId::LISTALL, "list-cmd" ,"displays information about all the commands");

  parser.addCommand(cmd::commandId::CLOSE, "close", "closes the application");

  parser.addCommand(cmd::commandId::CONCURRENT, "concurrent-server", "a command that can be used to show that the servers are concurrent");
  parser.addCommandArgumentString(cmd::commandId::CONCURRENT, "peer-ip");
  parser.addCommandArgumentNumber(cmd::commandId::CONCURRENT, "peer-port");
}

void serverConcurrentFunction(int sd) {
  node peer;
  int number;
  if (!readNodeInfo(sd,peer)){
    printf("[server]Failed to read the information about the other peer in the concurrentFunction!\n");
    return;
  }
  std::cout<<"Node "<<peer<<" has connected!\n";
  if (-1 == read(sd,&number,4)){
    printf("[server]Failed to read the number sent by the peer!\n");
    return;
  }
  ++number;
  if (-1 == write(sd,&number,4)){
    printf("[server]Failed to sent the incremented number to the peer!\n");
    return;
  }
}

void clientConcurrentFunction(const cmd::commandResult& command) {
  int peerPort = command.getNumberArgumentValue("peer-port");
  if (peerPort == -1){
    printf("[client]You have to specify the port of the peer!\n");
    return;
  }
  std::string peerIp = command.getStringArgumentValue("peer-ip");
  if (peerIp == "none"){
    printf("[client]You have to specify the ip of the peer!\n");
    return;
  }
  uint request = SRV_CONCURRENT;
  int sd;
  if (!initClient(sd,peerIp.c_str(),peerPort)){
    printf("[client]Failed to connect to the other peer!\n");
    return;
  }
  if (-1 == write(sd,&request,4) || !sendNodeInfo(sd,info.me)) {
    printf("[client]Failed to send protocol data in the client concurrent function!\n");
    close(sd);
    return;
  }
  int number;
  std::cout<<"Number = ";
  std::cin>>number;
  if (-1 == write(sd,&number,4)){
    printf("[client]Failed to send protocol data in the client concurrent function!\n");
    close(sd);
    return;
  }
  if (-1 == read(sd,&number,4)){
    printf("[client]Failed to read the number sent by the peer!\n");
    close(sd);
    return;
  }
  std::cout<<"Server sent: "<<number<<'\n';
  close(sd);
}
