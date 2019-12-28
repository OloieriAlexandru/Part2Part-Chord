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

bool check(int argc, char* argv[]);
void shareFilesFromConfigFile();

nodeInfo                                        info;
umap<std::string,std::vector<sharedFileInfo>>   sharedFiles;

void          shareFile(const std::string& name, const std::string& path, bool atInit = false);
void          shareFile(const cmd::commandResult& command);
void          listSharedFiles();

static void*  serverLogic(void *);
int           createServer();

bool          executeClientCommand(cmd::commandResult& command);
void          initCmd(cmd::commandParser& parser);

static void*  treat(void *);

void clientLogic();

int main(int argc, char* argv[]) {
  if (!check(argc, argv)){
    return 1;
  }

  if (!configFileInit()) {
    fprintf(stderr, "[main]Error when creating/checking the config file!");
    return 1;
  }
  shareFilesFromConfigFile();

  pthread_t serverThread;
  if (pthread_create(&serverThread, NULL, serverLogic, NULL)){
    perror("[main]Error when creating main thread for the server!");
    return 1;
  }

  clientLogic();
  return 0;
};

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

void shareFile(const std::string& name, const std::string& path, bool atInit) {
  if (!fileExists(path.c_str())) {
    if (!atInit){
      std::cout<<"Invalid path! The file doesn't exist!\n";
    } else {
      std::cout<<"Error when loading the file "<<path<<", the file is not located there anymore!\n";
    }
    return;
  }
  sharedFiles[name].push_back(sharedFileInfo(name, path));
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
      printf("%d. name:%s path:%s\n", fileNumber++, file.name.c_str(), file.path.c_str());
    }
  }
}

static void* serverLogic(void *) {
	pthread_detach(pthread_self());

  struct sockaddr_in from;
  int sd = createServer();
  bool running = true;

  if (listen(sd, 5) == -1){
    perror("[server]Error when calling listen() in server!");
  }

  pthread_t cv;

  while (running){
    int client;
    memset(&from, 0, sizeof(from));
    uint length = sizeof(from);
    if ((client = accept (sd, (struct sockaddr *) &from, &length)) < 0){
      perror ("[server]Error when calling accept()!");
      continue;
    }
    pthread_create(&cv, NULL, treat, &client);
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
    case SRV_FIND_SUCCESSOR:

      break;
    case SRV_GET_SUCCESSOR:

      break;
    case SRV_GET_PREDECESSOR:

      break;
    case SRV_ADD_FILE:

      break;
    default:
      break;
  }

  if (-1 == close(sd)){
    perror("[server]Error when calling close() for a client socket descriptor!");
    return NULL;
  }
  return NULL;
};

void clientLogic() {
  bool clientRunning = true;

  cmd::commandParser parser;
  initCmd(parser);

  while (clientRunning){
    std::string inputLine;
    std::getline(std::cin,inputLine);

    cmd::commandResult info = parser.parse(inputLine);
    clientRunning = executeClientCommand(info);
  }
}

bool executeClientCommand(cmd::commandResult& command){
  switch (command.id){
    case cmd::commandId::ADD_FILE:
      shareFile(command);
      break;
    case cmd::commandId::SEARCH_FILE:
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
    case cmd::commandId::LISTALL:
      break;
    case cmd::commandId::CLOSE:
      return false;
      break;
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
  parser.addCommand(cmd::commandId::ADD_FILE, "add-file", "adds a file");
  parser.addCommandOptionString(cmd::commandId::ADD_FILE, "-name:<fileNameInChord>", "none");
  parser.addCommandOptionString(cmd::commandId::ADD_FILE, "-path:<filePath>", "none");

  parser.addCommand(cmd::commandId::SEARCH_FILE, "search-file", "searches a file");
  parser.addCommandOptionString(cmd::commandId::SEARCH_FILE, "-name:<fileName>", "none");

  parser.addCommand(cmd::commandId::LIST_FILES, "list-files", "list all the shared files");

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

  parser.addCommand(cmd::commandId::LISTALL, "list" ,"displays information about all the commands");

  parser.addCommand(cmd::commandId::CLOSE, "close", "closes the application");
}