#include "smb.h"
#include "hash_fid.h"
#include "FsCrypt.h"

LIST_ENTRY g_SmbCreateList;

typedef struct _CREATE_INFOR
{
	LIST_ENTRY node;
	USHORT Mid;
	USHORT Fid;
	WCHAR FileName[1024];
}CREATE_INFOR, *PCREATE_INFOR;

VOID smbInsertCreateList(PLIST_ENTRY pEntry)
{
	InsertTailList(&g_SmbCreateList, pEntry);
}

BOOLEAN smbIsProcessFile(PWCHAR FileName)
{
	INT FileLength = wcslen(FileName);
	BOOLEAN bRtn = FALSE;
	//测试 txt doc tmp
	FileLength--;

	if (FileName[FileLength] == L'0')
	{//excel2003
		bRtn = TRUE;
	}

	if (FileName[FileLength-2] == L't' &&
		FileName[FileLength-1] == L'x' && 
		FileName[FileLength] == L't')
	{
		bRtn = TRUE;
	}

	if (FileName[FileLength-2] == L'p' &&
		FileName[FileLength-1] == L'd' && 
		FileName[FileLength] == L'f')
	{
		bRtn = TRUE;
	}

	if (FileName[FileLength-2] == L't' &&
		FileName[FileLength-1] == L'm' && 
		FileName[FileLength] == L'p')
	{
		bRtn = TRUE;
	}

	if (FileName[FileLength-2] == L'd' &&
		FileName[FileLength-1] == L'o' && 
		FileName[FileLength] == L'c')
	{
		bRtn = TRUE;
	}
	if (FileName[FileLength-2] == L'x' &&
		FileName[FileLength-1] == L'l' && 
		FileName[FileLength] == L's')
	{
		bRtn = TRUE;
	}
	if (FileName[FileLength-2] == L'p' &&
		FileName[FileLength-1] == L'p' && 
		FileName[FileLength] == L't')
	{
		bRtn = TRUE;
	}

	if (FileName[FileLength-3] == L'd' &&
		FileName[FileLength-2] == L'o' && 
		FileName[FileLength-1] == L'c' &&
		FileName[FileLength] == L'x')
	{
		bRtn = TRUE;
	}

	if (FileName[FileLength-3] == L'x' &&
		FileName[FileLength-2] == L'l' && 
		FileName[FileLength-1] == L's' &&
		FileName[FileLength] == L'x')
	{
		bRtn = TRUE;
	}

	if (FileName[FileLength-3] == L'p' &&
		FileName[FileLength-2] == L'p' && 
		FileName[FileLength-1] == L't' &&
		FileName[FileLength] == L'x')
	{
		bRtn = TRUE;
	}

	return bRtn;
}

PCREATE_INFOR smbGetCreateInfor(USHORT Mid)
{
	PLIST_ENTRY tempnode;
	PCREATE_INFOR pRtn = NULL;
	PCREATE_INFOR tempInfo = NULL;

	for (tempnode = g_SmbCreateList.Flink; tempnode != &g_SmbCreateList; tempnode = tempnode->Flink)
	{
		tempInfo = CONTAINING_RECORD(
			tempnode,
			CREATE_INFOR, 
			node
			);
		if (Mid == tempInfo->Mid)
		{
			RemoveEntryList(tempnode);
			pRtn = tempInfo;
			break;
		}
	}

	return pRtn;
}

VOID smbMoveMemory(PUCHAR des, PUCHAR src, ULONG length)
{
	PUCHAR buffer = ExAllocatePoolWithTag(NonPagedPool, length, 'xxxx');
	if (buffer == NULL)
	{
		ASSERT(FALSE);
	}

	RtlCopyMemory(buffer, src, length);
	RtlCopyMemory(des, buffer, length);
	ExFreePoolWithTag(buffer, 'xxxx');
}

//若修改数据，直接申请buffer，替换原来buffer
UINT smb_send_ndis_packet(PAPP_FICTION_PROCESS pProcess)
{
	UINT status = 0;
	INT index = 0;
	PSMB_HEADER smbHeader = (PSMB_HEADER)(pProcess->sendBuffer.Buffer+pProcess->sendBuffer.Offset);
	PSMB_SESSION pSession = NULL;
	ULONG ValidDataLength = 0;
	ULONG TailLength = 0;
	PFID_NET_FILE pNetFile = NULL;
	BOOLEAN bDecryptOk = FALSE;
	UINT step = 0;

	if (pProcess->Context == NULL)
	{
		pProcess->Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(SMB_SESSION), 'xxxx');
		ASSERT((pProcess->Context != NULL));
		RtlZeroMemory(pProcess->Context, sizeof(SMB_SESSION));
	}
	

	pSession = (PSMB_SESSION)pProcess->Context;

	if (smbHeader->Protocol[0] == 0xff &&
		smbHeader->Protocol[1] == 'S' &&
		smbHeader->Protocol[2] == 'M' &&
		smbHeader->Protocol[3] == 'B')
	{
		step = smb_start;//以请求头开始的
	}

	if (step == smb_start)
	{
		switch(smbHeader->Command)
		{
		case SMB_COM_NT_CREATE_ANDX:
			{
				PNT_CREATE_ANDX_REQUEST ntCreate = 
					(PNT_CREATE_ANDX_REQUEST)((PUCHAR)smbHeader+sizeof(SMB_HEADER));

				if (smbHeader->WordCount > 0)
				{
					PCREATE_INFOR p = NULL;

					do 
					{
						p = ExAllocatePoolWithTag(NonPagedPool, sizeof(CREATE_INFOR), 'xxxx');
					} while (p == NULL);
					RtlZeroMemory(p, sizeof(CREATE_INFOR));
					p->Mid = smbHeader->Mid;
					RtlMoveMemory(p->FileName, &ntCreate->FileName[1], ntCreate->FileNameLen);
					InitializeListHead(&p->node);
					smbInsertCreateList(&p->node);
				}

			}
			break;

		case SMB_COM_CLOSE:
			{
				psmb_com_close_req writerequest = 
					(psmb_com_close_req) ((PUCHAR)smbHeader+sizeof(SMB_HEADER));

				hfDeleteFid(writerequest->FileID);

			}
			break;

		case SMB_COM_READ_ANDX:
			{
				PREAD_ANDX_REQUEST readrequest = 
					(PREAD_ANDX_REQUEST) ((PUCHAR)smbHeader+sizeof(SMB_HEADER));

				pNetFile = hfFindFid(readrequest->Fid);

				if (pNetFile)
				{//凡是本id的读操作都加密
					PCREATE_INFOR p = ExAllocatePoolWithTag(NonPagedPool, sizeof(CREATE_INFOR), 'xxxx');
					RtlZeroMemory(p, sizeof(CREATE_INFOR));
					p->Fid = readrequest->Fid;
					p->Mid = smbHeader->Mid;
					InitializeListHead(&p->node);

					smbInsertCreateList(&p->node);
					if (pNetFile->read.isFirstRead)
					{
						pNetFile->read.isFirstRead = FALSE;
						pNetFile->isEncryptFile = TRUE;
					}

					pNetFile->read.bChangeRequest = FALSE;

					pNetFile->read.Offset = readrequest->HighOffset;
					pNetFile->read.Offset <<= 32;
					pNetFile->read.Offset += readrequest->Offset;

					pNetFile->read.FirstOffset = pNetFile->read.Offset;

					pNetFile->read.FirstLength = readrequest->MaxCountHigh;
					pNetFile->read.FirstLength <<= 16;
					pNetFile->read.FirstLength += readrequest->MaxCountLow;

					if (pNetFile->read.FirstOffset < 512/*% 512 != 0*/)
					{//测试修正偏移，powerpnt 2003
						UINT remainder = pNetFile->read.FirstOffset%512;
						UINT Size			= pNetFile->read.FirstLength/512;

						if (pNetFile->read.FirstOffset == 0 && pNetFile->read.FirstLength >= 512)
						{
						}
						else
						{
							readrequest->Offset -= remainder;
							readrequest->MinCount = (Size+1)*512;
							readrequest->MaxCountLow = (Size+1)*512;
							//readrequest->MaxCountHigh = (Size+1)*512;
							readrequest->Remaining = (Size+1)*512;

							pNetFile->read.bChangeRequest = TRUE;

							pNetFile->read.Offset = readrequest->Offset;
							FLAG_SET_UINT(status, CHANGE_PACKET_DATA);
						}
					}
				}


			}
			break;

		case SMB_COM_WRITE_ANDX:
			{
				UINT Padding = 0;
				PWRITE_ANDX_REQUEST writerequest = 
					(PWRITE_ANDX_REQUEST) ((PUCHAR)smbHeader+sizeof(SMB_HEADER));
				
				PFID_NET_FILE pNetFile = hfFindFid(writerequest->Fid);

				if (pNetFile &&
				(writerequest->Offset % 512 != 0 || writerequest->DataLength < 512)
				)
				{
					KdPrint(("write request offset =%d\n", writerequest->Offset));
				}

				//KdPrint(("Port=%d pProcess=%x pre write\n", pProcess->desPort, pProcess));
				if (pNetFile)
				{
					if (writerequest->DataLengthHigh)
					{
						pNetFile->write.DataLength = writerequest->DataLengthHigh;
						pNetFile->write.DataLength <<= 16;
						pNetFile->write.DataLength += writerequest->DataLength;
						
					}
					else
					{
						pNetFile->write.DataLength = writerequest->DataLength;
					}
					
					pNetFile->write.Offset = writerequest->OffsetHigh;
					pNetFile->write.Offset <<= 32;
					pNetFile->write.Offset += writerequest->Offset;

					Padding = writerequest->ByteCount - pNetFile->write.DataLength;
					ValidDataLength = pProcess->sendBuffer.length - sizeof(SMB_HEADER) - (writerequest->Data - (PUCHAR)writerequest);
					ValidDataLength -= Padding;
					
					KdPrint(("    writefile offset=%d validedatalength=%d\n", writerequest->Offset, ValidDataLength));

					if (pNetFile->write.Offset >= 512)
					{
						FsRandomEndecrypt(
							&writerequest->Data[Padding],
							ValidDataLength,
							pNetFile->write.Offset,
							"01234567890123456789",
							pSession->KeyBox
							);
						pNetFile->write.Offset += ValidDataLength;
						pNetFile->write.DataLength -= ValidDataLength;
						FLAG_SET_UINT(status, CHANGE_PACKET_DATA);
					}
					else
					{
						if (pNetFile->write.Offset == 0 && ValidDataLength >= 512
							)
						{
							bDecryptOk = FsReadBlockDecrypt(
								&writerequest->Data[Padding],
								ValidDataLength,
								pNetFile->write.Offset,
								"01234567890123456789",
								pSession->KeyBox
								);
						}
						else if(pNetFile->write.Offset == 0 && ValidDataLength < 512)
						{
							bDecryptOk = FsReadBlockDecrypt(
								&writerequest->Data[Padding],
								ValidDataLength,
								pNetFile->write.Offset,
								"01234567890123456789",
								pSession->KeyBox
								);
						}
						else if (pNetFile->write.Offset != 0)
						{
							pNetFile->isEncryptFile = FALSE;
							//ASSERT(FALSE);
						}

						pNetFile->write.DataLength -= ValidDataLength;
						pNetFile->write.Offset += ValidDataLength;

						if (!bDecryptOk)
						{
							pNetFile->isEncryptFile = FALSE;
						}
						else
						{
							FLAG_SET_UINT(status, CHANGE_PACKET_DATA);
						}
					}
				}
				else if(pNetFile)
				{
					KdPrint(("write request xxxxxxxx\n"));
				}
			}
			break;

		

		default:
			;
		}
	}
	else/* if (pSession->step == smb_write_data)*/
	{//全部都是要写入的数据
		//没有netbios
		
		if (pNetFile->isEncryptFile == TRUE/*pNetFile->write.decryptData==TRUE*/)
		{
			KdPrint(("%x -- %x\n", pProcess->sendBuffer.Buffer[0], pProcess->sendBuffer.Buffer[pProcess->sendBuffer.MaximumLength-1]));
			ValidDataLength = pProcess->sendBuffer.MaximumLength;

			if (pNetFile->write.Offset >= 512)
			{
				FsRandomEndecrypt(
					pProcess->sendBuffer.Buffer,
					ValidDataLength,
					pNetFile->write.Offset,
					"01234567890123456789",
					pSession->KeyBox
					);
				pNetFile->write.Offset += ValidDataLength;
				pNetFile->write.DataLength -= ValidDataLength;
				FLAG_SET_UINT(status, CHANGE_PACKET_DATA);
			}
			else
			{
				if(pNetFile->write.Offset == 0 && ValidDataLength >= 512)
				{
					bDecryptOk = FsReadBlockDecrypt(
						pProcess->sendBuffer.Buffer,
						ValidDataLength,
						pNetFile->write.Offset,
						"01234567890123456789",
						pSession->KeyBox
						);
				}
				else if(pNetFile->write.Offset == 0 && ValidDataLength < 512)
				{
					bDecryptOk = FsReadBlockDecrypt(
						pProcess->sendBuffer.Buffer,
						ValidDataLength,
						pNetFile->write.Offset,
						"01234567890123456789",
						pSession->KeyBox
						);
				}
				else if (pNetFile->write.Offset != 0)
				{
					ASSERT(FALSE);
				}

				if (!bDecryptOk)
				{
					pNetFile->isEncryptFile = FALSE;
				}
				else
				{
					pNetFile->write.Offset += ValidDataLength;
					pNetFile->write.DataLength -= ValidDataLength;
					FLAG_SET_UINT(status, CHANGE_PACKET_DATA);
				}
			}
		}
	}

	return status;
}

UINT smb_receive_ndis_packet(PAPP_FICTION_PROCESS pProcess)
{
	UINT status = 0;
	PSMB_HEADER smbHeader = (PSMB_HEADER)(pProcess->receiveBuffer.Buffer+pProcess->receiveBuffer.Offset);
	PSMB_SESSION pSession = NULL;
	ULONG ValidLength = 0;
	UINT index = 0;
	UINT Padding = 0;
	UINT TailLength = 0;
	PFID_NET_FILE pNetFile = NULL;
	PCREATE_INFOR createInfo = NULL;
	UINT step = 0;

	if (pProcess->Context == NULL)
	{
		pProcess->Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(SMB_SESSION), 'xxxx');

		ASSERT((pProcess->Context != NULL));
		NdisZeroMemory(pProcess->Context, sizeof(SMB_SESSION));
	}

	pSession = (PSMB_SESSION)pProcess->Context;

	if (smbHeader->Protocol[0] == 0xff &&
		smbHeader->Protocol[1] == 'S' &&
		smbHeader->Protocol[2] == 'M' &&
		smbHeader->Protocol[3] == 'B')
	{
		step = smb_start;
	}
	else
	{
		step = smb_no_head;
	}

	if (step == smb_start)
	{
		switch(smbHeader->Command)
		{
		case SMB_COM_NT_CREATE_ANDX:
			{
				PNT_CREATE_ANDX_RSP createRsp =
					(PNT_CREATE_ANDX_RSP)((PUCHAR)smbHeader+sizeof(SMB_HEADER));
				

				if (
					!createRsp->DirectoryFlag &&
					!(createRsp->FileAttributes & SMB_DEVICE_FILE) &&
					!(createRsp->FileAttributes & SMB_HIDDEN_FILE) &&
					!(createRsp->FileAttributes & SMB_READ_ONLY) &&
					!(createRsp->FileAttributes & SMB_SYSTEM_FILE)  &&
					!(createRsp->FileAttributes & SMB_COMPRESSED_FILE)  &&
					(createRsp->FileAttributes & SMB_ARCHIVE) &&
					smbHeader->WordCount > 0 &&
					smbHeader->Status.Status == 0
					)
				{
					PFID_NET_FILE fid = NULL;
					PCREATE_INFOR createInfo = NULL;
					//KdPrint(("%wZ Fid=%x\n", &g_CreateFileName, createRsp->Fid));
					
					createInfo = smbGetCreateInfor(smbHeader->Mid);
					if (createInfo && smbIsProcessFile(createInfo->FileName))
					{//指定结尾的文件进行处理
						fid = ExAllocatePoolWithTag(NonPagedPool, sizeof(FID_NET_FILE), 'xxxx');
						fid->Fid = createRsp->Fid;
						fid->FileName.Length = wcslen(createInfo->FileName) * sizeof(WCHAR);
						fid->FileName.MaximumLength = fid->FileName.Length+2;
						fid->FileName.Buffer = ExAllocatePoolWithTag(NonPagedPool, fid->FileName.MaximumLength, 'xxxx');
						RtlZeroMemory(fid->FileName.Buffer, fid->FileName.MaximumLength);
						RtlCopyMemory(fid->FileName.Buffer, createInfo->FileName, fid->FileName.Length);
						ExFreePoolWithTag(createInfo, 'xxxx');
						fid->isEncryptFile = TRUE;

						//KdPrint(("FileName=%wZ\n", &fid->FileName));

						hfInsertFid(fid);
						ExFreePoolWithTag(fid, 'xxxx');
					}
				}
				else
				{//删除不处理的信息
					PCREATE_INFOR createInfo = NULL;
					createInfo = smbGetCreateInfor(smbHeader->Mid);
					if (createInfo)
					{
						ExFreePoolWithTag(createInfo, 'xxxx');
					}
					
				}

			}
			break;
		case SMB_COM_READ_ANDX:
			{
				PSMB_COM_READ_RSP readresponse =
					(PSMB_COM_READ_RSP) ((PUCHAR)smbHeader+sizeof(SMB_HEADER));
				//KdPrint(("post read Port=%d pProcess=%x Context=%x\n", pProcess->desPort, pProcess, pProcess->Context));
				createInfo = smbGetCreateInfor(smbHeader->Mid);
				if (createInfo)
				{
					pNetFile = hfFindFid(createInfo->Fid);
					ExFreePoolWithTag(createInfo, 'xxxx');
				}
				if (pNetFile && pNetFile->isEncryptFile /*pNetFile->ReadEncrypt*/ && smbHeader->Status.Status == 0)
				{
					pSession->NetFile = pNetFile;

					ValidLength = pProcess->receiveBuffer.length - sizeof(SMB_HEADER) - (&readresponse->Data[0] - (PUCHAR)readresponse);
					if (readresponse->DataLengthHigh)
					{
						pNetFile->read.DataLength = readresponse->DataLengthHigh;
						pNetFile->read.DataLength <<= 16;
						pNetFile->read.DataLength += readresponse->DataLength;
					}
					else
					{
						pNetFile->read.DataLength = readresponse->DataLength;
					}

					Padding = readresponse->ByteCount - readresponse->DataLength;
					ValidLength -= Padding;

					KdPrint(("read %x-%x Padding=%d Offset=%x DataLength=%x\n", 
						readresponse->Data[Padding], 
						readresponse->Data[ValidLength-1], 
						Padding,
						pNetFile->read.Offset, 
						pNetFile->read.DataLength));
					if (pNetFile->read.Offset >= 512)
					{
						FsRandomEndecrypt(
							readresponse->Data+Padding, 
							ValidLength, 
							pNetFile->read.Offset,
							"01234567890123456789", 
							pSession->KeyBox
						);
						pNetFile->read.DataLength -= ValidLength;
						pNetFile->read.Offset += ValidLength;
						FLAG_SET_UINT(status, CHANGE_PACKET_DATA);
					}
					else
					{
						if(pNetFile->read.Offset == 0 && ValidLength >= 512)
						{
							if (FsWriteBlockEncrypt(
								readresponse->Data+Padding, 
								ValidLength, 
								pNetFile->read.Offset,
								"01234567890123456789", 
								pSession->KeyBox
								))
							{
								pNetFile->read.DataLength -= ValidLength;
								pNetFile->read.Offset += ValidLength;
								FLAG_SET_UINT(status, CHANGE_PACKET_DATA);
							}
							else
							{
								pNetFile->isEncryptFile = FALSE;
							}
							
						}
						else if(pNetFile->read.Offset == 0 && ValidLength < 512)
						{
							if (FsWriteBlockEncrypt(
								readresponse->Data+Padding, 
								ValidLength, 
								pNetFile->read.Offset,
								"01234567890123456789", 
								pSession->KeyBox
								))
							{
								pNetFile->read.DataLength -= ValidLength;
								pNetFile->read.Offset += ValidLength;
								FLAG_SET_UINT(status, CHANGE_PACKET_DATA);
							}
							else
							{
								pNetFile->isEncryptFile = FALSE;
							}
							
						}
						else
						{
							ASSERT(FALSE);
						}

						if (pNetFile->read.bChangeRequest)
						{
							int remainder = pNetFile->read.FirstOffset%512;
							if (remainder > 0)
							{
								RtlCopyMemory(&readresponse->Data[Padding], 
									&readresponse->Data[Padding+remainder], pNetFile->read.FirstLength);
							}

							readresponse->DataLength = pNetFile->read.FirstLength;
							readresponse->ByteCount = readresponse->DataLength + Padding;
						}
						//KdPrint(("encrypt read data\n"));
					}
					
				}

			}
			break;

		default:
			;
		}
	}
	else if (pSession->NetFile && pSession->NetFile->valide)
	{//没有请求头，是请求的数据部分

		pNetFile = pSession->NetFile;
		if (pNetFile->isEncryptFile/*pNetFile->read.encryptData*/)
		{
			ValidLength = pProcess->receiveBuffer.MaximumLength;

			KdPrint(("h readfile %x-%x Offset=%x DataLength=%x\n", 
				pProcess->receiveBuffer.Buffer[0], 
				pProcess->receiveBuffer.Buffer[ValidLength-1],
				pNetFile->read.Offset,
				pNetFile->read.DataLength));

			if (pNetFile->read.Offset >= 512)
			{
				FsRandomEndecrypt(
					pProcess->receiveBuffer.Buffer, 
					ValidLength, 
					pNetFile->read.Offset,
					"01234567890123456789", 
					pSession->KeyBox
					);
				pNetFile->read.DataLength -= ValidLength;
				pNetFile->read.Offset += ValidLength;
				FLAG_SET_UINT(status, CHANGE_PACKET_DATA);
			}
			else
			{
				if(pNetFile->read.Offset == 0 && ValidLength >= 512)
				{
					if (FsWriteBlockEncrypt(
						pProcess->receiveBuffer.Buffer, 
						ValidLength, 
						pNetFile->read.Offset,
						"01234567890123456789", 
						pSession->KeyBox
						))
					{
						pNetFile->read.DataLength -= ValidLength;
						pNetFile->read.Offset += ValidLength;
						FLAG_SET_UINT(status, CHANGE_PACKET_DATA);
					}
					else
					{
						pNetFile->isEncryptFile = FALSE;
					}
					
				}
				else if(pNetFile->read.Offset == 0 && ValidLength < 512)
				{
					if (FsWriteBlockEncrypt(
						pProcess->receiveBuffer.Buffer, 
						ValidLength, 
						pNetFile->read.Offset,
						"01234567890123456789", 
						pSession->KeyBox
						))
					{
						pNetFile->read.DataLength -= ValidLength;
						pNetFile->read.Offset += ValidLength;
						FLAG_SET_UINT(status, CHANGE_PACKET_DATA);
					}
					else
					{
						pNetFile->isEncryptFile = FALSE;
					}

					
				}
				else
				{
					pNetFile->isEncryptFile = FALSE;
					//ASSERT(FALSE);
				}

				if (pNetFile->read.bChangeRequest)
				{
					int remainder = pNetFile->read.FirstOffset%512;
					if (remainder > 0)
					{
						RtlCopyMemory(pProcess->receiveBuffer.Buffer, 
							pProcess->receiveBuffer.Buffer+remainder, pNetFile->read.FirstLength);
					}
				}
			}
			
		}
	}

	return status;
}

VOID smbClearReceiveBuffer(PAPP_FICTION_PROCESS pProcess)
{//receive后可能发生
	PSMB_SESSION pSession = (PSMB_SESSION)pProcess->Context;
	PFID_NET_FILE pNetFile = pSession->NetFile;

	if (pNetFile)
	{
		pNetFile->read.DataLength = pNetFile->read.FirstLength;
		pNetFile->read.Offset = pNetFile->read.FirstOffset;

		pNetFile->ReceiveBufferLen = 0;
	}
}