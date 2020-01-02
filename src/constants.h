#ifndef CONSTANTS_H
#define CONSTANTS_H

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

#define uchar   unsigned char
#define uint    unsigned int
#define ulld    unsigned long long

const char* const userConfigFilePath        = ".user.config";
const char* const userDownloadsFolder       = "downloads/";
const char* const userDownloadsExtension    = ".txt";

#endif // CONSTANTS_H
