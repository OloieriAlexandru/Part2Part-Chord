#ifndef CONSTANTS_H
#define CONSTANTS_H

#define SHA_HASH_BITS   20
#define SHA_HASH_MOD    (1u << HASH_BITS ) - 1

#define CUSTOM_HASH_MOD     99993 // 1000000009u
#define CUSTOM_HASH_BASE    131u

#define uchar   unsigned char
#define uint    unsigned int
#define ulld    unsigned long long

const char* const userConfigFilePath    = ".user.config";
const char* const userDownloadsFolder   = "downloads/";

#endif // CONSTANTS_H