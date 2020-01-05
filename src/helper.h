#ifndef HELPER_H
#define HELPER_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdarg.h>

#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <mutex>
#include <ctime>

#include "hash/sha1.h"

#include "constants.h"
#include "command-line.h"
#include "chord.h"

struct  threadInfo {
    int sd;
    int threadNo;
};

class guardLock {
private:
    std::mutex& m;
public:
    guardLock(std::mutex& mt): m(mt){ 
        m.lock();
    }
    ~guardLock(){
        m.unlock();
    }
};

extern std::vector<std::string> fileCategories;

void    debugMessage(const char* format, ...);
void    notifyMessage(const char* format, ...);

void        initFileSharingInfo();
bool        isAnInvalidCategory(const std::string& commandName);
bool        isAnInvalidCategory(uchar categoryId);
uchar       getCategoryId(const std::string& commandName);
std::string getCategoryString(uchar categoryId);
std::string getGeneralCategory();
void        printFileCategories(const cmd::commandResult& command);

uint    getHash(SHA1& sha1, const char* str);
uint    getCustomHash(const char* str);

uint    getFileSize(const char* filePath);
bool    fileCreate(const char* fileName);
bool    fileExists(const char* filePath);
bool    isDirectory(const char* path);

void        readConfigFileDescription(std::ifstream& fileIn, std::string& description);
bool        configFileGetFlagValue();
bool        configFileExists();
bool        configFileInitInfo();
bool        configFileCreate();
bool        configFileInit();
bool        configFileAddEntry(const std::string& fileName, const std::string& filePath);
bool        configFileRemoveEntry(const std::string& fileName);

void    configFileAddEntry(const cmd::commandResult& command);
void    configFileRemoveEntry(const cmd::commandResult& command);
void    configFileRemoveAll(const cmd::commandResult& command);
void    configFileListAll(const cmd::commandResult& command);
void    configFileAutoAdd(const cmd::commandResult& command);

bool        readSpecialStringFromFile(std::istream& fileIn, std::string& str);

bool        historyFileExists();
bool        historyFileCreate();
bool        historyFileInit();
void        addDownloadedFileToHistory(const std::string& fileName, const std::string& filePath);

void        printDownloadHistory(const cmd::commandResult& command);
void        printDownloadsFolderFiles(const cmd::commandResult& command);
void        removeFileFromDownloadsFolder(const cmd::commandResult& command);

#endif // HELPER_H
