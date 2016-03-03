#include <ntddk.h>

#define  FS_PWD_SIZE 16
ULONG
FsKeyProcess(UCHAR* KeyOut, UCHAR* KeyIn, ULONG KeyLength);

ULONG
FsDataProcess(UCHAR* Key, UCHAR* Data, ULONG DataLength);

ULONG
FsEnDecryptData(UCHAR* Data, ULONG DataLength, UCHAR* Key, UCHAR* Buffer);