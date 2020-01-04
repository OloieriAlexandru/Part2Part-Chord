#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

#define DEBUG           0

#define THREADS_COUNT   120
#define THREAD_EXIT     -123456

#define SHA_HASH_BITS   10
#define SHA_HASH_VAL    (1u << SHA_HASH_BITS)
#define SHA_HASH_MOD    (1u << SHA_HASH_BITS) - 1

#define CUSTOM_HASH_MOD     99993 // 1000000009u
#define CUSTOM_HASH_BASE    131u

#define PACKAGE_SIZE        1024u
#define CHORD_FIRST_PORT    3500

#define FILE_MIN_SIZE       0
#define FILE_MAX_SIZE       2000000000
#define FILE_DESC_MAX_LEN   100

#define uchar   unsigned char
#define uint    unsigned int
#define ulld    unsigned long long
#define umap    std::unordered_map

const char* const userConfigFilePath        = ".user.config";
const char* const userDownloadsFolder       = "downloads/";
const char* const userDownloadsExtension    = ".txt";

const std::string myAddress                 = "127.0.0.1";
const std::string fileEmptyDescription      = "none";

#endif // CONSTANTS_H
