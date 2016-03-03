#ifndef __PACKET_SMB_H__
#define __PACKET_SMB_H__
//#include <ndis.h>
#include "stdafx.h"
#include "hash_fid.h"

/* SMB command codes, from the SNIA CIFS spec. With MSVC and a
 * libwireshark.dll, we need a special declaration.
 */

#define SMB_COM_CREATE_DIRECTORY		0x00
#define SMB_COM_DELETE_DIRECTORY		0x01
#define SMB_COM_OPEN				0x02
#define SMB_COM_CREATE				0x03
#define SMB_COM_CLOSE				0x04
#define SMB_COM_FLUSH				0x05
#define SMB_COM_DELETE				0x06
#define SMB_COM_RENAME				0x07
#define SMB_COM_QUERY_INFORMATION		0x08
#define SMB_COM_SET_INFORMATION			0x09
#define SMB_COM_READ				0x0A
#define SMB_COM_WRITE				0x0B
#define SMB_COM_LOCK_BYTE_RANGE			0x0C
#define SMB_COM_UNLOCK_BYTE_RANGE		0x0D
#define SMB_COM_CREATE_TEMPORARY		0x0E
#define SMB_COM_CREATE_NEW			0x0F
#define SMB_COM_CHECK_DIRECTORY			0x10
#define SMB_COM_PROCESS_EXIT			0x11
#define SMB_COM_SEEK				0x12
#define SMB_COM_LOCK_AND_READ			0x13
#define SMB_COM_WRITE_AND_UNLOCK		0x14
#define SMB_COM_READ_RAW			0x1A
#define SMB_COM_READ_MPX			0x1B
#define SMB_COM_READ_MPX_SECONDARY		0x1C
#define SMB_COM_WRITE_RAW			0x1D
#define SMB_COM_WRITE_MPX			0x1E
#define SMB_COM_WRITE_MPX_SECONDARY		0x1F
#define SMB_COM_WRITE_COMPLETE			0x20
#define SMB_COM_QUERY_SERVER			0x21
#define SMB_COM_SET_INFORMATION2		0x22
#define SMB_COM_QUERY_INFORMATION2		0x23
#define SMB_COM_LOCKING_ANDX			0x24
#define SMB_COM_TRANSACTION			0x25
#define SMB_COM_TRANSACTION_SECONDARY		0x26
#define SMB_COM_IOCTL				0x27
#define SMB_COM_IOCTL_SECONDARY			0x28
#define SMB_COM_COPY				0x29
#define SMB_COM_MOVE				0x2A
#define SMB_COM_ECHO				0x2B
#define SMB_COM_WRITE_AND_CLOSE			0x2C
#define SMB_COM_OPEN_ANDX			0x2D
#define SMB_COM_READ_ANDX			0x2E
#define SMB_COM_WRITE_ANDX			0x2F
#define SMB_COM_NEW_FILE_SIZE			0x30
#define SMB_COM_CLOSE_AND_TREE_DISC		0x31
#define SMB_COM_TRANSACTION2			0x32
#define SMB_COM_TRANSACTION2_SECONDARY		0x33
#define SMB_COM_FIND_CLOSE2			0x34
#define SMB_COM_FIND_NOTIFY_CLOSE		0x35
/* Used by Xenix/Unix		0x60-0x6E */
#define SMB_COM_TREE_CONNECT			0x70
#define SMB_COM_TREE_DISCONNECT			0x71
#define SMB_COM_NEGOTIATE			0x72
#define SMB_COM_SESSION_SETUP_ANDX		0x73
#define SMB_COM_LOGOFF_ANDX			0x74
#define SMB_COM_TREE_CONNECT_ANDX		0x75
#define SMB_COM_QUERY_INFORMATION_DISK		0x80
#define SMB_COM_SEARCH				0x81
#define SMB_COM_FIND				0x82
#define SMB_COM_FIND_UNIQUE			0x83
#define SMB_COM_FIND_CLOSE			0x84
#define SMB_COM_NT_TRANSACT			0xA0
#define SMB_COM_NT_TRANSACT_SECONDARY		0xA1
#define SMB_COM_NT_CREATE_ANDX			0xA2
#define SMB_COM_NT_CANCEL			0xA4
#define SMB_COM_NT_RENAME			0xA5
#define SMB_COM_OPEN_PRINT_FILE			0xC0
#define SMB_COM_WRITE_PRINT_FILE		0xC1
#define SMB_COM_CLOSE_PRINT_FILE		0xC2
#define SMB_COM_GET_PRINT_QUEUE			0xC3
#define SMB_COM_READ_BULK			0xD8
#define SMB_COM_WRITE_BULK			0xD9
#define SMB_COM_WRITE_BULK_DATA			0xDA

/* Error codes */

#define SMB_SUCCESS 0x00  /* All OK */
#define SMB_ERRDOS  0x01  /* DOS based error */
#define SMB_ERRSRV  0x02  /* server error, network file manager */
#define SMB_ERRHRD  0x03  /* Hardware style error */
#define SMB_ERRCMD  0x04  /* Not an SMB format command */

/*File Attributes*/
#define SMB_READ_ONLY		0x1
#define SMB_HIDDEN_FILE		0x2
#define SMB_SYSTEM_FILE	0x4
#define SMB_VOLUME_ID		0x8
#define SMB_DIRECTORY_FILE	0x10
#define SMB_ARCHIVE			0x20
#define SMB_DEVICE_FILE	0x40
#define SMB_NORMAL			0x80
#define SMB_TEMPORARY_FILE	0x100
#define SMB_SPARSE_FILE	0x200
#define SMB_REPARSE_POINT	0x400
#define SMB_COMPRESSED_FILE	0x800
#define SMB_OFFLINE			0x1000
#define SMB_CONTENT_INDEXED		0x2000
#define	 SMB_ENCRYPTED_FILE		0x4000


#pragma pack (1) 
typedef struct _SMB_HEADER

{
	UCHAR Protocol[4];                // 包含 0xFF,'SMB' 四个字节
	UCHAR Command;                 // SMB命令

	union
	{
		struct
		{
			UCHAR ErrorClass;         // Error class
			UCHAR Reserved;           // Reserved for future use
			USHORT Error;             // Error code
		} DosError;
		ULONG Status;                 // 32-bit error code
	} Status;                               //SMB状态码
	UCHAR Flags;                      // Flags SMB标记
	USHORT Flags2;                    // More flags
	union
	{

		USHORT Pad[6];                // Ensure section is 12 bytes long
		struct
		{
			USHORT PidHigh;           // High part of PID
			UCHAR SecuritySignature[8];	  // reserved for security
		} Extra;
	};                                             //进程ID的高16位
	USHORT Tid;                       // Tree identifier 树id

	USHORT Pid;                       // Caller's process id 进程id
	USHORT Uid;                       // Unauthenticated user id 用户id
	USHORT Mid;                       // multiplex id 多路id

	UCHAR WordCount;//参数的大小
} SMB_HEADER, *PSMB_HEADER;

typedef struct _WRITE_ANDX_REQUEST
{
		UCHAR ANDXCommand;//下一个命令
		UCHAR ANDXReserved;//必须为0
		USHORT ANDXOffset;//下一命令WrodCount偏移
		USHORT Fid;//文件id或句柄
		ULONG Offset;//写入文件的偏移量
		ULONG Reserved;//必须为0
		USHORT WriteMode;//writethrough设置为0
		USHORT CountBytesRemaining;//
		USHORT DataLengthHigh;//DataLength的高16位，否则为0
		USHORT DataLength;
		USHORT DataOffset;//Data的偏移
		ULONG OffsetHigh;
		USHORT ByteCount;//Data的字节数
		//UCHAR Pad[1];
		UCHAR Data[1];
}WRITE_ANDX_REQUEST, *PWRITE_ANDX_REQUEST;

typedef struct _READ_ANDX_REQUEST
{
	UCHAR ANDXCommand;//下一个命令
	UCHAR ANDXReserved;//必须为0
	USHORT ANDXOffset;//下一命令WrodCount偏移
	USHORT Fid;//文件id或句柄
	ULONG Offset;//写入文件的偏移量
	USHORT MaxCountLow;
	USHORT MinCount;
	ULONG MaxCountHigh;
	USHORT Remaining;
	ULONG HighOffset;
	USHORT ByteCount;
}READ_ANDX_REQUEST, *PREAD_ANDX_REQUEST;

typedef struct _READ_ANDX_RESPONSE
{
	UCHAR ANDXCommand;//下一个命令
	UCHAR ANDXReserved;//必须为0
	USHORT ANDXOffset;//下一命令WrodCount偏移
	USHORT Remaining;
	USHORT DataCompactionMode;
	USHORT Reserved;
	USHORT DataLengthLow;
	USHORT DataOffset;
	ULONG DataLengthHigh;
	UCHAR Reserved2[6];
	USHORT ByteCount;
	UCHAR Padding[1];
	UCHAR FileData[1];
}READ_ANDX_RESPONSE, *PREAD_ANDX_RESPONSE;

typedef struct _SMB_COM_READ_RSP {
	UCHAR AndXCommand;
	UCHAR AndXReserved;
	USHORT AndXOffset;
	USHORT Remaining;
	USHORT DataCompactionMode;
	USHORT Reserved;
	USHORT DataLength;
	USHORT DataOffset;
	USHORT DataLengthHigh;
	ULONGLONG Reserved2;
	USHORT ByteCount;
	/* read response data immediately follows */
	//UCHAR Padding;
	UCHAR Data[1];
} SMB_COM_READ_RSP, *PSMB_COM_READ_RSP;

typedef struct _NT_CREATE_ANDX_REQUEST
{
	UCHAR ANDXCommand;//下一个命令
	UCHAR ANDXReserved;//必须为0
	USHORT ANDXOffset;//下一命令WrodCount偏移
	UCHAR Reserved;
	USHORT FileNameLen;
	ULONG CreateFlags;
	ULONG RootFid;
	ULONG AccessMask;
	ULONGLONG AllocationSize;
	ULONG FileAttributes;
	ULONG ShareAccess;
	ULONG Disposition;
	ULONG CreateOptions;
	ULONG Impersonation;
	UCHAR SecurityFlags;
	USHORT ByteCount;
	UCHAR FileName[1];
}NT_CREATE_ANDX_REQUEST, *PNT_CREATE_ANDX_REQUEST;

typedef struct _NT_CREATE_ANDX_RSP {
	//struct smb_hdr hdr;	/* wct = 34 BB */
	UCHAR AndXCommand;
	UCHAR AndXReserved;
	USHORT AndXOffset;
	UCHAR OplockLevel;
	USHORT Fid;
	ULONG CreateAction;
	ULONGLONG CreationTime;
	ULONGLONG LastAccessTime;
	ULONGLONG LastWriteTime;
	ULONGLONG ChangeTime;
	ULONG FileAttributes;
	ULONGLONG AllocationSize;
	ULONGLONG EndOfFile;
	USHORT FileType;
	USHORT DeviceState;
	UCHAR DirectoryFlag;
	USHORT ByteCount;	/* bct = 0 */
}NT_CREATE_ANDX_RSP, *PNT_CREATE_ANDX_RSP;

typedef struct _smb_com_close_req {
	USHORT FileID;
	ULONG LastWriteTime;	/* should be zero or -1 */
	USHORT ByteCount;	/* 0 */
}smb_com_close_req, *psmb_com_close_req;

enum {
	smb_start =0,
	smb_write_data,
	smb_read_data,
	smb_no_head,
};


typedef struct _SMB_SESSION
{
	PFID_NET_FILE NetFile;
	UCHAR KeyBox[2048];
}SMB_SESSION, *PSMB_SESSION;

#pragma pack () 
UINT smb_send_ndis_packet(PAPP_FICTION_PROCESS pProcess);
UINT smb_receive_ndis_packet(PAPP_FICTION_PROCESS pProcess);
VOID smbClearReceiveBuffer(PAPP_FICTION_PROCESS pProcess);

#endif