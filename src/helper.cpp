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

void notifyMessage(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("\033[1;36m");
    vprintf(format, args);
	va_end(args);
    printf("\033[0m");
}

void initFileSharingInfo() {
    fileCategories.push_back(std::string("any"));
    fileCategories.push_back(std::string("text"));
    fileCategories.push_back(std::string("image"));
    fileCategories.push_back(std::string("code"));
    fileCategories.push_back(std::string("video"));
    fileCategories.push_back(std::string("program"));
    fileCategories.push_back(std::string("audio"));
    fileCategories.push_back(std::string("binary"));
}

bool isAnInvalidCategory(const std::string& commandName){
    return isAnInvalidCategory(getCategoryId(commandName));
}

bool isAnInvalidCategory(uchar categoryId) {
    return categoryId == fileCategories.size();
}

uchar getCategoryId(const std::string& commandName) {
    if (!commandName.size()){
        return (uchar)0;
    }
    for (int i=0;i<fileCategories.size();++i){
        if (commandName == fileCategories[i]){
            return (uchar)i;
        }
    }
    return (uchar)fileCategories.size();
}

std::string getGeneralCategory(){
    return fileCategories[0];
}

std::string getCategoryString(uchar categoryId) {
    if (categoryId < fileCategories.size()){
        return fileCategories[(int)categoryId];
    }
    return "invalid";
}

void printFileCategories(const cmd::commandResult& command){
    printf("There are %d file categories:\n", (int)fileCategories.size()-1);
    for (int i=1;i<fileCategories.size();++i){
        printf("%d. %s\n", i, fileCategories[i].c_str());
    }
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

void readConfigFileDescription(std::ifstream& fileIn, std::string& description) {
    int descLen;
    fileIn >> descLen;
    fileIn.get();
    description.resize(descLen);
    for (int i=0;i<descLen;++i){
        description[i] = fileIn.get();
    }
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
    std::string fileName = command.getStringArgumentValue("file-name");
    std::string filePath = command.getStringArgumentValue("file-path");
    if (fileName == "" || filePath == ""){
        printf("You have to specify the name and the path of the file!\n");
        return;
    }
    if (!fileExists(filePath.c_str())){
        printf("The file doesn't exist!\n");
        return;
    }
    std::string description = command.getStringOptionValue("-description");
    if (description.size() > FILE_DESC_MAX_LEN){
        std::cout<<"The description is too long! Max characters allowed: "<<FILE_DESC_MAX_LEN<<"!\n";
        return;
    }
    std::string category = command.getStringOptionValue("-category");
    if (isAnInvalidCategory(category)){
        std::cout<<"Invalid category!\n";
        return;
    }
    std::ofstream fileOut(userConfigFilePath, std::fstream::app);
    if (!fileOut.is_open()){
        printf("Error when opening the config file for reading!\n");
        return;
    }
    if (description == ""){
        description = fileEmptyDescription;
    }
    if (category == ""){
        category = getGeneralCategory();
    }
    fileOut<<fileName<<' '<<filePath<<' '<< description.size() << ' ' << description<<' '<<category<<'\n';
    fileOut.close();
}

void configFileRemoveEntry(const cmd::commandResult& command) {
    std::string fileName = command.getStringArgumentValue("file-name");
    if (fileName == "none"){
        printf("You have to specify the name of the file!\n");
        return;
    }
    std::string name, path, description, category;
    std::vector<std::pair<std::pair<std::string,std::string>, std::pair<std::string,std::string>>> entries;
    int autoAdd;
    std::ifstream fileIn(userConfigFilePath);
    if (!fileIn.is_open()){
        printf("Error when opening the file for reading!\n");
        return;
    }
    fileIn >> autoAdd;
    while (fileIn >> name >> path){
        readConfigFileDescription(fileIn, description);
        fileIn >> category;
        if (name != fileName){
            entries.push_back(make_pair(make_pair(name, path),make_pair(description, category)));
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
        fileOut << entries[i].first.first<< ' ' << entries[i].first.second<<' '<< entries[i].second.first << ' ' << entries[i].second.second <<'\n';
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
    std::string name, path, description, category;
    fileIn >> autoAdd;
    while (fileIn >> name >> path){
        readConfigFileDescription(fileIn, description);
        fileIn >> category;
        std::cout << index++ << ". " << "Name: " << name << ", path: " << path << ", description: \"" << description << "\", category: " << category <<'\n';
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
