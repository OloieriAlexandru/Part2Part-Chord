#ifndef DEFINES_H
#define DEFINES_H

// The ids of the operations the server can execute + information about what will be sent by the client

#define     SRV_ERROR                       0                       // ---
//          -
#define     SRV_FIND_SUCCESSOR              1                       // ---
//          uint                            - id
#define     SRV_GET_SUCCESSOR               2                       // ---
//          -
#define     SRV_GET_PREDECESSOR             3                       // ---
//          -
#define     SRV_ADD_FILE                    4                       // ---
//          uint                            - fileNameLen
//          char[fileNameLen]               - fileName
//          uint                            - ownerPort
//          uint                            - ownerAddressLen
//          char[ownerAddressLen]          - ownerAddress     
#define     SRV_DOWNLOAD_FILE               10                      // ---
//          client:
//          uint                            - fileNameLen;
//          char[fileNameLen]               - fileName;
//          uint                            - fileId
//          server:
#define     SRV_DOWNLOAD_FILE_NOT_EXISTS    100
//          client:
//          endOperation
#define     SRV_DOWNLOAD_NOT_AVAILABLE      101
//          client:
//          endOperation
#define     SRV_DOWNLOAD_FILE_OK_BEGIN      102
//          uint                            - numberOfPackages (1024 bytes each)
//          numberOfPackages times:
//          uint                            - packageSize
//          char[packageSize]               - packageInfo
#define     SRV_LIST_FILES                  11                      // ---
//          client:
//          uint                            - fileNameLen
//          char[fileNameLen]               - fileName
//         *if fileNameLen == 0, send info about all my files
//          server:
//          uint                            - filesNumber
//          filesNumber times:
//          uint                            - fileNameLen
//          char[fileNameLen]               - fileName
//          uint                            - fileId (needed when you download a file)

#endif // DEFINES_H