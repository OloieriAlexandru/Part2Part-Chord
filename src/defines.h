#ifndef DEFINES_H
#define DEFINES_H

// The ids of the operations the server can execute + information about what will be sent by the client

#define     SRV_FIND_SUCCESSOR      1
//          uint                    - id
#define     SRV_GET_SUCCESSOR       2
//          -
#define     SRV_GET_PREDECESSOR     3
//          -
#define     SRV_ADD_FILE            4
//          uint                    - fileNameLen
//          char[fileNameLen]       - fileName
//          uint                    - ownerPort
//          uint                    - ownerAddressLen
//          uchar[ownerAddressLen]  - ownerAddress        

#endif // DEFINES_H