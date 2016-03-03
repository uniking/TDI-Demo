#ifndef _HTTP_H
#define _HTTP_H
#include <ndis.h>
#include "stdafx.h"
//#include <ntdef.h>

#define HTTP_PARSE_INVALID_METHOD 0x10
#define HTTP_PARSE_INVALID_REQUEST 0x11

#define HTTP_PARSE_INVALID_HEADER 0x13




#define CR 0x0d
#define LF 0x0a

typedef struct _HTTP_STR
{
	PCHAR Start;
	PCHAR End;
	CHAR buffer[256];
}HTTP_STR, *PHTTP_STR;

typedef struct _HTTP_TEMP_BUFFER
{
	UCHAR Buffer[512];
	UINT		length;
}HTTP_TEMP_BUFFER, *PHTTP_TEMP_BUFFER;

typedef struct _MIME_FILE
{
	PUCHAR FileName;//释放
	PUCHAR FileBuffer;//
	PUCHAR FileBufferEnd;//NULL表示没到文件结尾
	ULONG offset;
	BOOLEAN isEnd;//文件结束
}MIME_FILE, *PMIME_FILE;



typedef struct _HTTP_REQUEST
{
	PUCHAR buffer;
	INT length;
	INT headstatus;
	INT step;// 0 处理请求行  1处理请求头  2处理请求头

	//
	HTTP_STR RequestLine;
	HTTP_STR RespondLine;
	HTTP_STR HeaderLine;

	char Boundary[100];
	char boundarybuffer[100];//缓存
	int boundarybufferlen;

	HTTP_STR Body;

	//
	HTTP_STR sMethod;
	int iMethod;
	HTTP_STR Uri;
	UCHAR http_minor;
	UCHAR http_major;

	HTTP_STR Host;
	

	//
	HTTP_STR HeaderName;
	HTTP_STR HeaderValue;

	int MiMeState;
	MIME_FILE File[1024];
	int FileNumber;

	INT FileType;//0 表示不是文件， 1表示stream 2表示form
	BOOLEAN PassFile; //必须放过的文件
	BOOLEAN isStreamAttatchmen;
	UINT FileLength;//单个文件长度
	UCHAR KeyBox[1024];//RC4

	HTTP_TEMP_BUFFER SendTempBuffer;
	HTTP_TEMP_BUFFER ReceiveTempBuffer;

}HTTP_REQUEST, *PHTTP_REQUEST;

UINT HttpAnalysis(PVOID pBuffer, INT length);

UINT http_send_ndis_packet(PAPP_FICTION_PROCESS pProcess);
UINT http_receive_ndis_packet(PAPP_FICTION_PROCESS pProcess);

UINT HttpParseRespondLine(PHTTP_REQUEST pHttp);

#endif