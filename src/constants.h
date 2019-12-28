#ifndef CONSTANTS_H
#define CONSTANTS_H

#define HASH_BITS   20
#define HASH_MOD    (1u << HASH_BITS ) - 1

#define uchar   unsigned char

const char* const userConfigFilePath = ".user.config";

#endif // CONSTANTS_H