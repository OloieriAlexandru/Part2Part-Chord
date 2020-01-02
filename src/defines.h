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
#define     SRV_FIND_PREDECESSOR            4                       // ---
//          uint                            - id
#define     SRV_UPDATE_PREDECESSOR          5                       // ---
//          uint                            - predId
//          uint                            - predPort
//          uint                            - predIpLen
//          char[predIpLen]                 - predIp
#define     SRV_UPDATE_FINGERS_TABLE        6                       // ---
//          uint                            - nodeId
//          uint                            - nodePort
//          uint                            - nodeIpLen
//          char[nodeIpLen]                 - nodeIp
//          uint                            - fingerIndex
#define     SRV_UPDATE_SUCCESSOR            7                       // ---
//          uint                            - succId
//          uint                            - succPort
//          uint                            - succIpLen
//          char[succIpLen]                 - succIp
#define     SRV_STABILIZATION               8
//          uint                            - nodeId
//          uint                            - nodePort
//          uint                            - nodeIpLen
//          char[nodeIpLen]                 - nodeIp
#define     SRV_ADD_FILE                    9                       // ---
//          client:
//          uint                            - fileNameLen
//          char[fileNameLen]               - fileName
//          uint                            - fileChordId
//          uint                            - fileId
//          uint                            - ownerAddressLen
//          char[ownerAddressLen]           - ownerAddress
//          uint                            - ownerPort
//          server:
#define     SRV_ADD_FILE_ALREADY_EXISTS     98
#define     SRV_ADD_FILE_OK_ADDED           99
#define     SRV_DOWNLOAD_FILE               10                      // ---
//          client:
//          uint                            - fileNameLen;
//          char[fileNameLen]               - fileName;
//          uint                            - fileId
//          server:
#define     SRV_DOWNLOAD_FILE_NOT_EXISTS    100
//          client:
//          endOperation
#define     SRV_DOWNLOAD_FILE_NOT_AVAILABLE 101
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
#define     SRV_SEARCH_FILE                 12                      // ---
//          client:
//          uint                            - fileNameLen
//          char[fileNameLen]               - fileName
//          server:
//          uint                            - numberOfFiles
//          numberOfFiles times:
//          uint                            - fileId
//          uint                            - fileOwnerIpLen
//          char[fileIpLen]                 - fileOwnerIp
//          uint                            - fileOwnerPort
#define     SRV_REMOVE_FILE                 13                      // ---
//          client:
//          uint                            - fileNameLen
//          char[fileNameLen]               - fileName
//          uint                            - fileChordId
//          uint                            - fileId
//          uint                            - ownerAddressLen
//          char[ownerAddressLen]           - ownerAddress
//          uint                            - ownerPort
//          server:
#define     SRV_REMOVE_FILE_NOT_FOUND       103
#define     SRV_REMOVE_FILE_OK_REMOVED      104
#define     SRV_GET_MY_KEYS                 14                      // ---
//          client:
//          uint                            - successorKey
//          server:
//          SRV_ERROR
#define     SRV_GET_MY_KEYS_OK              105
//          uint                            - numberOfFiles
//          numberOfFiles times:
//          uint                            - fileNameLen
//          char[fileNameLen]               - fileName
//          uint                            - fileChordId
//          uint                            - fileId
//          uint                            - ownerAddressLen
//          char[ownerAddressLen]           - ownerAddress
//          uint                            - ownerPort
#define     SRV_NODE_LEFT                   15                      // ---
//          uint                            - nodeLeftId
//          uint                            - nodeLeftPort
//          uint                            - nodeLeftIpLen
//          char[nodeLeftIpLen]             - nodeLeftIp
//          uint                            - nodeReplacerId
//          uint                            - nodeReplacerPort
//          uint                            - nodeReplacerIpLen
//          char[nodeReplacerIpLen]         - nodeReplacerIp
#define     SRV_ADD_PREDECESSOR_FILES       16                      // ---
//          client:
//          uint                            - predId
//          uint                            - predPort
//          uint                            - predIpLen
//          char[predIpLen]                 - predIp
//          server:
#define     SRV_ADD_PREDECESSOR_FILES_NO    106
#define     SRV_ADD_PREDECESSOR_FILES_OK    107
//          if (107) client:
//          uint                            - numberOfFiles
//          numberOfFiles times:
//          uint                            - fileNameLen
//          char[fileNameLen]               - fileName
//          uint                            - fileChordId
//          uint                            - fileId
//          uint                            - ownerAddressLen
//          char[ownerAddressLen]           - ownerAddress
//          uint                            - ownerPort
#define     SRV_CONCURRENT                   255                    // ---
//          client:
//          uint                            - nodeId
//          uint                            - nodePort
//          uint                            - nodeIpLen
//          char[nodeIpLen]                 - nodeIp
//          int                             - number
//          server:
//          int                             - number + 1

#endif // DEFINES_H
