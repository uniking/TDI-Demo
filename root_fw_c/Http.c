#include "Http.h"
#include "FsCrypt.h"

int HttpParseRequestLine(PHTTP_REQUEST pHttp);
int httpParseHeaderLine(PHTTP_REQUEST pHttp);

char g_boundarybuffer[200] = {0};
int g_boundarybufferlen=0;

int MiMeParseFile(PHTTP_REQUEST pHttp);

BOOLEAN straddchar(char* buffer, char ch)
{
	int i = strlen(buffer);

	if (i >=256)
	{
		return FALSE;
	}

	buffer[i] = ch;
	buffer[++i] = 0;

	return TRUE;
}

char* strnstr(char* str, int strlen,
			  char* subString, int subStringlen)
{
	char* pSrc = str;
	char* pSub = NULL;
	BOOLEAN bFindChar = FALSE;
	int i=0;//

	while(i < strlen)
	{
		if (_strnicmp(&pSrc[i], subString, subStringlen) == 0)
		{
			pSub = pSrc+i;
			break;
		}
		i++;
	}

	return pSub;
}

BOOLEAN HttpHasFileTailByBoundary(PHTTP_REQUEST pHttp)
{
	char end[100] = {0};
	int endlen;
	int bodylen;

	strcat(end, "--");
	strcat(end, pHttp->Boundary);
	strcat(end, "--");
	endlen = strlen(end);
	bodylen = pHttp->Body.End-pHttp->Body.Start;

	if (endlen <= bodylen)
	{
		if (!_strnicmp(end, pHttp->Body.End-endlen, endlen))
		{
			pHttp->boundarybufferlen = 0;
			return TRUE;
		}
		else
		{
			memcpy(pHttp->boundarybuffer, pHttp->Body.End-endlen, endlen);
			pHttp->boundarybufferlen = endlen;
			return FALSE;
		}
	}
	else
	{
		memcpy(pHttp->boundarybuffer+pHttp->boundarybufferlen, pHttp->Body.Start, bodylen);
		pHttp->boundarybufferlen += bodylen;

		if (!_strnicmp(end, pHttp->boundarybuffer, endlen))
		{
			pHttp->boundarybufferlen = 0;
			return TRUE;
		}
		else
		{
			if (pHttp->boundarybufferlen <= endlen)
			{
				;
			}
			else
			{
				RtlMoveMemory(pHttp->boundarybuffer, pHttp->boundarybuffer+(pHttp->boundarybufferlen - endlen), endlen);
				pHttp->boundarybufferlen = endlen;
			}
			
			return FALSE;
		}
	}

	return TRUE;
}


UINT HttpSendAnalysis(PAPP_FICTION_PROCESS pProcess)
{
	PHTTP_REQUEST httpre = (PHTTP_REQUEST)pProcess->SendContext;
	PVOID pBuffer = pProcess->sendBuffer.Buffer;
	INT length = pProcess->sendBuffer.length;

	UINT iRnt = 0;
	ANSI_STRING temp = {0};

	enum {
		sw_parse_respond_line = 0,
		sw_parse_header_line,
		sw_file_type,
		sw_baidu_upload_mime,
		sw_baidu_upload_body,
		sw_baidu_upload_all,
	} state;

	enum{
		sw_unknow_file = 0,
		sw_unknow_file2,
		sw_baidu_upload_file,
		sw_baidu_download_file,

	}filetype;
	

	httpre->buffer = pBuffer;
	httpre->length = length;

	do 
	{
		switch(httpre->step)
		{
		case sw_parse_respond_line://解析http请求行
			if (HttpParseRequestLine(httpre))
			{
				goto done;
			}

			if (httpre->iMethod != 1)
			{
				goto done;
			}

			httpre->step = sw_parse_header_line;

			/*temp.Buffer = httpre.sMethod.Start;
		temp.MaximumLength = temp.Length = httpre.sMethod.End - httpre.sMethod.Start;
		KdPrint(("Method=%Z ", &temp));

		temp.Buffer = httpre.Uri.Start;
		temp.MaximumLength = temp.Length = httpre.Uri.End - httpre.Uri.Start;
		KdPrint(("Uri=%Z ", &temp));

		KdPrint(("Http Version %d.%d\n", httpre.http_major, httpre.http_minor));*/

			break;
		case sw_parse_header_line://解析http请求头
			if (httpParseHeaderLine(httpre))
			{
				goto done;
			}

			if (httpre->headstatus != 0)
			{
				httpre->step = sw_file_type;
				goto udone;
			}

			httpre->step = sw_baidu_upload_mime;
			break;
		case sw_file_type://文件开头是 请求头
			httpre->RequestLine.Start = httpre->RequestLine.End = 0;
			if (httpParseHeaderLine(httpre))
			{
				goto done;
			}

			if (httpre->headstatus != 0)
			{
				goto udone;
			}

			httpre->step = sw_baidu_upload_mime;
			break;

		case sw_baidu_upload_mime://
			if (httpre->FileType == sw_unknow_file2)
			{
				KdPrint(("updata file stream\n"));
				goto done;
			}
			else if (httpre->FileType == sw_baidu_upload_file)
			{
				KdPrint(("updata file form\n"));
				

				if (httpre->Body.End > httpre->Body.Start)
				{//有文件内容
					MiMeParseFile(httpre);

					if (!HttpHasFileTailByBoundary(httpre))
					{
						httpre->step = sw_baidu_upload_body;
						goto udone;
					}
					else
					{
						goto done;
					}
					goto done;
				}
				else
				{//无body内容，下一个包是body
					httpre->step = sw_baidu_upload_body;
					goto udone;
				}
			}
			else
			{
				goto done;
			}

			break;
		case sw_baidu_upload_body://包是body
			{
				if (httpre->FileType == sw_unknow_file2)
				{
					goto done;
				}
				else if (httpre->FileType == sw_baidu_upload_file)
				{
					httpre->Body.Start = httpre->buffer;
					httpre->Body.End = httpre->buffer+httpre->length;

					MiMeParseFile(httpre);

					if (HttpHasFileTailByBoundary(httpre))
					{
						KdPrint(("HttpHasFileTailByBoundary\n"));
						goto done;
					}
					else
					{
						goto udone;
					}
				}
				else
				{
					goto done;
				}
				
			}
			break;
		default:
			goto done;
		}

	} while (TRUE);


done:
httpre->step = sw_parse_respond_line;

udone:

	//处理解析出来的文件
	if (httpre->FileType == sw_baidu_upload_file)
	{
		int i=0;
		int j=0;
		KdPrint(("BodyStart=%x BodyEnd=%x FileNumber=%d MiMeState=%d\n",
			httpre->Body.Start,
			httpre->Body.End,
			httpre->FileNumber,
			httpre->MiMeState));

		j = httpre->FileNumber;
		while(i<j)
		{
			if (httpre->File[i].FileBufferEnd && httpre->File[i].FileBuffer != httpre->File[i].FileBufferEnd)
			{
				KdPrint(("FileName=%s FileBuffer=0x%x offset=%x FileBufferEnd=0x%x startvalue=0x%x, endvalue=0x%x\n",
					httpre->File[i].FileName,
					httpre->File[i].FileBuffer,
					httpre->File[i].offset,
					httpre->File[i].FileBufferEnd,
					*(httpre->File[i].FileBuffer),
					*(httpre->File[i].FileBufferEnd-1)
					));

				if (httpre->File[i].FileBuffer == httpre->buffer)
				{//文件的开始就是包的开始
					if ((httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer) <512 &&
						httpre->MiMeState != 0 &&
						httpre->File[i].offset == 0)
					{
						FLAG_SET_UINT(iRnt, DATA_IS_LESS);
						return iRnt;
					}
				}
				else
				{
					if (httpre->File[i].offset == 0 &&
						(httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer) <512
						)
					{
						FLAG_SET_UINT(iRnt, SPLIT_PACKET);
						pProcess->SplitLength = httpre->File[i].FileBuffer - httpre->buffer;
						return iRnt;
					}
				}

				{//

					if (httpre->File[i].offset == 0)
					{
						if (httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer > 512)
						{
							FsWriteEncryptHeader(
								httpre->File[i].FileBuffer, 
								512, 
								"01234567890123456789", 
								httpre->KeyBox
								);
							if (httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer-512 >0)
							{
								FsRandomEndecrypt(
									httpre->File[i].FileBuffer+512,
									httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer-512,
									512,
									"01234567890123456789",
									httpre->KeyBox
									);
							}
						}
						else
						{
							FsWriteEncryptHeader(
								httpre->File[i].FileBuffer, 
								httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer, 
								"01234567890123456789", 
								httpre->KeyBox
								);
						}
	

					}
					else
					{
						FsRandomEndecrypt(
							httpre->File[i].FileBuffer,
								httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer,
								httpre->File[i].offset,
								"01234567890123456789",
								httpre->KeyBox
							);

					}
					FLAG_SET_UINT(iRnt, CHANGE_PACKET_DATA);


				}

				httpre->File[i].offset+=httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer;
			}
			
			if (httpre->MiMeState == 0)
			{
				ExFreePoolWithTag(httpre->File[i].FileName, 'xxxx');
				httpre->File[i].FileName=0;
				httpre->File[i].FileBuffer = NULL;
				httpre->File[i].FileBufferEnd = NULL;
				httpre->FileNumber--;
			}

			i++;
		}

	}

	return iRnt;
}

UINT HttpReceiveAnalysis(PAPP_FICTION_PROCESS pProcess)
{//PAPP_FICTION_PROCESS pProcess
	//(PHTTP_REQUEST)pProcess->ReceiveContext, pProcess->receiveBuffer.buffer, pProcess->receiveBuffer.length
	PHTTP_REQUEST httpre = (PHTTP_REQUEST)pProcess->ReceiveContext;
	PVOID pBuffer = pProcess->receiveBuffer.Buffer;
	INT length = pProcess->receiveBuffer.length;
	UINT rtnStatus = 0;
	ANSI_STRING temp = {0};

	//KdPrint(("HttpReceiveAnalysis datalength=%d\n", length));

	enum {
		sw_parse_respond_line = 0,
		sw_parse_header_line,
		sw_file_type,
		sw_baidu_upload_mime,
		sw_baidu_download_body,
		sw_baidu_download_all,
	} state;

	enum{
		sw_unknow_file = 0,
		sw_115_download_file,
		sw_baidu_upload_file,
		sw_baidu_download_file,
		sw_huawei_download_file,

	}filetype;
	

	httpre->buffer = pBuffer;
	httpre->length = length;

	httpre->headstatus = 0;
	do 
	{
		switch(httpre->step)
		{
		case sw_parse_respond_line://解析http请求行
			if (HttpParseRespondLine(httpre))
			{
				goto done;
			}
			httpre->step = sw_parse_header_line;

			/*temp.Buffer = httpre.sMethod.Start;
		temp.MaximumLength = temp.Length = httpre.sMethod.End - httpre.sMethod.Start;
		KdPrint(("Method=%Z ", &temp));

		temp.Buffer = httpre.Uri.Start;
		temp.MaximumLength = temp.Length = httpre.Uri.End - httpre.Uri.Start;
		KdPrint(("Uri=%Z ", &temp));

		KdPrint(("Http Version %d.%d\n", httpre.http_major, httpre.http_minor));*/

			break;
		case sw_parse_header_line://解析http请求头
			if (httpParseHeaderLine(httpre))
			{
				goto done;
			}
			httpre->step = sw_file_type;
			break;

		case sw_file_type://
			if (httpre->FileType == sw_115_download_file)
			{
				if (httpre->isStreamAttatchmen)
				{
					KdPrint(("sw_115_download_file\n"));
					goto baidu_download;
				}
				else
				{
					goto done;
				}
			}
			else if (httpre->FileType == sw_huawei_download_file)
			{
				if (httpre->isStreamAttatchmen)
				{
					KdPrint(("sw_115_download_file\n"));
					goto baidu_download;
				}
				else
				{
					goto done;
				}
			}
			else if (httpre->FileType == sw_baidu_upload_file)
			{
				if (httpre->Body.End > httpre->Body.Start)
				{//有文件内容
					MiMeParseFile(httpre);
					goto done;
				}
				else
				{//无body内容，下一个包是body
					httpre->step = sw_baidu_upload_mime;
					goto udone;
				}
			}
			else if(httpre->FileType == sw_baidu_download_file)
			{
				httpre->step = sw_baidu_download_body;
			}
			else
			{
				goto done;
			}

			break;
		case sw_baidu_upload_mime://包是body
			{
				if (httpre->FileType == 1)
				{
					goto done;
				}
				else if (httpre->FileType == 2)
				{
					httpre->Body.Start = httpre->buffer;
					httpre->Body.End = httpre->buffer+httpre->length;

					MiMeParseFile(httpre);

					if (HttpHasFileTailByBoundary(httpre))
					{
						goto done;
					}
					else
					{
						goto udone;
					}
				}
				else
				{
					goto done;
				}
				
			}
			break;
		case sw_baidu_download_body://body就是文件的内容
			{
baidu_download:
				KdPrint(("\n--file--\n"));
				if (httpre->Body.End-httpre->Body.Start >= httpre->FileLength) //如果收集的长度等于文件长度
				{
					KdPrint(("head=%x tail=%x \n", *httpre->Body.Start, httpre->Body.Start[httpre->FileLength-1]));
					httpre->File[0].FileBuffer = httpre->Body.Start;
					httpre->File[0].FileBufferEnd = &(httpre->Body.Start[httpre->FileLength]);
					goto done;
				}
				else //继续
				{
					KdPrint(("head=%x \n", *httpre->Body.Start));
					

					if (httpre->File[0].offset == 0 &&
						(httpre->Body.End - httpre->Body.Start <512)
						)
					{//只有文件头才进行缓存
						KdPrint(("file head size < 512\n"));
						//FLAG_SET_UINT(rtnStatus, SPLIT_PACKET);
						//pProcess->SplitLength = httpre->Body.Start - httpre->buffer;
						FLAG_SET_UINT(rtnStatus, CHANGE_PACKET_DATA);
						FLAG_SET_UINT(rtnStatus, SPLIT_PACKET);
						httpre->step = 0;
						//pProcess->receiveBuffer.length = httpre->Body.Start - httpre->buffer;
						
						httpre->ReceiveTempBuffer.length = httpre->Body.End - httpre->Body.Start;
						RtlCopyMemory(httpre->ReceiveTempBuffer.Buffer, httpre->Body.Start, httpre->ReceiveTempBuffer.length);
						return rtnStatus;
					}
					else
					{
						httpre->step = sw_baidu_download_all;
					}
					//httpre->FileLength -= (httpre->Body.End-httpre->Body.Start);
					httpre->File[0].FileBuffer = httpre->Body.Start;
					httpre->File[0].FileBufferEnd = httpre->Body.End;

					goto udone;
				}
			}
			break;
		case sw_baidu_download_all:
			{
				httpre->Body.Start = httpre->buffer;
				httpre->Body.End = httpre->buffer+httpre->length;

				if (httpre->Body.End-httpre->Body.Start >= httpre->FileLength) //如果收集的长度等于文件长度
				{
					KdPrint(("tail=%x \n", httpre->Body.Start[httpre->FileLength-1]));
					httpre->File[0].FileBuffer = httpre->Body.Start;
					httpre->File[0].FileBufferEnd = &(httpre->Body.Start[httpre->FileLength]);
					goto done;
				}
				else //继续
				{
					//KdPrint(("head=%c tail=%c \n", *httpre->Body.Start, *(httpre->Body.End-1)));
					//httpre->FileLength -= (httpre->Body.End-httpre->Body.Start);
					httpre->File[0].FileBuffer = httpre->Body.Start;
					httpre->File[0].FileBufferEnd = httpre->Body.End;
					goto udone;
				}
			}
			break;
		default:
			goto done;
		}

	} while (TRUE);


done:
httpre->step = 0;

udone:

	//处理解析出来的文件
	if (httpre->FileType == sw_baidu_upload_file)
	{
		int i=0;
		int j=0;
		KdPrint(("BodyStart=%x BodyEnd=%x FileNumber=%d MiMeState=%d\n",
			httpre->Body.Start,
			httpre->Body.End,
			httpre->FileNumber,
			httpre->MiMeState));

		j = httpre->FileNumber;
		while(i<j)
		{
			KdPrint(("FileName=%s FileBuffer=%x FileBufferEnd=%x\n",
				httpre->File[i].FileName,
				httpre->File[i].FileBuffer,
				httpre->File[i].FileBufferEnd));

			//修改文件内容
			if (/*httpre->File[i].FileBuffer < httpre->Body.End &&*/
				httpre->File[i].FileBuffer < httpre->File[i].FileBufferEnd)
			{
				//if (*httpre->File[i].FileBuffer == '0')
				//{
				//	//*httpre->File[i].FileBuffer = 'w';
				//	//*(httpre->File[i].FileBuffer+1) = 'a';

				//	RtlCopyMemory(httpre->File[i].FileBuffer, "abcdefjabcdefjabcdefj", strlen("abcdefjabcdefjabcdefj"));
				//	iRnt = 1;
				//}

				KdPrint(("%c to w\n", *httpre->File[i].FileBuffer));
				*httpre->File[i].FileBuffer = 'w';
			}



			if (httpre->MiMeState == 0)
			{
				ExFreePoolWithTag(httpre->File[i].FileName, 0);
				httpre->File[i].FileName=0;
				httpre->FileNumber--;
			}

			i++;
		}

	}

	if (httpre->FileType == sw_baidu_download_file)
	{
		int i=0;
		int j=0;

		//if (length < 5 && httpre->step != 0)
		//{
		//	FLAG_SET_UINT(rtnStatus, DATA_IS_LESS);
		//	return rtnStatus;
		//}
		
		if (httpre->step != 0)
		{
			httpre->FileLength -= (httpre->Body.End-httpre->Body.Start);
		}
		
		KdPrint(("filetype=%d BodyStart=%x BodyEnd=%x FileNumber=%d\n",
			httpre->FileType,
			httpre->Body.Start,
			httpre->Body.End,
			httpre->FileNumber));

		KdPrint(("FileName=%s Length=%d offset=%x FileBuffer=%x ch=%x FileBufferEnd=%x\n",
			httpre->File[i].FileName,
			httpre->FileLength,
			httpre->File[i].offset,
			httpre->File[i].FileBuffer,
			httpre->File[i].FileBuffer[0],
			httpre->File[i].FileBufferEnd));
		
		
		{//解密下载的文件
			//int index =0;
			//for (;index < httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer;index++)
			//{
			//	httpre->File[i].FileBuffer[index] ^= 0x6523;

			//	if (httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer == 1)
			//	{
			//		KdPrint(("x111111  after xor=%x\n", httpre->File[i].FileBuffer[0]));
			//		FLAG_SET_UINT(rtnStatus, DEBUG_TEST);
			//	}
			//}
			ULONG filelength = httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer;
			if (httpre->File[i].offset==0)
			{
				if (filelength > 512)
				{
					filelength = 512;
					if (FsReadDecryptHeader(
						httpre->File[i].FileBuffer, 
						&filelength,
						"01234567890123456789",
						httpre->KeyBox))
					{
						FsRandomEndecrypt(
							httpre->File[i].FileBuffer+512, 
							httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer - 512, 
							512,
							"01234567890123456789", 
							httpre->KeyBox
							);
					}
					else
					{//解密头失败
						httpre->step = 0;
						goto label_endcrypt_baidu;
					}
				}
				else
				{
					if (!FsReadDecryptHeader(
						httpre->File[i].FileBuffer, 
						&filelength,
						"01234567890123456789",
						httpre->KeyBox))
					{
						httpre->step = 0;
						goto label_endcrypt_baidu;
					}
				}
				
			}
			else
			{
				//FsEnDecryptDataMore(
				//	httpre->File[i].FileBuffer,
				//	httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer,
				//	"01234567890123456789",
				//	httpre->KeyBox
				//	);
					FsRandomEndecrypt(
					httpre->File[i].FileBuffer, 
					httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer, 
					httpre->File[i].offset,
					"01234567890123456789", 
					httpre->KeyBox
					);
			}
			

			FLAG_SET_UINT(rtnStatus, CHANGE_PACKET_DATA);

		}

		httpre->File[i].offset+=httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer;
label_endcrypt_baidu:
		if (httpre->step == 0 && httpre->FileNumber>0) 
		{
			if (httpre->File[i].FileName)
			{
				ExFreePoolWithTag(httpre->File[i].FileName, 'xxxx');
				httpre->File[i].FileName=NULL;
			}
			httpre->File[i].FileBuffer = NULL;
			httpre->File[i].FileBufferEnd = NULL;
			httpre->FileType = 0;
			httpre->FileNumber--;
		}
		
	}


	if ( (httpre->FileType == sw_115_download_file  || httpre->FileType == sw_huawei_download_file)
		&& httpre->isStreamAttatchmen)
	{
		int i=0;
		int j=0;

		//if (length < 5 && httpre->step != 0)
		//{
		//	FLAG_SET_UINT(rtnStatus, DATA_IS_LESS);
		//	return rtnStatus;
		//}

		if (httpre->step != 0)
		{
			httpre->FileLength -= (httpre->Body.End-httpre->Body.Start);
		}

		KdPrint(("filetype=%d BodyStart=%x BodyEnd=%x FileNumber=%d\n",
			httpre->FileType,
			httpre->Body.Start,
			httpre->Body.End,
			httpre->FileNumber));

		KdPrint(("Length=%d offset=%x FileBuffer=%x ch=%x FileBufferEnd=%x\n",
			httpre->FileLength,
			httpre->File[i].offset,
			httpre->File[i].FileBuffer,
			httpre->File[i].FileBuffer[0],
			httpre->File[i].FileBufferEnd));


		{//解密下载的文件
			//int index =0;
			//for (;index < httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer;index++)
			//{
			//	httpre->File[i].FileBuffer[index] ^= 0x6523;

			//	if (httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer == 1)
			//	{
			//		KdPrint(("x111111  after xor=%x\n", httpre->File[i].FileBuffer[0]));
			//		FLAG_SET_UINT(rtnStatus, DEBUG_TEST);
			//	}
			//}
			ULONG filelength = httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer;
			if (httpre->File[i].offset==0)
			{
				if (filelength > 512)
				{
					filelength = 512;
					if (FsReadDecryptHeader(
						httpre->File[i].FileBuffer, 
						&filelength,
						"01234567890123456789",
						httpre->KeyBox))
					{
						FsRandomEndecrypt(
							httpre->File[i].FileBuffer+512, 
							httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer - 512, 
							512,
							"01234567890123456789", 
							httpre->KeyBox
							);
					}
					else
					{//解密头失败
						httpre->step = 0;
						goto label_endcrypt_115;
					}
				}
				else
				{
					if (!FsReadDecryptHeader(
						httpre->File[i].FileBuffer, 
						&filelength,
						"01234567890123456789",
						httpre->KeyBox))
					{//解密头失败
						httpre->step = 0;
						goto label_endcrypt_115;
					}
				}

			}
			else
			{
				//FsEnDecryptDataMore(
				//	httpre->File[i].FileBuffer,
				//	httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer,
				//	"01234567890123456789",
				//	httpre->KeyBox
				//	);
				FsRandomEndecrypt(
				httpre->File[i].FileBuffer, 
				httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer, 
				httpre->File[i].offset,
				"01234567890123456789", 
				httpre->KeyBox
				);
			}


			FLAG_SET_UINT(rtnStatus, CHANGE_PACKET_DATA);

		}

		httpre->File[i].offset+=httpre->File[i].FileBufferEnd-httpre->File[i].FileBuffer;

label_endcrypt_115:
		if (httpre->step == 0 && httpre->FileNumber>0) 
		{
			//NdisFreeMemory(httpre->File[i].FileName, 512, 0);
			//httpre->File[i].FileName=NULL;
			httpre->File[i].FileBuffer = NULL;
			httpre->File[i].FileBufferEnd = NULL;
			httpre->FileType = 0;
			httpre->FileNumber--;
		}

	}

	return rtnStatus;
}

int HttpParseRequestLine(PHTTP_REQUEST pHttp)
{
	PCHAR pRequestLine;
	UCHAR ch;
	UCHAR m;
	UCHAR c;

	enum {
		sw_start = 0,
		sw_method,
		sw_spaces_before_uri,
		sw_schema,
		sw_schema_slash,
		sw_schema_slash_slash,
		sw_host_start,
		sw_host,
		sw_host_end,
		sw_host_ip_literal,
		sw_port,
		sw_host_http_09,
		sw_after_slash_in_uri,
		sw_check_uri,
		sw_check_uri_http_09,
		sw_uri,
		sw_http_09,
		sw_http_H,
		sw_http_HT,
		sw_http_HTT,
		sw_http_HTTP,
		sw_first_major_digit,
		sw_major_digit,
		sw_first_minor_digit,
		sw_minor_digit,
		sw_spaces_after_digit,
		sw_almost_done
	} state;

	pHttp->FileType = FALSE;

	pRequestLine = pHttp->buffer;
	state = pHttp->headstatus;
	while(pRequestLine <= pHttp->buffer+pHttp->length)
	{
		ch = *pRequestLine;

		switch (state)
		{
		case sw_start:
			{
				pHttp->RequestLine.Start = pRequestLine;
				if (ch == 0x0a || ch == 0x0d)
				{
					break;
				}

				if ((ch < 'A' || ch > 'Z') && ch != '_') 
				{
					return HTTP_PARSE_INVALID_METHOD;
				}

				state = sw_method;

				break;
			}
		case sw_method:
			{
				if (ch == ' ')
				{
					pHttp->sMethod.End = pRequestLine;
					pHttp->sMethod.Start = pHttp->RequestLine.Start;
					switch(pHttp->sMethod.End - pHttp->sMethod.Start)
					{//提取iMethod
					case 3:
						break;
					case 4:
						break;
					case 5:
						break;
					case 6:
						break;
					case 7:
						break;
					case 8:
						break;
					case 9:
						break;
					}

					if (!_strnicmp(pHttp->sMethod.Start, "POST", 4))
					{
						pHttp->iMethod = 1;
					}
					state = sw_spaces_before_uri;
					break;
				}

				if ((ch < 'A' || ch > 'Z') && ch != '_') 
				{
					return HTTP_PARSE_INVALID_METHOD;
				}
				break;
			}
		case sw_spaces_before_uri:
			{
				if (ch == '/') 
				{
					pHttp->Uri.Start = pRequestLine;
					state = sw_after_slash_in_uri;
					break;
				}

				c =  (ch | 0x20);
				if (c >= 'a' && c <= 'z') 
				{//uri可能带有host
					state = sw_schema;
					break;
				}

				switch (ch) 
				{
					case ' ':
						break;
					default:
						return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_schema:
		case sw_after_slash_in_uri:
			{//暂时不解析uri
				if (ch == ' ')
				{
					pHttp->Uri.End = pRequestLine;
					pRequestLine++;
					state = sw_http_H;
					break;
				}

				break;
			}
		case sw_http_H:
			{
				switch (ch) 
				{
					case 'T':
						state = sw_http_HT;
						break;
					default:
						return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_http_HT:
			{
				switch (ch) 
				{
				case 'T':
					state = sw_http_HTT;
					break;
				default:
					return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_http_HTT:
			{
				switch (ch) 
				{
				case 'P':
					state = sw_http_HTTP;
					break;
				default:
					return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_http_HTTP:
			{
				switch (ch) 
				{
				case '/':
					state = sw_first_major_digit;
					break;
				default:
					return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_first_major_digit:
			{
				if (ch < '1' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

				pHttp->http_major = ch - '0';
				state = sw_major_digit;
				break;
			}
		case sw_major_digit:
			{
				if (ch == '.') 
				{
					state = sw_first_minor_digit;
					break;
				}

				if (ch < '0' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

			 	pHttp->http_major = pHttp->http_major * 10 + ch - '0';
				break;
			}
		case sw_first_minor_digit:
			{
				if (ch < '0' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

				pHttp->http_minor = ch - '0';
				state = sw_minor_digit;
				break;
			}
		case sw_minor_digit:
			{
				if (ch == 0x0d) 
				{
					state = sw_almost_done;
					break;
				}

				if (ch == 0x0a) 
				{
					goto done;
				}

				if (ch == ' ') 
				{
					state = sw_spaces_after_digit;
					break;
				}

				if (ch < '0' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

				pHttp->http_minor = pHttp->http_minor * 10 + ch - '0';
				break;
			}
		case sw_spaces_after_digit:
			{
				switch (ch) 
				{
					case ' ':
						break;
					case 0x0d:
						state = sw_almost_done;
						break;
					case 0x0a:
						goto done;
					default:
						return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}


		case sw_almost_done:
			{
				pHttp->RequestLine.End = pRequestLine-1;
				switch (ch) 
				{
					case 0x0a:
						goto done;
					default:
						return HTTP_PARSE_INVALID_REQUEST;
				}
			}

		default:
			KdPrint(("unknow\n"));
		}

		pHttp->headstatus = state;
		pRequestLine++;
	}

	return 3;
done:
	pHttp->headstatus = sw_start;
	return 0;
}

UINT HttpParseRespondLine(PHTTP_REQUEST pHttp)
{
	PCHAR pRespondLine;
	UCHAR ch;
	UCHAR m;
	UCHAR c;

	enum {
		sw_start = 0,
		sw_http_H,
		sw_http_HT,
		sw_http_HTT,
		sw_http_HTTP,
		sw_first_major_digit,
		sw_major_digit,
		sw_first_minor_digit,
		sw_minor_digit,
		sw_spaces_after_digit,
		sw_status_code,
		sw_spaces_after_status,
		sw_reason_phrase,
		sw_almost_done
	} state;

	//初始化一些变量
	pHttp->FileType = 0;
	pHttp->PassFile = FALSE;

	pRespondLine = pHttp->buffer;
	state = pHttp->headstatus;
	while(pRespondLine <= pHttp->buffer+pHttp->length)
	{
		ch = *pRespondLine;

		switch (state)
		{
		case sw_start:
			{
				pHttp->RequestLine.Start = pRespondLine;
				if (ch == 0x0a || ch == 0x0d)
				{
					break;
				}

				if ((ch < 'A' || ch > 'Z') && ch != '_') 
				{
					return HTTP_PARSE_INVALID_METHOD;
				}

				state = sw_http_H;

				break;
			}
		case sw_http_H:
			{
				switch (ch) 
				{
				case 'T':
					state = sw_http_HT;
					break;
				default:
					return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_http_HT:
			{
				switch (ch) 
				{
				case 'T':
					state = sw_http_HTT;
					break;
				default:
					return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_http_HTT:
			{
				switch (ch) 
				{
				case 'P':
					state = sw_http_HTTP;
					break;
				default:
					return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_http_HTTP:
			{
				switch (ch) 
				{
				case '/':
					state = sw_first_major_digit;
					break;
				default:
					return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_first_major_digit:
			{
				if (ch < '1' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

				pHttp->http_major = ch - '0';
				state = sw_major_digit;
				break;
			}
		case sw_major_digit:
			{
				if (ch == '.') 
				{
					state = sw_first_minor_digit;
					break;
				}

				if (ch < '0' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

				pHttp->http_major = pHttp->http_major * 10 + ch - '0';
				break;
			}
		case sw_first_minor_digit:
			{
				if (ch < '0' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

				pHttp->http_minor = ch - '0';
				state = sw_minor_digit;
				break;
			}
		case sw_minor_digit:
			{
				if (ch == 0x0d) 
				{
					state = sw_almost_done;
					break;
				}

				if (ch == 0x0a) 
				{
					goto done;
				}

				if (ch == ' ') 
				{
					state = sw_spaces_after_digit;
					break;
				}

				if (ch < '0' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

				pHttp->http_minor = pHttp->http_minor * 10 + ch - '0';
				break;
			}
		case sw_spaces_after_digit:
			{
				if (ch == ' ')
				{
					break;
				}

				if (ch <'0' &&ch >9)
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

				state = sw_status_code;
				
				break;
			}
		case sw_status_code:
			if (ch == ' ')
			{
				state = sw_spaces_after_status;
				break;
			}
			break;
		case sw_spaces_after_status:
			if (ch == ' ')
			{
				break;
			}

			state = sw_reason_phrase;
			break;
		case sw_reason_phrase:
			if (ch == 0x0a ||
				ch == 0x0d)
			{
				state = sw_almost_done;
			}
			break;

		case sw_almost_done:
			{
				pHttp->RequestLine.End = pRespondLine-1;
				switch (ch) 
				{
				case 0x0a:
					goto done;
				default:
					return HTTP_PARSE_INVALID_REQUEST;
				}
			}

		default:
			KdPrint(("unknow\n"));
		}

		pHttp->headstatus = state;
		pRespondLine++;
	}

	return 3;
done:
	pHttp->headstatus = sw_start;
	return 0;
}

int httpMatchFieldName(PUCHAR name, INT length)
{
	int nRtn = 0;

	if (!_strnicmp(name, "Mail-Upload-name", length))
	{
		nRtn = 1;
	}
	else if(!_strnicmp(name, "Content-Type", length))
	{
		nRtn = 2;
	}
	else if (!_strnicmp(name, "Content-Disposition", length))
	{
		nRtn = 3;
	}
	else if (!_strnicmp(name, "Content-Length", length))
	{
		nRtn = 4;
	}
	else if (!_strnicmp(name, "Server", length))
	{
	}
	else if (!_strnicmp(name, "Connection", length))
	{
	}
	else if (!_strnicmp(name, "Host", length))
	{
	}
	else
	{
		nRtn = 0;
	}

	return nRtn;
}

int httpMatchContentType(PUCHAR name, INT length)
{
	int nRtn = 0;

	if (!_strnicmp(name, "application/octet-stream", strlen("application/octet-stream")))
	{
		nRtn = 1;
	}
	else if(!_strnicmp(name, "multipart/form-data", strlen("multipart/form-data")))
	{
		nRtn = 2;
	}
	else if(!_strnicmp(name, "text/plain", strlen("text/plain")))
	{
		nRtn = 3;
	}
	else if (!_strnicmp(name, "application/json", strlen("application/json")))
	{
		nRtn = 4; //放过的文件类型
	}
	//else if (!_strnicmp(name, "text/x-c", strlen("text/x-c")))
	//{
	//	nRtn = 4;
	//}
	else
	{
		nRtn = 0;
	}

	return nRtn;
}

BOOLEAN httpGetProperty(PHTTP_REQUEST pHttp)
{
	int n = 0;
	int m = 0;
	int i=0;
	int BoundaryIndex = 0;
	PUCHAR pValue;
	INT Valuelen = 0;
	UCHAR ch;
	int filenamelength = 0;

	enum {
		sw_name = 0,
		sw_first_quotation,
		sw_value,
		sw_almost_done,
	} state;
	enum{
		sw_Mail_Upload_name = 1,
		sw_Content_Type,
		sw_Content_Disposition,
		sw_Content_Length,
	}fieldname;

	enum{
		sw_unknow_file = 0,
		sw_115_download_file,
		sw_baidu_upload_file,
		sw_baidu_download_file,

	}filetype;

	n = httpMatchFieldName(pHttp->HeaderName.buffer, strlen(pHttp->HeaderName.buffer));
	switch(n)
	{
	case sw_Mail_Upload_name:
		break;
	case sw_Content_Type: //Content-Type
		Valuelen = strlen(pHttp->HeaderValue.buffer);
		m = httpMatchContentType(pHttp->HeaderValue.buffer, Valuelen);
		switch(m)
		{
		case 1://application/octet-stream
			pHttp->FileType = 1;
			break;
		case 2://multipart/form-data
			//提取boundary
			BoundaryIndex = 0;
			pValue = pHttp->HeaderValue.buffer;
			i = 0;
			state = sw_name;
			while(i < Valuelen)
			{
				
				ch = *pValue;
				switch(state)
				{
				case sw_name:
					if (ch == '=')
					{
						state = sw_value;
					}
					else if (ch == 0x0d || ch == 0x0a || ch == 0)
					{
						state = sw_value;
					}
					break;
				case sw_value:
					if (ch == 0x0d ||
						ch == 0x0a)
					{
						state = sw_almost_done;
						break;
					}
					else
					{
						pHttp->Boundary[BoundaryIndex]=ch;
						BoundaryIndex++;
					}
					break;
				case sw_almost_done:
					goto boundaryend;

				}
				pValue++;
				i++;
			}
boundaryend:
			pHttp->FileType = 2;
			break;
			
		case 3://华为网盘下载
			pHttp->FileType = 4;
			break;

		case 4://要排除的类型
			pHttp->PassFile = TRUE;
			pHttp->FileType = 0; //防止在Content_Disposition之后
			break;

		default:
			;
		}
		break;
	case sw_Content_Disposition:
		//提取文件名字，body就是文件
		//Content-Disposition: attachment; filename="..................mapping_test....zip"
		{
			if (_strnicmp(pHttp->HeaderValue.buffer, "attachment", 10) == 0)
			{
				pHttp->isStreamAttatchmen = TRUE;
			}
			else
			{
				pHttp->isStreamAttatchmen = FALSE; //不是attatchmen 直接退 inline属于浏览器正常文件
				pHttp->FileType = 0;
				break;
			}

			Valuelen = strlen(pHttp->HeaderValue.buffer);
			pValue = strnstr(pHttp->HeaderValue.buffer, Valuelen, "filename=\"", 10);
			if (pValue == NULL)
			{
				if (pHttp->isStreamAttatchmen == FALSE ||
					pHttp->PassFile)
				{
					pHttp->FileType = 0;
				}
				else
				{

					pHttp->FileType = 4;//华为、不依赖内容类型 sw_Content_Type
				}
				
				break;
			}

			state = sw_name;
			while(pValue < (pHttp->HeaderValue.buffer+Valuelen))
			{
				ch = *pValue;
				switch(state)
				{
				case sw_name:
					if (ch == '=')
					{
						state = sw_first_quotation;
					}
					break;
				case sw_first_quotation:
					if (ch == '\"')
					{
						filenamelength=0;
						pHttp->File[pHttp->FileNumber].FileName = ExAllocatePoolWithTag(NonPagedPool, 512, 0);
						RtlZeroMemory(pHttp->File[pHttp->FileNumber].FileName, 512);
						pHttp->File[pHttp->FileNumber].offset = 0;
						pHttp->FileNumber++;
						state = sw_value;
					}
					break;
				case sw_value:
					if (ch == '\"')
					{
						pHttp->File[pHttp->FileNumber-1].FileName[filenamelength] = 0;
						goto label_3_nameend;
					}

					if (filenamelength==512)
					{
						filenamelength=0;
					}

					pHttp->File[pHttp->FileNumber-1].FileName[filenamelength] = ch;
					filenamelength++;
					break;
				default:
					;
				}
				pValue++;
			}

			


label_3_nameend:
			pHttp->FileType = 3;//响应头中含有文件的名字
			}
		break;

	case sw_Content_Length://Content-Length
		{
			int i;
			pValue = pHttp->HeaderValue.Start;
			pHttp->FileLength = 0;
			while(TRUE)
			{
				ch = *pValue;
				if (ch < '0' || ch > '9')
				{
					break;
				}

				pHttp->FileLength = pHttp->FileLength*10 + (ch-'0');
				pValue++;
			}

		}
		break;
	case 5:
		break;
	default:
		;
	}


	return 0;
}

int httpParseHeaderLine(PHTTP_REQUEST pHttp)
{
	UCHAR ch;
	PUCHAR pRequestHeader;
	ANSI_STRING msg;

	enum {
		sw_start = 0,
		sw_name,
		sw_space_before_value,
		sw_value,
		sw_space_after_value,
		sw_ignore_line,
		sw_almost_done,
		sw_header_almost_done
	} state;

	if (pHttp->RequestLine.End)
	{
		pRequestHeader = pHttp->RequestLine.End+2;
	}
	else
	{
		pRequestHeader = pHttp->buffer;
	}
	
	state = pHttp->headstatus;

	while(pRequestHeader < (pHttp->buffer+pHttp->length))
	{
		ch = *pRequestHeader;
		switch(state)
		{
		case sw_start:
			{
				if (ch == 0x0d)
				{
					pHttp->HeaderLine.End = pRequestHeader;
					state = sw_header_almost_done;
					break;
				}
				if (ch == 0x0a)
				{
					pHttp->HeaderLine.End = pRequestHeader-1;
					goto done;
				}
				if (ch == '/')
				{
					state = sw_ignore_line;
					break;
				}

				pHttp->HeaderLine.Start = pRequestHeader;
				pHttp->HeaderName.Start = pRequestHeader;
				pHttp->HeaderName.buffer[0] = 0;
				straddchar(pHttp->HeaderName.buffer, ch);
				
				state = sw_name;
				break;
			}
		case sw_name:
			{
				if (ch == ':')
				{
					pHttp->HeaderName.End = pRequestHeader;
					state = sw_space_before_value;
					break;
				}
				if (ch == 0x0d)
				{
					state = sw_almost_done;
					break;
				}
				if (ch == 0x0a)
				{
					goto done;
				}

				straddchar(pHttp->HeaderName.buffer, ch);
				break;
			}
		case sw_space_before_value:
			{
				switch (ch)
				{
					case ' ':
						break;
					case CR:
						state = sw_almost_done;
						break;
					case LF:
						goto done;
					case '\0':
						return HTTP_PARSE_INVALID_HEADER;
					default:
						pHttp->HeaderValue.Start = pRequestHeader;
						pHttp->HeaderValue.buffer[0] = 0;

						straddchar(pHttp->HeaderValue.buffer, ch);
						state = sw_value;
						break;
				}

				break;
			}
		case sw_value:
			{
				switch(ch)
				{
				case 0x0d:
					pHttp->HeaderValue.End = pRequestHeader;
					state = sw_almost_done;
					break;
				case 0x0a:
					goto done;
					break;
				case ' ':
					straddchar(pHttp->HeaderValue.buffer, ch);
					state = sw_space_after_value;
					break;
				default:
					straddchar(pHttp->HeaderValue.buffer, ch);
				}
				break;
			}
		case sw_space_after_value:
			{
				switch (ch) 
				{
					case ' ':
						straddchar(pHttp->HeaderValue.buffer, ch);
						break;
					case 0x0d:
						state = sw_almost_done;
						break;
					case 0x0a:
						goto done;
					case '\0':
						return HTTP_PARSE_INVALID_HEADER;
					default:
						straddchar(pHttp->HeaderValue.buffer, ch);
						state = sw_value;
						break;
				}

				break;
			}
		case sw_ignore_line:
			{
				switch (ch) 
				{
				case 0x0a:
					state = sw_start;
					break;
				default:
					break;
				}
				break;
			}
		case sw_almost_done://解析完一个请求行
			{
				switch (ch) 
				{
					case 0x0a:
						//goto done;
					case 0x0d:
						{
							pHttp->HeaderLine.End = pRequestHeader;

							//打印下完成的name和value
							/*msg.Buffer = pHttp->HeaderName.Start;
							msg.MaximumLength = msg.Length = pHttp->HeaderName.End - pHttp->HeaderName.Start;
							KdPrint(("name:%Z ", &msg));

							msg.Buffer = pHttp->HeaderValue.Start;
							msg.MaximumLength = msg.Length = pHttp->HeaderValue.End - pHttp->HeaderValue.Start;
							KdPrint(("value:%Z\n", &msg));*/


							httpGetProperty(pHttp);

							//开始下一行
							state = sw_start;
						}
						
						break;
					default:
						return HTTP_PARSE_INVALID_HEADER;
				}
				
				break;
			}
		case sw_header_almost_done://整个请求头解析完成
			{
				switch (ch) 
				{
					case 0x0a:
						pRequestHeader++;
						state = sw_start;
						goto done;
					default:
						return HTTP_PARSE_INVALID_HEADER;
				}
				break;
			}
		}

		pHttp->headstatus = state;
		pRequestHeader++;
	}

done:

	pHttp->headstatus = state;
	pHttp->Body.Start = pRequestHeader;
	pHttp->Body.End = pHttp->buffer+pHttp->length;
	return 0;
}



int MiMeParseFile(PHTTP_REQUEST pHttp)
{
	CHAR ch;
	int filenamelength = 0;
	PCHAR pBody;
	BOOLEAN isContent = FALSE;
	enum {
		sw_start = 0,
		sw_filename_fi,
		sw_filename_fil,
		sw_filename_file,
		sw_filename_filen,
		sw_filename_filena,
		sw_filename_filenam,
		sw_filename_filename,
		sw_equal_after_filename,
		sw_quotation_after_filename,
		sw_filename,
		sw_enter,
		sw_blank_line,
		sw_file_content,
		sw_new_segment_minus_sign,
		sw_ignore_line,
		sw_almost_done,
		sw_header_almost_done
	} state;

	state = pHttp->MiMeState;
	pBody = pHttp->Body.Start;

	if (pHttp->FileNumber)
	{
		pHttp->File[pHttp->FileNumber-1].FileBuffer = pHttp->Body.Start;
		pHttp->File[pHttp->FileNumber-1].FileBufferEnd = pHttp->Body.End;
	}
	

	while(pBody < pHttp->Body.End)
	{
		ch = *pBody;
		switch(state)
		{
		case sw_start:
			if (ch == 'f')
			{
				filenamelength = 0;
				state = sw_filename_fi;
			}
			break;
		case sw_filename_fi:
			if (ch == 'i')
			{
				state = sw_filename_fil;
				break;
			}
			state = sw_start;
			break;
		case sw_filename_fil:
			if (ch == 'l')
			{
				state = sw_filename_file;
				break;
			}
			state = sw_start;
			break;
		case sw_filename_file:
			if (ch == 'e')
			{
				state = sw_filename_filen;
				break;
			}
			state = sw_start;
			break;
		case sw_filename_filen:
			if (ch == 'n')
			{
				state = sw_filename_filena;
				break;
			}
			state = sw_start;
			break;
		case sw_filename_filena:
			if (ch == 'a')
			{
				state = sw_filename_filenam;
				break;
			}
			state = sw_start;
			break;
		case sw_filename_filenam:
			if (ch == 'm')
			{
				state = sw_filename_filename;
				break;
			}
			state = sw_start;
		case sw_filename_filename:
			if (ch == 'e')
			{
				state = sw_equal_after_filename;
				break;
			}
			state = sw_start;
			break;
		case sw_equal_after_filename:
			if (ch == '=')
			{
				state = sw_quotation_after_filename;
			}
			else
			{
				state = sw_start;
			}
			break;
		case sw_quotation_after_filename:
			if (ch == '\"')
			{
				state = sw_filename;
				pHttp->File[pHttp->FileNumber].FileName = ExAllocatePoolWithTag(NonPagedPool, 512, 'xxxx');
				RtlZeroMemory(pHttp->File[pHttp->FileNumber].FileName, 512);
				pHttp->File[pHttp->FileNumber].offset = 0;
				pHttp->FileNumber++;
			}
			else
			{
				state = sw_start;
			}
			break;
		case sw_filename:
			if (ch == '\"')
			{
				state = sw_enter;
			}
			else
			{
				if (filenamelength==512)
				{
					filenamelength=0;
				}

				pHttp->File[pHttp->FileNumber-1].FileName[filenamelength] = ch;
				filenamelength++;
			}
			break;
		case sw_enter:
			if (ch == 0x0d)
			{
			}
			if (ch == 0x0a)
			{
				state = sw_blank_line;
			}
			break;
		case sw_blank_line:
			if (ch == 0x0d)
			{
			}
			else if (ch == 0x0a)
			{//更新FileBuffer
				state = sw_file_content;
				pHttp->File[pHttp->FileNumber-1].FileBuffer = pBody+1;
				break;
			}
			else
			{
				state = sw_enter;
			}


			break;
		case sw_file_content:
			//pBody指向文件的内容，开始判断文件结尾
			isContent = TRUE;
			//pHttp->File[pHttp->FileNumber-1].FileBufferEnd = pBody+1;
			if (ch == '-')
			{//更新FileBufferEnd
				//pHttp->File[pHttp->FileNumber-1].FileBufferEnd = pBody-2;
				state = sw_new_segment_minus_sign;
				break;
			}
			//else
			//{
			//	pHttp->File[pHttp->FileNumber-1].FileBufferEnd = pBody+1;
			//}
			break;

		case sw_new_segment_minus_sign:
			if (ch == '-')
			{
				if (!_strnicmp(pBody+1, pHttp->Boundary, strlen(pHttp->Boundary) ))
				{//有新节或到整个MIME尾部，重新开始
					pHttp->File[pHttp->FileNumber-1].FileBufferEnd = pBody-3;
					if (pHttp->File[pHttp->FileNumber-1].FileBufferEnd <= pHttp->File[pHttp->FileNumber-1].FileBuffer)
					{
						pHttp->File[pHttp->FileNumber-1].FileBufferEnd = pHttp->File[pHttp->FileNumber-1].FileBuffer = 0;
					}
					state = sw_start;
				}
				else
				{
					pBody--;
					state = sw_file_content;

				}
			}
			else
			{
				state = sw_file_content;

			}
			break;

		default:
			break;
		}
		pBody++;
	}

	if (state == sw_file_content)
	{
		pHttp->File[pHttp->FileNumber-1].FileBufferEnd = pHttp->Body.End;
	}

	pHttp->MiMeState = state;

	return 0;
}

UINT http_receive_ndis_packet(PAPP_FICTION_PROCESS pProcess)
{
	INT status = RECEIVE_STORAGE_PACKET;
	PUCHAR pHttp = NULL;
	UINT iHttpLen = 0;

	do 
	{
		//分配上下文
		if (pProcess->ReceiveContext == NULL)
		{
			pProcess->ReceiveContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(HTTP_REQUEST), 'xxxx');
			if (pProcess->ReceiveContext == NULL)
			{
				break;
			}

			RtlZeroMemory(pProcess->ReceiveContext, sizeof(HTTP_REQUEST));
		}

		status = HttpReceiveAnalysis(pProcess);

	} while (FALSE);

	return status;
}


UINT http_send_ndis_packet(PAPP_FICTION_PROCESS pProcess)
{
	UINT status = 0;

	do 
	{
		//分配上下文
		if (pProcess->SendContext == NULL)
		{
			pProcess->SendContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(HTTP_REQUEST), 'xxxx');
			if (pProcess->SendContext == NULL)
			{
				break;
			}

			RtlZeroMemory(pProcess->SendContext, sizeof(HTTP_REQUEST));
		}

		status = HttpSendAnalysis(pProcess);

	} while (FALSE);

	return status;
}