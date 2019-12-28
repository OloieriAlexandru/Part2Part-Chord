#ifndef HELPER_H
#define HELPER_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <string>

#include "constants.h"
#include "command-line.h"

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