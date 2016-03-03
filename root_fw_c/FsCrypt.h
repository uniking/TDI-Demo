#include <ntddk.h>

#define unk_0x58 0x58
#define unk_0xFFFFFF 0xFFFFFF

#define FS_NAME_LOCK				"LOCK"

#define FS_FILE_DATA_HEADER_SIZE 512
#define FS_CACHE_HEADERDATA_SIZE 512
#define FS_FILE_DATA_MINSIZE 12
#define FS_PWD_SIZE 16


#define unk_0x58 0x58
#define unk_0xFFFFFF 0xFFFFFF
#define unk_0x1FF 0x1FF
#define unk_0x535743 0x535743
#define unk_0xFF231466 0xFF231466
#define unk_0x535746 0x535746
#define unk_0xFF231467 0xFF231467
#define unk_0x88B1F 0x88B1F
#define unk_0xFF231468 0xFF231468
#define unk_0x1EB65F4F 0x1EB65F4F
#define unk_0x65231461 0x65231461
#define unk_0xFF535743 0xFF535743
#define unk_0xFF535746 0xFF535746
#define unk_0xFF088B1F 0xFF088B1F

typedef unsigned char       BYTE;
typedef unsigned long DWORD;
typedef unsigned short      WORD;
typedef int                 BOOL;

typedef BYTE _BYTE; 
typedef DWORD _DWORD;
typedef WORD _WORD;

enum FS_FILE_HEAD_FLAG
{
	Flag6_            = 0x231466,
	Flag7            = 0x231467,
	Flag8            = 0x231468,
	Flag11           = 0x2A4949,
	Flag12_           = 0x2A004D4D,
	Flag1            = 0x65231461,
	Flag_standardEncrypt  = 0x65231462,
	Flag_HaveOtherFlag  = 0x65231463,
	Flag9_Offset4    = 0x65231464,
	Flag_NoCompress11  = 0x65231465,
	Flag_NoCompress12  = 0x65231469,
	Flag5            = 0x65231477
};

typedef struct _FS_ENCRYPT_FILE_HEAD
{
	ULONG Flag;
	WORD EncryptDataOffset;
	WORD EncryptDataLength;
	ULONG field_Data;
	char field_PassWd[16];
	char PassWD[16];
	char field_PassWdProcessed[FS_PWD_SIZE];
	WORD field_9;
	WORD field_A;
	ULONG field_C;
	ULONG field_10;
	WORD field_14;
	CHAR field_18[12];//LOCL标记
	DWORD flag4_0x200;
	CHAR PassWD_0x204[FS_PWD_SIZE];
	DWORD IsDealOk_0x21c;
	ULONG field_218;
	LARGE_INTEGER ByteOffset_0x220;
	DWORD flag3_0x228;

	ULONG field_22C;
}FS_ENCRYPT_FILE_HEAD, *PFS_ENCRYPT_FILE_HEAD;

typedef struct _FS_POLYCE_FILE_HEAD
{
	ULONG _0Flag;
	ULONG field_4;
	ULONG field_8;
	ULONG field_C;
	ULONG field_10;
	ULONG field_14;
	CHAR _18Name[12];
	ULONG _24PolyceNum;
	UCHAR _28Passwd[16]; //密码共16位
	ULONG field_38;
	ULONG field_3C;
	ULONG field_40;
	ULONG field_44;
	ULONG field_48;
	ULONG field_4C;
	ULONG field_50;
	ULONG field_54;
	UCHAR _58HeadData[424]; //大小424 512-424=88 上面是22个dd，正好
}FS_POLYCE_FILE_HEAD, *PFS_POLYCE_FILE_HEAD;

typedef struct _FS_ENCRYPT_FILE_HEAD_TWO
{
	ULONG _0_Magic;
	WORD _4_EncryptDataOffset;
	WORD _6_EncryptDataLength;
	WORD _9_CheckSum;//used to save the byte checksum of next 512 bytes in the content for verification of decryption
	WORD _A_Version;
	CHAR _C_Type[12];	// "SunInfo"
	CHAR _18_SubType[12];
	ULONG _24_Items;//used to save the item numbers saved in the file
	char _28_field_PassWdProcessed[FS_PWD_SIZE];//file header key
	char _38_field_PassWd[FS_PWD_SIZE];
}FS_ENCRYPT_FILE_HEAD_TWO, *PFS_ENCRYPT_FILE_HEAD_TWO;


BOOLEAN
FsDecryptDataByOffset_sub_80A27CF4(
								   IN PUCHAR Data, 
								   IN LARGE_INTEGER Offset,
								   IN ULONG DataSize,
								   IN PUCHAR PolicyPwd, 
								   IN PUCHAR Buffer
								   );

FsEnDecryptDataMore(
					PVOID Data, 
					ULONG DataLength, 
					BYTE *pucKeyIn, 
					BYTE *pucKeyOut
					);

BOOLEAN 
FsFileHeaderVerify(
				   PFS_POLYCE_FILE_HEAD FileHeader, 
				   PCHAR HeaderFlag
				   );

BOOLEAN
FsWriteEncryptHeader(
					 IN PUCHAR Data, 
					 IN ULONG DataLength, 
					 IN PUCHAR Password, 
					 IN PUCHAR PwdBuffer
					 );
BOOLEAN
FsReadDecryptHeader(
					PUCHAR Buffer, 
					PULONG DataLengthPointer, 
					PUCHAR PolicyPwd,
					PUCHAR KeyBox
					);

BOOLEAN
FsWriteBlockEncrypt(
					IN PUCHAR Data, 
					IN ULONG DataLength, 
					IN ULONGLONG Offset,
					IN PUCHAR Password, 
					IN PUCHAR PwdBuffer
					);
BOOLEAN
FsReadBlockDecrypt(
				   IN PUCHAR Data, 
				   IN ULONG DataLength, 
				   IN ULONGLONG Offset,
				   IN PUCHAR Password, 
				   IN PUCHAR PwdBuffer
				   );

BOOLEAN
FsRandomEndecrypt(
				  IN PUCHAR Data, 
				  IN ULONG DataLength, 
				  IN ULONGLONG Offset,
				  IN PUCHAR Password, 
				  IN PUCHAR PwdBuffer
				  );