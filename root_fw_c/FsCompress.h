#include <ntddk.h>
#include "../Compress/include/lzo/lzoconf.h"
#include "../Compress/include/lzo/lzo1x.h"

#define Tagi 'x00x'

typedef unsigned char       BYTE;
typedef unsigned long DWORD;
typedef unsigned short      WORD;
typedef int                 BOOL;

typedef BYTE _BYTE; 
typedef DWORD _DWORD;
typedef WORD _WORD;

int FsDataCompress62(BYTE* pucData, 
					 DWORD dwLength, 
					 BYTE* pucCompress, 
					 DWORD* pdwCompressLength);

int FsDataDecompress62(BYTE* pucCompress, 
					   DWORD dwCompressLength, 
					   BYTE* pucData, 
					   DWORD* pdwLength);

signed int FsDataDecompress6364(BYTE* pucCompress, DWORD dwCompressLength, BYTE* pucData, DWORD* pdwLength);
int FsDataCompress6364(BYTE* pucData, DWORD dwDataLength, BYTE* pucCompress, DWORD* pdwCompressLength);

int FsDataCompressDo62(BYTE* pucData, 
					   DWORD dwLength, 
					   BYTE* pucCompress, 
					   DWORD* pdwCompressLength, 
					   BYTE* pucBuffer);

BYTE* AnalyseFileType6364(BYTE* pucData, DWORD dwLength, int nMaxLength, DWORD* pdwCount);