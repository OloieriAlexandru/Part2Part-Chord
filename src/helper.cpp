#include "helper.h"

void debugMessage(const char* format, ...){
    if (!DEBUG){
        return;
    }
    va_list args;
    va_start(args, format);
	printf("\nChord node %u: ", info.me.key);
    vprintf(format, args);
	va_end(args);
}

uint getHash(SHA1& sha1, const char *str){
    std::string hsh = sha1(str);
    uint res = 0, pw = 1, mul;
    for (int i=hsh.size()-1;i>=0;--i){
        mul = (hsh[i] >= '0' && hsh[i] <= '9' ? hsh[i]-'0':hsh[i]-'a'+10);
        res = ((ulld)1 * pw * mul + res) & SHA_HASH_MOD;
        pw = (pw * 16) & SHA_HASH_MOD;
    }
    return res;
}

uint getCustomHash(const char* str){
    if (!str){
        return 0u;
    }
    ulld res = 0u;
    uint index = 0u, basePow = 1u;
    while (str[index]){
        res = (res + (ulld)1 * basePow * str[index]) % CUSTOM_HASH_MOD;
        basePow = ((ulld)1 * basePow * CUSTOM_HASH_BASE) % CUSTOM_HASH_MOD;
        ++index;
    }
    return (uint)res;
}

uint getFileSize(const char* filePath) {
    struct stat st;
    stat(filePath, &st);
    return st.st_size;
}

bool fileCreate(const char* fileName){
    if (-1 == creat(fileName, S_IRWXU)){
        return false;
    }
    return true;
}

bool fileExists(const char* filePath) {
    if (!filePath || access(filePath, F_OK) == -1){
        return false;
    }
    return true;
}

bool configFileGetFlagValue() {
    std::ifstream fileIn(userConfigFilePath);
    if (!fileIn.is_open()){
        printf("Error when opening the config file for reading!\n");
        return false;
    }
    int autoAdd;
    fileIn >> autoAdd;
    fileIn.close();
    return (autoAdd == 1);
}

bool configFileExists() {
    return fileExists(userConfigFilePath);
}

bool configFileInitInfo() {
    std::ofstream fileOut(userConfigFilePath);
    if (fileOut) {
        fileOut<<"0\n";
        fileOut.close();
    } else {
        std::cout<<"Error when opening the config file for writing!\n";
        return false;
    }
    return true;
}

bool configFileCreate() {
    return fileCreate(userConfigFilePath);
}

bool configFileInit() {
    if (!configFileExists()){
        if (!configFileCreate()){
            return false;
        }
        return configFileInitInfo();
    }
    return true;
}

void configFileAddEntry(const cmd::commandResult& command) {
    std::string fileName = command.getStringOptionValue("-name");
    std::string filePath = command.getStringOptionValue("-path");
    if (fileName == "none" || filePath == "none"){
        printf("You have to specify the name and the path of the file! (-name and -path options)\n");
        return;
    }
    if (!fileExists(filePath.c_str())){
        printf("The file doesn't exist!\n");
        return;
    }
    std::ofstream fileOut(userConfigFilePath, std::fstream::app);
    if (!fileOut.is_open()){
        printf("Error when opening the config file for reading!\n");
        return;
    }
    fileOut<<fileName<<' '<<filePath<<'\n';
    fileOut.close();
}

void configFileRemoveEntry(const cmd::commandResult& command) {
    std::string fileName = command.getStringOptionValue("-name");
    if (fileName == "none"){
        printf("You have to specify the name of the file!\n");
        return;
    }
    std::string name, path;
    std::vector<std::pair<std::string,std::string>> entries;
    int autoAdd;
    std::ifstream fileIn(userConfigFilePath);
    if (!fileIn.is_open()){
        printf("Error when opening the file for reading!\n");
        return;
    }
    fileIn >> autoAdd;
    while (fileIn >> name >> path){
        if (name != fileName){
            entries.push_back(make_pair(name, path));
        }
    }
    fileIn.close();
    std::ofstream fileOut(userConfigFilePath);  
    if (!fileOut.is_open()){
        printf("Error when opening the file for writing!\n");
        return;
    }
    fileOut<<autoAdd<<'\n';
    for (int i=0;i<entries.size();++i){
        fileOut << entries[i].first<< ' ' << entries[i].second<<'\n';
    }
    fileOut.close();
}

void configFileRemoveAll(const cmd::commandResult& command) {
    int autoAdd;
    std::ifstream fileIn(userConfigFilePath);
    if (!fileIn.is_open()){
        printf("Error when opening the file for reading!\n");
        return;
    }
    fileIn >> autoAdd;
    fileIn.close();
    std::ofstream fileOut(userConfigFilePath);
    if (!fileOut.is_open()){
        printf("Error when opening the file for writing!\n");
        return;
    }
    fileOut<<autoAdd<<'\n';
    fileOut.close();
}

void configFileListAll(const cmd::commandResult& command) {
    std::ifstream fileIn(userConfigFilePath);
    if (!fileIn.is_open()){
        printf("Error when opening the file for reading!\n");
        return;
    }
    int autoAdd, index = 1;
    std::string name, path;
    fileIn >> autoAdd;
    while (fileIn >> name >> path){
        std::cout << index++ << ". " << name << ' ' << path<<'\n';
    }
    fileIn.close();
}

void configFileAutoAdd(const cmd::commandResult& command) {
    int fd = open(userConfigFilePath, O_RDWR);
    bool enabled = command.getBooleanOptionValue("-enable");
    bool disabled = command.getBooleanOptionValue("-disable");
    if (!(enabled || disabled)) {
        printf("You have to specify one of the two options: enable/disable!\n");
        return;
    }
    if (-1 == fd){
        printf("Error when opening the file!\n");
        return;
    }
    char wr = '0';
    if (enabled){
        wr = '1';
    }
    if (-1 == write(fd, &wr, sizeof(wr))){
        printf("Error when writing to the file!\n");
        return;
    }
    close(fd);
}
