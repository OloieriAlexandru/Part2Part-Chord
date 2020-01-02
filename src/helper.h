#ifndef HELPER_H
#define HELPER_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <fstream>
#include <string>
#include <iostream>

#include "hash/sha1.h"

#include "constants.h"
#include "command-line.h"

#include "chord.h"

struct  threadInfo {
    int sd;
    int threadNo;
};

void    debugMessage(const char* format, ...);

uint    getHash(SHA1& sha1, const char* str);
uint    getCustomHash(const char* str);

uint    getFileSize(const char* filePath);
bool    fileCreate(const char* fileName);
bool    fileExists(const char* filePath);

bool    configFileGetFlagValue();
bool    configFileExists();
bool    configFileInitInfo();
bool    configFileCreate();
bool    configFileInit();
bool    configFileAddEntry(const std::string& fileName, const std::string& filePath);
bool    configFileRemoveEntry(const std::string& fileName);

void    configFileAddEntry(const cmd::commandResult& command);
void    configFileRemoveEntry(const cmd::commandResult& command);
void    configFileRemoveAll(const cmd::commandResult& command);
void    configFileListAll(const cmd::commandResult& command);
void    configFileAutoAdd(const cmd::commandResult& command);

#endif // HELPER_H
