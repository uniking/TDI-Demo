#ifndef _HASH_FID_
#define  _HASH_FID_

#define HASH_BUFFER_LENGTH	10240
#define HASH_MAGIC	10007

#include <ndis.h>

typedef struct _SMB_READ_INFOR
{
	BOOLEAN isFirstRead;
	ULONGLONG Offset;
	ULONG DataLength;
	//BOOLEAN encryptData;//报文没有请求头，只是数据

	ULONGLONG FirstOffset;//存储请求的偏移
	ULONG FirstLength;

	//修改请求相关
	BOOLEAN bChangeRequest;
	ULONGLONG requestOffset;
	ULONG requestDataLength;
}SMB_READ_INFOR, *PSMB_READ_INFOR;

typedef struct _SMB_WRITE_INFOR
{
	ULONGLONG Offset;
	ULONG DataLength;
	//BOOLEAN decryptData;
}SMB_WRITE_INFOR, *PSMB_WRITE_INFOR;

typedef struct _FID_NET_FILE
{
	BOOLEAN valide;
	USHORT Fid;
	BOOLEAN isEncryptFile;
	//BOOLEAN WriteDecrypt; //处理同一个文件的不同偏移
	//BOOLEAN ReadEncrypt;
	UNICODE_STRING FileName;

	SMB_READ_INFOR read;
	SMB_WRITE_INFOR write;
	UCHAR FileHeader[512];
	UCHAR SendBuffer[512];//缓存数据
	UINT SendBufferLen;//缓存长度
	UCHAR ReceiveBuffer[512];//缓存数据
	UINT ReceiveBufferLen;//缓存长度
}FID_NET_FILE, *PFID_NET_FILE;

BOOLEAN hfInsertFid(PFID_NET_FILE FidNode);
PFID_NET_FILE hfFindFid(USHORT Fid);
BOOLEAN hfDeleteFid(USHORT Fid);


#endif

