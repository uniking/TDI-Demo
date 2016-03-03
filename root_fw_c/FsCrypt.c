#include "FsCrypt.h"
#include "FsCryptAlg.h"
#include "FsCompress.h"
#include <ndis.h>

BOOLEAN
FsDecryptDataByOffset_sub_80A27CF4(
								   IN PUCHAR Data, 
								   IN LARGE_INTEGER Offset,
								   IN ULONG DataSize,
								   IN PUCHAR PolicyPwd, 
								   IN PUCHAR Buffer
								   )
{
	ULONG V17Length = 0;
	PUCHAR DataPos = Data;
	UCHAR TempBuffer[FS_FILE_DATA_HEADER_SIZE] = {0};
	ULONG V9RemainOffset = 0;
	ULONG V10RemainData = 0;

	// offset = 0, file begging
	if (!Offset.QuadPart)
	{
		if (DataSize < FS_FILE_DATA_MINSIZE)
		{
			return FALSE;
		}

		V17Length = FS_FILE_DATA_HEADER_SIZE;
		if (DataSize < V17Length)
		{
			V17Length = DataSize;
		}
		//if (!FsReadDecryptHeader(DataPos, &V17Length, PolicyPwd)
		//	|| DataSize < V17Length)
		//{
		//	return FALSE;
		//}

		DataPos += V17Length;
		DataSize -= V17Length;
		Offset.QuadPart = V17Length;
	}

	//
	// two conditions
	// 1: offset is 0, header already decrypted, start to decrypt data
	// 2: offset is not 0, start to decrypt data
	// 
	if (!DataSize)
	{
		return TRUE;
	}
	if (Offset.HighPart || Offset.LowPart >= FS_FILE_DATA_HEADER_SIZE)
	{
		V9RemainOffset = Offset.LowPart & 0x1FF;
		if (V9RemainOffset)
		{
			V10RemainData = FS_FILE_DATA_HEADER_SIZE - V9RemainOffset;
			if (V9RemainOffset + DataSize < FS_FILE_DATA_HEADER_SIZE 
				|| V10RemainData > DataSize)
			{
				V10RemainData = DataSize;
			}

			memcpy(TempBuffer+V9RemainOffset, DataPos, V10RemainData);
			FsEnDecryptDataMore(TempBuffer, FS_FILE_DATA_HEADER_SIZE, PolicyPwd, Buffer);
			memcpy(DataPos, TempBuffer+V9RemainOffset, V10RemainData);

			DataPos += V10RemainData;
			DataSize -= V10RemainData;
		}

		FsEnDecryptDataMore(DataPos, DataSize, PolicyPwd, Buffer);
		return TRUE;	
	}
	else
	{
		return FALSE;
	}
}

NTSTATUS 
FsEnDecryptDataMore(
					PVOID Data, 
					ULONG DataLength, 
					BYTE *pucKeyIn, 
					BYTE *pucKeyOut
					)
{
	ULONG RemainLength = 0;
	ULONG Length = 0;
	NTSTATUS result = 0;

	for ( RemainLength = DataLength; RemainLength; RemainLength -= Length )
	{
		Length = FS_CACHE_HEADERDATA_SIZE;
		if ( RemainLength <= FS_FILE_DATA_HEADER_SIZE )
		{
			Length = RemainLength;
		}

		//FsKeyProcess(pucKeyOut, pucKeyIn, 16);
		result = FsDataProcess(pucKeyOut, Data, Length);
		Data = (char *)Data + Length;
	}

	return result;
}

NTSTATUS 
FsEnDecryptDataBlockMore(
					PVOID Data, 
					ULONG DataLength, 
					BYTE *pucKeyIn, 
					BYTE *pucKeyOut
					)
{
	ULONG RemainLength = 0;
	ULONG Length = 0;
	NTSTATUS result;

	for ( RemainLength = DataLength; RemainLength; RemainLength -= Length )
	{
		Length = FS_CACHE_HEADERDATA_SIZE;
		if ( RemainLength <= FS_FILE_DATA_HEADER_SIZE )
		{
			Length = RemainLength;
		}

		FsKeyProcess(pucKeyOut, pucKeyIn, 16);
		result = FsDataProcess(pucKeyOut, Data, Length);
		Data = (char *)Data + Length;
	}

	return result;
}

//BOOLEAN
//FsWriteReadDecryptEx_sub_80A3BEE2(
//								  IN PUCHAR Data, 
//								  IN PLARGE_INTEGER Offset,
//								  IN ULONG DataSize,
//								  IN PFS_SHIELD_CONTEXT ShieldContext
//								  )
//{
//	BOOLEAN Result = FALSE;
//	NTSTATUS Status = 0;
//	PVOID Buffer = NULL;
//	PVOID Buffer108 = NULL;
//	ULONG ReadLength = 0;
//	ULONG OffsetRemainder = Offset->LowPart&0x1FF;
//	ULONG V15Length = 0;
//	ULONG V20Length = 0;
//	
//	do 
//	{
//		Buffer = ExAllocateFromNPagedLookasideList(&g_NBuffer440F83EBBB8);
//	} while (!Buffer);
//	Buffer108 = (PUCHAR)Buffer + 0x108;
//
//	// 
//	if (!Offset->QuadPart && DataSize > FS_FILE_DATA_HEADER_SIZE)
//	{
//		if (!FsFakeHeaderVerify((PFS_ENCRYPT_FILE_HEAD)Data))
//		{
//			ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//			return FALSE;
//		}
//		
//		ReadLength = FS_FILE_DATA_HEADER_SIZE;
//		if (!FsReadDecryptHeader(Data, &ReadLength, ShieldContext->_24CPolicyPwd))
//		{
//			ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//			return FALSE;	
//		}
//		
//		ShieldContext->_248Flags = TABLE_230FLAGS_1;
//		memcpy(ShieldContext->_48HeaderData, Data, FS_FILE_DATA_HEADER_SIZE);
//
//		Data += FS_FILE_DATA_HEADER_SIZE;
//		DataSize -= FS_FILE_DATA_HEADER_SIZE;
//
//		FsEnDecryptDataMore(Data, DataSize, ShieldContext->_24CPolicyPwd, Buffer);
//		ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//
//		return TRUE;
//	}
//
//	//
//	if (!OffsetRemainder && Offset->QuadPart >= FS_FILE_DATA_HEADER_SIZE)
//	{
//		FsEnDecryptDataMore(Data, DataSize, ShieldContext->_24CPolicyPwd, Buffer);
//		ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//
//		return TRUE;
//	}
//
//	//
//	// 
//	if (OffsetRemainder + DataSize < FS_FILE_DATA_HEADER_SIZE
//		|| (V15Length = FS_FILE_DATA_HEADER_SIZE-OffsetRemainder, V15Length > DataSize))
//	{
//		V15Length = DataSize;
//	}
//	if (Offset->QuadPart >= FS_FILE_DATA_HEADER_SIZE)
//	{
//		memcpy((PUCHAR)Buffer108+OffsetRemainder, Data, V15Length);
//		FsEnDecryptDataMore(Buffer108, FS_FILE_DATA_HEADER_SIZE, ShieldContext->_24CPolicyPwd, Buffer);
//		
//		memcpy(Data, (PUCHAR)Buffer108+OffsetRemainder, V15Length);
//		DataSize -= V15Length;
//		Data += V15Length;
//
//		FsEnDecryptDataMore(Data, DataSize, ShieldContext->_24CPolicyPwd, Buffer);
//		ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//
//		return TRUE;
//	}
//	else
//	{
//		if (!Offset->QuadPart)
//		{
//			if (!FsFakeHeaderVerify((PFS_ENCRYPT_FILE_HEAD)Data))
//			{
//				ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//				return FALSE;		
//			}
//			
//			V20Length = FS_FILE_DATA_HEADER_SIZE;
//			if (DataSize <= FS_FILE_DATA_HEADER_SIZE)
//			{
//				V20Length = DataSize;
//			}
//			if (FsReadDecryptHeader(Data, &V20Length, ShieldContext->_24CPolicyPwd))
//			{
//				if (V20Length > DataSize)
//				{
//					V20Length = DataSize;
//				}
//				memcpy(ShieldContext->_48HeaderData, Data, V20Length);
//				ShieldContext->_248Flags = TABLE_230FLAGS_1;
//			}
//		}
//
//		if (!ShieldContext->_248Flags)
//		{
//			ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//			return FALSE;	
//		}
//		
//		memcpy(Data, ShieldContext->_48HeaderData+OffsetRemainder, V15Length);
//		DataSize -= V15Length;
//		Data += V15Length;
//
//		FsEnDecryptDataMore(Data, DataSize, ShieldContext->_24CPolicyPwd, Buffer);
//		ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//
//		return TRUE;
//	}					
//}

//BOOLEAN
//FsWriteWriteEncrypt_sub_80A2A812(
//								 IN PUCHAR Data, 
//								 IN PLARGE_INTEGER Offset,
//								 IN ULONG DataSize,
//								 IN PFS_DEVICE_EXTENSION DevExt,
//								 IN PFILE_OBJECT FileObject,
//								 IN PFS_SHIELD_CONTEXT ShieldContext,
//								 IN BOOLEAN IrpPaging
//								 )
//{
//	BOOLEAN Result = FALSE;
//	NTSTATUS Status = 0;
//	PDEVICE_OBJECT DeviceObject = DevExt->_8AttachedToDeviceObject;
//	ULONG OffsetRemainder = Offset->LowPart&0x1FF; 
//	PVOID Buffer = NULL;
//	PVOID Buffer108 = NULL;	
//	ULONG V30Length = 0;
//	ULONG V36Length = 0;
//	ULONG V14Length = 0;
//	LARGE_INTEGER WriteOffset = {0};
//	
//	do 
//	{
//		Buffer = ExAllocateFromNPagedLookasideList(&g_NBuffer440F83EBBB8);
//	} while (!Buffer);
//	Buffer108 = (PUCHAR)Buffer + 0x108;
//	
//	// 
//	if (Offset->QuadPart || DataSize < FS_FILE_DATA_HEADER_SIZE)
//	{
//		//
//		if (!ShieldContext->_264Flags)
//		{
//			BOOLEAN ReadEncryptResult = FALSE;
//			ReadEncryptResult = FsWriteReadEncrypt(
//				DeviceObject,
//				FileObject,
//				ShieldContext->_24CPolicyPwd,
//				ShieldContext->_268FileSize
//				);
//			if (FlagOn(ShieldContext->_270Flags, TABLE_258FLAGS_1))
//			{				
//				ClearFlag(ShieldContext->_270Flags, TABLE_258FLAGS_1); 
//			}
//
//			if (ReadEncryptResult != 1)
//			{
//				ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//
//				if (Offset->QuadPart < FS_FILE_DATA_HEADER_SIZE)
//				{
//					if (OffsetRemainder + DataSize < FS_FILE_DATA_HEADER_SIZE
//						|| (V30Length = FS_FILE_DATA_HEADER_SIZE-OffsetRemainder, V30Length > DataSize))
//					{
//						V30Length = DataSize;
//					}
//					memcpy(ShieldContext->_48HeaderData+OffsetRemainder, Data, V30Length);
//				}
//
//				return FALSE;
//			}
//			
//			//
//			if (!ShieldContext->_248Flags)
//			{
//				FsIrpReadHeader(DeviceObject, FileObject, ShieldContext->_48HeaderData);
//			}	
//
//			ShieldContext->_264Flags = TABLE_24CFLAGS_1;
//		}//if (!ShieldContext->_264Flags)
//		
//		//
//		if (!OffsetRemainder && Offset->QuadPart >= FS_FILE_DATA_HEADER_SIZE)
//		{
//			ShieldContext->_264Flags = TABLE_24CFLAGS_1;
//			FsEnDecryptDataMore(Data, DataSize, ShieldContext->_24CPolicyPwd, Buffer);
//			ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//			
//			return TRUE;
//		}
//		
//		// 
//		if (OffsetRemainder + DataSize < FS_FILE_DATA_HEADER_SIZE
//			|| (V30Length = FS_FILE_DATA_HEADER_SIZE-OffsetRemainder, V30Length > DataSize))
//		{
//			V30Length = DataSize;
//		}
//		if (Offset->QuadPart >= FS_FILE_DATA_HEADER_SIZE)
//		{
//			memcpy((PUCHAR)Buffer108+OffsetRemainder, Data, V30Length);
//			FsEnDecryptDataMore(Buffer108, FS_FILE_DATA_HEADER_SIZE, ShieldContext->_24CPolicyPwd, Buffer);
//			memcpy(Data, (PUCHAR)Buffer108+OffsetRemainder, V30Length);
//			
//			DataSize -= V30Length;
//			Data += V30Length;
//
//			ShieldContext->_264Flags = TABLE_24CFLAGS_1;
//			FsEnDecryptDataMore(Data, DataSize, ShieldContext->_24CPolicyPwd, Buffer);
//			ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//
//			return TRUE;
//		}
//
//		//
//		if (!ShieldContext->_248Flags)
//		{
//			FsIrpReadHeader(DeviceObject, FileObject, ShieldContext->_48HeaderData);
//			if (!ShieldContext->_248Flags)
//			{
//				ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//
//				if (ShieldContext->_264Flags != TABLE_24CFLAGS_1
//					|| (ShieldContext->_268FileSize.QuadPart < DataSize && !Offset->QuadPart))
//				{
//					ShieldContext->_264Flags = 0;
//					return FALSE;
//				}
//
//				FsWriteReadDecrypt(DeviceObject, FileObject, ShieldContext->_24CPolicyPwd, ShieldContext->_268FileSize);
//				ShieldContext->_264Flags = 0;
//				return FALSE;
//			}
//		}//if (!ShieldContext->_248Flags)
//
//		//
//		memcpy(ShieldContext->_48HeaderData+OffsetRemainder, Data, V30Length);
//		memcpy(Buffer108, ShieldContext->_48HeaderData, FS_FILE_DATA_HEADER_SIZE);
//		if (ShieldContext->_268FileSize.QuadPart < FS_FILE_DATA_HEADER_SIZE)
//		{
//			ShieldContext->_248Flags = TABLE_230FLAGS_8;
//			if (IrpPaging)
//			{
//				V36Length = ShieldContext->_268FileSize.LowPart;
//_69:
//				if (!FsWriteEncryptHeader(Buffer108, V36Length, ShieldContext->_24CPolicyPwd, Buffer))
//				{
//					ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//
//					if (ShieldContext->_264Flags != TABLE_24CFLAGS_1
//						|| (ShieldContext->_268FileSize.QuadPart < DataSize && !Offset->QuadPart))
//					{
//						ShieldContext->_264Flags = 0;
//						return FALSE;
//					}
//
//					FsWriteReadDecrypt(DeviceObject, FileObject, ShieldContext->_24CPolicyPwd, ShieldContext->_268FileSize);
//					ShieldContext->_264Flags = 0;
//					return FALSE;
//				}
//
//				memcpy(Data, (PUCHAR)Buffer108+OffsetRemainder, V30Length);
//				FsIrpReadWrite(
//					IRP_MJ_WRITE,
//					DeviceObject,
//					FileObject,
//					&WriteOffset,
//					Buffer108,
//					FS_FILE_DATA_HEADER_SIZE,
//					0x43,
//					NULL
//					);
//
//				DataSize -= V30Length;
//				Data += V30Length;
//				
//				ShieldContext->_264Flags = TABLE_24CFLAGS_1;
//				FsEnDecryptDataMore(Data, DataSize, ShieldContext->_24CPolicyPwd, Buffer);
//				ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//
//				return TRUE;
//			}//if (IrpPaging)
//		}//if (ShieldContext->_268FileSize.QuadPart < FS_FILE_DATA_HEADER_SIZE)
//
//		V36Length = FS_FILE_DATA_HEADER_SIZE;		
//		goto _69;
//
//	}//if (Offset->QuadPart || DataSize < FS_FILE_DATA_HEADER_SIZE)
//	
//	//
//	if (FsFakeHeaderVerify((PFS_ENCRYPT_FILE_HEAD)Data))
//	{
//		ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//		ShieldContext->_264Flags = TABLE_24CFLAGS_3;
//		return FALSE;
//	}
//
//	//
//	memcpy(ShieldContext->_48HeaderData, Data, FS_FILE_DATA_HEADER_SIZE);
//	ShieldContext->_248Flags = TABLE_230FLAGS_1;
//	memcpy(Buffer108, Data, FS_FILE_DATA_HEADER_SIZE);
//	V14Length = FS_FILE_DATA_HEADER_SIZE;
//	if (ShieldContext->_268FileSize.QuadPart < FS_FILE_DATA_HEADER_SIZE)
//	{
//		if (IrpPaging)
//		{
//			V14Length = ShieldContext->_268FileSize.QuadPart;
//		}
//		ShieldContext->_248Flags = TABLE_230FLAGS_8;
//	}
//	
//	if (!FsWriteEncryptHeader(Buffer108, V14Length, ShieldContext->_24CPolicyPwd, Buffer))
//	{
//		if (ShieldContext->_264Flags != TABLE_24CFLAGS_1
//			|| ShieldContext->_268FileSize.HighPart < 0
//			|| (ShieldContext->_268FileSize.HighPart <= 0 && ShieldContext->_268FileSize.LowPart < DataSize))
//		{
//			ShieldContext->_264Flags = 0;
//			return FALSE;
//		}
//
//		FsWriteReadDecrypt(DeviceObject, FileObject, ShieldContext->_24CPolicyPwd, ShieldContext->_268FileSize);
//		ShieldContext->_264Flags = 0;
//		return FALSE;	
//	}
//
//	//
//	if (ShieldContext->_268FileSize.QuadPart > DataSize 
//		&& !ShieldContext->_264Flags)
//	{
//		if (FlagOn(ShieldContext->_270Flags, TABLE_258FLAGS_1))
//		{
//			ClearFlag(ShieldContext->_270Flags, TABLE_258FLAGS_1);
//		}
//		else
//		{
//			if (FsWriteReadEncryptEx_sub_80A26732(
//				DeviceObject, 
//				FileObject, 
//				ShieldContext->_24CPolicyPwd, 
//				ShieldContext->_268FileSize) != 1)
//			{
//				ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//				return FALSE;
//			}
//		}
//	}
//	
//	//
//	memcpy(Data, Buffer108, FS_FILE_DATA_HEADER_SIZE);
//	Data += FS_FILE_DATA_HEADER_SIZE;
//	DataSize -= FS_FILE_DATA_HEADER_SIZE;
//		
//	ShieldContext->_264Flags = TABLE_24CFLAGS_1;
//	FsEnDecryptDataMore(Data, DataSize, ShieldContext->_24CPolicyPwd, Buffer);
//	ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, Buffer);
//
//	return TRUE;
//}

BOOLEAN FsFakeHeaderVerify(PFS_ENCRYPT_FILE_HEAD FileHeader)
{
	BOOLEAN result = FALSE;
	int Flag;


	if ( FileHeader->Flag >= (unsigned int)Flag1 )
	{
		if ( FileHeader->Flag <= (unsigned int)Flag_standardEncrypt )
		{
			result = TRUE;
			if ( FileHeader->EncryptDataOffset > unk_0x58 )
			{//偏移大于0x58，有空间写入lock标记
				result = FsFileHeaderVerify((PFS_POLYCE_FILE_HEAD)FileHeader, FS_NAME_LOCK);
			}

			return result;
		}
		if ( FileHeader->Flag <= (unsigned int)Flag_NoCompress11
			|| FileHeader->Flag == Flag_NoCompress12
			|| FileHeader->Flag == Flag5 )
		{
			return TRUE;
		}
	}

	Flag = FileHeader->Flag & unk_0xFFFFFF;
	if ( Flag == Flag6_ 
		|| Flag == Flag7 
		|| Flag == Flag8 )
	{
		result = TRUE;
	}

	return result;
}

BOOLEAN 
FsFileHeaderVerify(
				   PFS_POLYCE_FILE_HEAD FileHeader, 
				   PCHAR HeaderFlag
				   )
{
	return (strncmp(FileHeader->_18Name, HeaderFlag, strlen(HeaderFlag)) == 0);
}

BOOLEAN
FsReadDecryptHeader(
					PUCHAR Buffer, 
					PULONG DataLengthPointer, 
					PUCHAR PolicyPwd,
					PUCHAR KeyBox
					)
{
	// 参数：
	// a1-解密的数据内容
	// a2-解密的数据长度
	// a3-策略密码

	PFS_ENCRYPT_FILE_HEAD InputdataBuffer;
	PULONG InputDataLengthPointer;
	BOOLEAN flagLarger512 = FALSE;
	PVOID DecryptDataBuffer;
	PVOID EncryptDataBuffer;
	DWORD EncryptDataSize;
	ULONG DataLength;
	BOOLEAN ReturnStatus = FALSE; 

	DWORD v8_EncryptDataOffset;
	WORD v10_EncryptDataOffset; 
	ULONG v9_InputDataLength; 
	DWORD v15_InputDataLength;
	size_t v11_EncryptDataLength;
	DWORD v17_EncryptDataLength;
	NTSTATUS Result;
	PVOID v18_pEncryptData;
	ULONG v19_InputDataLength; 
	PULONG v22_DeCompressDataLength;
	int v25; 
	int v26; 
	PUCHAR v29; 
	int EncryptDataLength; 

	InputdataBuffer = (FS_ENCRYPT_FILE_HEAD *)Buffer;
	InputDataLengthPointer = DataLengthPointer;

	if ( *InputDataLengthPointer >= FS_FILE_DATA_HEADER_SIZE )
	{
		flagLarger512 = TRUE;
	}
	else
	{
		if ( *InputDataLengthPointer < FS_FILE_DATA_MINSIZE
			|| InputdataBuffer->Flag  <= Flag_standardEncrypt
			&& InputdataBuffer->EncryptDataOffset + InputdataBuffer->EncryptDataLength > (WORD)*DataLengthPointer )
		{
			return FALSE;
		}
	}

	if ( !FsFakeHeaderVerify(InputdataBuffer) )
	{
		// 验证加密头部失败，返回0
		return FALSE;
	}

	if ( *InputDataLengthPointer > FS_FILE_DATA_HEADER_SIZE )
	{
		// 数据长度大于200H时，将其置为200H
		*InputDataLengthPointer = FS_FILE_DATA_HEADER_SIZE;
	}

	do
	{
		//DecryptDataBuffer = ExAllocateFromNPagedLookasideList(&g_NBuffer440F83EBBB8);
		DecryptDataBuffer = ExAllocatePoolWithTag(NonPagedPool,1024, 'XXXX');
		//NdisAllocateMemoryWithTag(&DecryptDataBuffer, 2048, 'xxxx');
	}while ( !DecryptDataBuffer );

	do
	{
		//Buffer = ExAllocateFromNPagedLookasideList(&g_NBuffer440F83EBBB8);
		Buffer = ExAllocatePoolWithTag(NonPagedPool,1024, 'XXXX');
		//NdisAllocateMemoryWithTag(&Buffer, 2048, 'xxxx');
	}while ( !Buffer );

	//处理秘钥盒子
	FsKeyProcess(KeyBox, PolicyPwd, 16);

	switch(InputdataBuffer->Flag)
	{
	case Flag1:
	case Flag_standardEncrypt:
		EncryptDataSize = InputdataBuffer->EncryptDataLength;

		if ( InputdataBuffer->EncryptDataOffset > *InputDataLengthPointer
			|| EncryptDataSize > *InputDataLengthPointer - 8 )
		{
			// 加密头部的标识数据偏移大于解密数据长度，或者
			// 加密头部的标识数据大小大于解密数据长度减去8
			// 不进行解密，返回0
			// 
			// 数据的长度减去压缩数据的长度必须大于8，
			// 否则不压缩，压不出8字节空间，不操作了
			break;
		}

		EncryptDataLength = InputdataBuffer->EncryptDataLength;
		if ( !EncryptDataSize )
		{
			// 标识数据大小为0时，将标识数据大小置为：解密数据大小-标识数据偏移
			EncryptDataLength = *InputDataLengthPointer - InputdataBuffer->EncryptDataOffset;
		}

		EncryptDataBuffer = (PCHAR)InputdataBuffer + InputdataBuffer->EncryptDataOffset;
		memcpy(DecryptDataBuffer, EncryptDataBuffer, EncryptDataLength);
		if ( InputdataBuffer->Flag == Flag_standardEncrypt )
		{
			// 参数：标识数据内容，标识数据大小，策略密码，密码数据缓冲
			// 先解密标识数据
			FsEnDecryptData((PCHAR)DecryptDataBuffer, EncryptDataLength, (PCHAR)PolicyPwd, KeyBox);
		}
		else
		{
			DataLength = 0;
			if ( EncryptDataLength )
			{
				do
				{                                                     
					// 简单异或加密
					*((_BYTE *)DecryptDataBuffer +DataLength) ^= (_BYTE)DataLength;
					++DataLength;
				}while ( DataLength < (ULONG)EncryptDataLength );
			}
		}

		DataLength = FS_CACHE_HEADERDATA_SIZE;                                    
		// /////////////////////////////////////
		// 到这里，已经解密的文件头部加密的数据
		// 
		// 
		// 一种标准加密，一种异或加密
		// 
		// 接下来，解压缩后覆盖那个文件头即可
		// 
		if ( FsDataDecompress62(DecryptDataBuffer, EncryptDataLength, (PVOID)Buffer, &DataLength) == 0)
		{                                                         
			// 参数：解密后数据，解密后数据长度，存放解压数据缓存，大小
			// 将解密后的数据，解压为原始数据，长度512字节
			if ( DataLength <= *InputDataLengthPointer )
			{
				// 将解压得到的512字节数据，复制到原数据缓冲区中，覆盖长度为512字节的文件加密头部
				memcpy((CHAR*)InputdataBuffer,(CHAR*) Buffer, DataLength);
				*InputDataLengthPointer = DataLength;
				ReturnStatus = TRUE;
				break;
			}
		}
		else
		{
			if ( InputdataBuffer->Flag == Flag_standardEncrypt )
			{
				memcpy((CHAR *)DecryptDataBuffer, (CHAR*)EncryptDataBuffer, EncryptDataLength);
				DataLength = 0;
				if ( EncryptDataLength )
				{
					do
					{
						*( (_BYTE *)DecryptDataBuffer+DataLength) ^= (_BYTE)DataLength;
						++DataLength;
					}
					while ( DataLength < (ULONG)EncryptDataLength );
				}

				// 通用解压失败，单独在用特殊解压试一次
				Result = FsDataDecompress6364(
					(BYTE *)DecryptDataBuffer, 
					EncryptDataLength, 
					Buffer, 
					(DWORD *)&DataLength
					);

				if ( !Result )
				{
					break;
				}

				if ( DataLength == (*InputDataLengthPointer + 1) )
				{
					--DataLength;
				}

				if ( (unsigned int)DataLength > *InputDataLengthPointer )
				{
					break;
				}

				// 将解压得到的512字节数据，复制到原数据缓冲区中，覆盖长度为512字节的文件加密头部
				memcpy((PCHAR)InputdataBuffer, (PCHAR)Buffer, DataLength);
				*InputDataLengthPointer = DataLength;
				ReturnStatus = TRUE;
				break;
			}
		}

		break;

	case Flag_HaveOtherFlag:
	case Flag9_Offset4:
		v15_InputDataLength = *InputDataLengthPointer;
		if (  InputdataBuffer->Flag == Flag_HaveOtherFlag )
		{                     

			// 文件头为0x65231463
			if ( InputdataBuffer->EncryptDataOffset > v15_InputDataLength )
			{
				break;
			}

			if ( InputdataBuffer->EncryptDataOffset < 8 )
			{
				v17_EncryptDataLength = v15_InputDataLength - InputdataBuffer->EncryptDataOffset;
			}
			else
			{
				v17_EncryptDataLength = InputdataBuffer->EncryptDataLength;
			}

			v18_pEncryptData = (PCHAR)InputdataBuffer + InputdataBuffer->EncryptDataOffset;
		}
		else
		{                                                         
			// 文件头为0x65231464
			v17_EncryptDataLength = v15_InputDataLength - 4;
			v18_pEncryptData = (char *)InputdataBuffer + 4;      // 偏移竟然是4,4个字节的Flag，剩下都是数据了
		}

		if ( v17_EncryptDataLength <= v15_InputDataLength )
		{
			memcpy((PCHAR)DecryptDataBuffer, (PCHAR)v18_pEncryptData, v17_EncryptDataLength);
			FsEnDecryptData(
				(PCHAR)DecryptDataBuffer, 
				v17_EncryptDataLength, 
				(PCHAR)PolicyPwd, KeyBox);

			// 注意：先用通用加压，再用特殊解压
			// 
			// 那么，压缩时，先用特殊压缩，再用通用压缩
			if ( FsDataDecompress6364((BYTE *)DecryptDataBuffer, v17_EncryptDataLength, Buffer, (DWORD *)&DataLength) )
			{
				v19_InputDataLength = *InputDataLengthPointer;
				v22_DeCompressDataLength = (PULONG)DataLength;
				if ( DataLength == (ULONG)(*InputDataLengthPointer + 1) )
				{
					v22_DeCompressDataLength = (PULONG)((char *)DataLength-- - 1);
				}

				if ( v22_DeCompressDataLength <= (PULONG)v19_InputDataLength )
				{
					// 解压缩后文件比指定长度小？？
					memcpy((PCHAR)InputdataBuffer, (PCHAR)Buffer, (size_t)v22_DeCompressDataLength);
					*InputDataLengthPointer = (ULONG)v22_DeCompressDataLength;
					ReturnStatus = 1;
				}
			}
		}

		break;


	case Flag_NoCompress11:
		FsEnDecryptData(
			(PCHAR)&InputdataBuffer->EncryptDataOffset,
			*InputDataLengthPointer - 4,
			(PCHAR)PolicyPwd,
			KeyBox);
		InputdataBuffer->Flag = Flag11;
		ReturnStatus = TRUE;

		break;

	case Flag_NoCompress12:
		FsEnDecryptData(
			(PCHAR)&InputdataBuffer->EncryptDataOffset,
			*InputDataLengthPointer - 4,
			(PCHAR)PolicyPwd,
			KeyBox);
		InputdataBuffer->Flag = Flag12_;
		ReturnStatus = TRUE;

		break;

	case Flag5:
		v10_EncryptDataOffset = InputdataBuffer->EncryptDataOffset;
		v9_InputDataLength = *InputDataLengthPointer;
		v8_EncryptDataOffset = (unsigned __int16)v10_EncryptDataOffset;
		if ( (unsigned __int16)v10_EncryptDataOffset > *InputDataLengthPointer
			|| ((unsigned __int16)v10_EncryptDataOffset < 8u ? (v11_EncryptDataLength = v9_InputDataLength
			- (unsigned __int16)v10_EncryptDataOffset) : (v11_EncryptDataLength = InputdataBuffer->EncryptDataLength), EncryptDataLength = v11_EncryptDataLength, v11_EncryptDataLength > v9_InputDataLength) )
		{
			break;
		}

		memcpy(
			(void *)DecryptDataBuffer,
			(char *)InputdataBuffer + v8_EncryptDataOffset,
			v11_EncryptDataLength);

		FsEnDecryptData(
			(PCHAR)DecryptDataBuffer, 
			EncryptDataLength, 
			(PCHAR)PolicyPwd, 
			KeyBox
			);

		// todo ??
		Result = FsDataDecompress6364(
			DecryptDataBuffer, 
			EncryptDataLength, 
			(PVOID)Buffer, 
			&DataLength
			);

		if ( !Result )
		{
			break;
		}

		if ( DataLength == (*InputDataLengthPointer + 1) )
		{
			--DataLength;
		}

		if ( (unsigned int)DataLength > *InputDataLengthPointer )
		{
			break;
		}

		// 将解压得到的512字节数据，复制到原数据缓冲区中，覆盖长度为512字节的文件加密头部
		memcpy((PCHAR)InputdataBuffer, (PCHAR)Buffer, DataLength);
		*InputDataLengthPointer = (ULONG)DataLength;
		ReturnStatus = TRUE;
		break;

	default:

		switch (InputdataBuffer->Flag & unk_0xFFFFFF)
		{
		case Flag6_:
			FsEnDecryptData(
				(PCHAR)&InputdataBuffer->EncryptDataOffset,
				*InputDataLengthPointer - 4,
				(PCHAR)PolicyPwd,
				KeyBox);

			InputdataBuffer->Flag = InputdataBuffer->Flag & unk_0xFF535743 | unk_0x535743;
			ReturnStatus = TRUE;
			break;
		case Flag7:
			FsEnDecryptData(
				(PCHAR)&InputdataBuffer->EncryptDataOffset,
				*InputDataLengthPointer - 4,
				(PCHAR)PolicyPwd,
				KeyBox);

			InputdataBuffer->Flag = InputdataBuffer->Flag & unk_0xFF535746 | unk_0x535746;
			ReturnStatus = TRUE;
			break;
		case Flag8:
			FsEnDecryptData(
				(PCHAR)&InputdataBuffer->EncryptDataOffset,
				*InputDataLengthPointer - 4,
				(PCHAR)PolicyPwd,
				KeyBox);

			InputdataBuffer->Flag =  InputdataBuffer->Flag & unk_0xFF088B1F | unk_0x88B1F;
			ReturnStatus = TRUE;
			break;
		}

	}

	ExFreePoolWithTag(Buffer, 'XXXX');
	ExFreePoolWithTag(DecryptDataBuffer, 'XXXX');
	//NdisFreeMemory(Buffer, 2048, 'xxxx');
	//NdisFreeMemory(DecryptDataBuffer, 2048, 'xxxx');

	return ReturnStatus;
}


signed int __stdcall FsFakeHeaderMake_sub_80A27142(void *Header, CHAR* DataSet)
{
	INT Counter;
	FS_ENCRYPT_FILE_HEAD_TWO *TempHeader = Header;
	signed int result;
	char TempChar;

	memset(TempHeader, 0, FS_FILE_DATA_HEADER_SIZE);

	TempHeader->_9_CheckSum = 0;
	TempHeader->_0_Magic = Flag_standardEncrypt;
	strcpy(TempHeader->_C_Type, "SunInfo");
	memcpy(TempHeader->_18_SubType, DataSet, strlen(DataSet));

	TempHeader->_A_Version = FS_CACHE_HEADERDATA_SIZE/2;
	TempHeader->_4_EncryptDataOffset = FS_CACHE_HEADERDATA_SIZE;
	result = FS_CACHE_HEADERDATA_SIZE;

	return result;
}

//通过运算，生成新密码
int __stdcall FsPasswordProcess_sub_80A271DC(CHAR *PassWd, CHAR *NewpassWd)
{
	int result;
	LONGLONG PassWdBuffer;
	ULONG *NewpassWdPointer;
	signed int Counter;
	LARGE_INTEGER v15;
	LARGE_INTEGER v11;
	LARGE_INTEGER Result;
	LARGE_INTEGER temp;

	// 参数：
	// a1:密码
	// a2:新密码缓冲区

	NewpassWdPointer = (ULONG*)NewpassWd;
	Counter = 8; 
	do
	{
		PassWdBuffer = *(_WORD *)PassWd;
		PassWd += 2;

		v15.QuadPart = 2701;

		temp.LowPart = (ULONG)PassWdBuffer;
		temp.HighPart = (ULONG)(PassWdBuffer >> 32);

		Result.QuadPart = 1;
		do
		{
			if ( v15.LowPart & 1 )
			{
				Result.QuadPart = (Result.QuadPart * temp.QuadPart) % 0x1EB65F4F;
			}

			v11.QuadPart = (temp.QuadPart * temp.QuadPart) % 0x1EB65F4F;
			v11.LowPart = (v15.HighPart >> 1) | (unsigned int)(v15.QuadPart >> 1);

			temp.QuadPart = v11.QuadPart;

			v15.QuadPart >>= 1;
		}while ( v11.LowPart );

		*NewpassWdPointer = Result.LowPart;
		NewpassWdPointer++;

		result = Result.LowPart;
	}while ( !(Counter-- == 1) );

	return result;
}

BOOLEAN
FsWriteEncryptHeader(
					 IN PUCHAR Data, 
					 IN ULONG DataLength, 
					 IN PUCHAR Password, 
					 IN PUCHAR PwdBuffer
					 )
{
	ULONG FileHeaderLength;
	PFS_ENCRYPT_FILE_HEAD_TWO FileHeaderBuffer;
	PFS_ENCRYPT_FILE_HEAD_TWO FileHeaderDataPointer;
	void *CompressDataBuffer;
	DWORD CompressDataLength;
	unsigned int EncryptDataOffset;
	int TempFlag;
	signed int nReturn;

	nReturn = 0;
	FileHeaderLength = DataLength;
	if ( FileHeaderLength < FS_FILE_DATA_MINSIZE )
	{// 处理数据小于10H，返回0
		return 0;
	}

	do
	{
		//FileHeaderBuffer = (PFS_ENCRYPT_FILE_HEAD_TWO)ExAllocateFromNPagedLookasideList(&g_NBuffer440F83EBBB8);
		FileHeaderBuffer = ExAllocatePoolWithTag(NonPagedPool, 1024, 'XXXX');
		//NdisAllocateMemoryWithTag(&FileHeaderBuffer, 2048, 'xxxx');
	}while ( !FileHeaderBuffer );

	do
	{
		//CompressDataBuffer = ExAllocateFromNPagedLookasideList(&g_NBuffer440F83EBBB8);
		CompressDataBuffer = ExAllocatePoolWithTag(NonPagedPool, 1024, 'XXXX');
		//NdisAllocateMemoryWithTag(&CompressDataBuffer, 2048, 'xxxx');
	}while ( !CompressDataBuffer );

	//初始化秘钥盒子
	FsKeyProcess(PwdBuffer, Password, 16);

	// 加密头赋值
	FsFakeHeaderMake_sub_80A27142((void*)FileHeaderBuffer, FS_NAME_LOCK);  
	// 密码处理后，存放到加密头部
	//FsPasswordProcess_sub_80A271DC(Password, FileHeaderBuffer->_28_field_PassWdProcessed);
	// 处理后的密码，再次处理，并存放在原位置
	//FsEnDecryptData(FileHeaderBuffer->_28_field_PassWdProcessed, FS_PWD_SIZE, FileHeaderBuffer->_38_field_PassWd, PwdBuffer);

	FileHeaderDataPointer = (PFS_ENCRYPT_FILE_HEAD_TWO)Data;

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	FsDataCompress62(
		(BYTE *)FileHeaderDataPointer, 
		FileHeaderLength, 
		(BYTE *)CompressDataBuffer, 
		(DWORD *)&CompressDataLength);
	if (CompressDataLength <= FileHeaderLength - 8 )
	{
		//62压缩成功，并且有多余8字节的空间
		FileHeaderBuffer->_4_EncryptDataOffset = (WORD)(FileHeaderLength - CompressDataLength); // Header+4 = 压缩前数据长度-压缩后数据长度
		FileHeaderBuffer->_6_EncryptDataLength = (WORD)CompressDataLength;                   // Header+6 = 压缩后数据长度

		// 将压缩后的数据，使用策略密码加密
		FsEnDecryptData((PCHAR)CompressDataBuffer, CompressDataLength, Password, PwdBuffer);
		memcpy((PCHAR)FileHeaderDataPointer, (PCHAR)FileHeaderBuffer, FileHeaderBuffer->_4_EncryptDataOffset);//可能复制不全，具有伸缩性
		memcpy((PCHAR)FileHeaderDataPointer + FileHeaderBuffer->_4_EncryptDataOffset, CompressDataBuffer, CompressDataLength);

		//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
		//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
		ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
		ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
		//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
		//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
		return 1;
	}

	//
	//到这里，可能62压缩失败，或者压缩空间小于8
	//
	if ( FsDataCompress6364(
		(PVOID)FileHeaderDataPointer, 
		(ULONG)FileHeaderLength, 
		(PVOID)CompressDataBuffer, 
		(PULONG)&CompressDataLength)
		&& (FileHeaderLength >= FS_FILE_DATA_HEADER_SIZE || CompressDataLength <= FileHeaderLength - 8)
		&& CompressDataLength <= FileHeaderLength - 4 )
	{                                                             
		// 如果文件头长度大于等于512，必须压出8字节空间
		// 否则必须压出4字节空间
		FsEnDecryptData((PCHAR)CompressDataBuffer, CompressDataLength, Password, PwdBuffer);
		if ( CompressDataLength > FileHeaderLength - 6 )
		{
			//只压缩出4字节的情况，
			FileHeaderBuffer->_0_Magic = Flag9_Offset4;
			EncryptDataOffset = 4;
			FileHeaderBuffer->_4_EncryptDataOffset = (WORD)EncryptDataOffset;
			FileHeaderBuffer->_6_EncryptDataLength = (WORD)CompressDataLength;
			memcpy((CHAR*)FileHeaderDataPointer, (CHAR*)FileHeaderBuffer, EncryptDataOffset);
			memcpy((char *)FileHeaderDataPointer + EncryptDataOffset, CompressDataBuffer, CompressDataLength);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
			ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
			ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
			//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
			//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
			return 1;
		}

		FileHeaderBuffer->_0_Magic = Flag_HaveOtherFlag;//与只有四个字节的，这里不同
		EncryptDataOffset = FileHeaderLength - CompressDataLength;
		FileHeaderBuffer->_4_EncryptDataOffset = (WORD)EncryptDataOffset;
		FileHeaderBuffer->_6_EncryptDataLength = (WORD)CompressDataLength;
		memcpy((CHAR*)FileHeaderDataPointer, (CHAR*)FileHeaderBuffer, EncryptDataOffset);
		memcpy((char *)FileHeaderDataPointer + EncryptDataOffset, CompressDataBuffer, CompressDataLength);
		//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
		//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
		ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
		ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
		//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
		//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
		return 1;
	}

	//
	//到这里，6364压缩失败，或者压缩空间小于4，
	//

	if (FileHeaderLength == FS_CACHE_HEADERDATA_SIZE)
	{
		if (FileHeaderDataPointer->_0_Magic == Flag11)
		{
			FileHeaderDataPointer->_0_Magic = Flag_NoCompress11;
			FsEnDecryptData((PCHAR)&FileHeaderDataPointer->_4_EncryptDataOffset, 508, Password, PwdBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
			ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
			ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
			//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
			//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
			return 1;
		}


		if ( FileHeaderDataPointer->_0_Magic == Flag12_ )
		{
			FileHeaderDataPointer->_0_Magic = Flag_NoCompress12;
			FsEnDecryptData((PCHAR)&FileHeaderDataPointer->_4_EncryptDataOffset, 508, Password, PwdBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
			ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
			ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
			//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
			//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
			return 1;
		}

		TempFlag = FileHeaderDataPointer->_0_Magic & unk_0xFFFFFF;
		if ( TempFlag == unk_0x535743 )
		{
			FileHeaderDataPointer->_0_Magic = FileHeaderDataPointer->_0_Magic & unk_0xFF231466 | Flag6_;
			FsEnDecryptData((PCHAR)&FileHeaderDataPointer->_4_EncryptDataOffset, 508, Password, PwdBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
			ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
			ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
			//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
			//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
			return 1;
		}

		if ( TempFlag == unk_0x535746 )
		{
			FileHeaderDataPointer->_0_Magic = FileHeaderDataPointer->_0_Magic & unk_0xFF231467 | Flag7;
			FsEnDecryptData((PCHAR)&FileHeaderDataPointer->_4_EncryptDataOffset, 508, Password, PwdBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
			ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
			ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
			//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
			//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
			return 1;
		}

		if ( TempFlag == unk_0x88B1F )
		{
			FileHeaderDataPointer->_0_Magic = FileHeaderDataPointer->_0_Magic & unk_0xFF231468 | Flag8;
			FsEnDecryptData((PCHAR)&FileHeaderDataPointer->_4_EncryptDataOffset, 508, Password, PwdBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
			ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
			ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
			//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
			//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
			return 1;
		}

		if ( !FsDataCompress6364((BYTE*)FileHeaderDataPointer, (DWORD)FileHeaderLength, (BYTE*)CompressDataBuffer, &CompressDataLength) 
			|| CompressDataLength > FileHeaderLength - 6 )
		{
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
			ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
			ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
			//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
			//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
			return nReturn;
		}

		FsEnDecryptData((PCHAR)CompressDataBuffer, CompressDataLength, Password, PwdBuffer);
		FileHeaderBuffer->_0_Magic = Flag5;
		EncryptDataOffset = FileHeaderLength - CompressDataLength;
		FileHeaderBuffer->_4_EncryptDataOffset = (WORD)EncryptDataOffset;
		FileHeaderBuffer->_6_EncryptDataLength = (WORD)CompressDataLength;
		memcpy((CHAR*)FileHeaderDataPointer, (CHAR*)FileHeaderBuffer, EncryptDataOffset);
		memcpy((char *)FileHeaderDataPointer + EncryptDataOffset, CompressDataBuffer, CompressDataLength);
		//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
		//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
		ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
		ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
		//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
		//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
		return 1;
	}
	else
	{
		// todo ??
		if ( !FsDataCompress6364(
			(PVOID)FileHeaderDataPointer, 
			(ULONG)FileHeaderLength, 
			(PVOID)CompressDataBuffer, 
			(PULONG)&CompressDataLength) ||
			CompressDataLength+4 > FileHeaderLength)
		{//最少压缩一个字节
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
			//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
			ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
			ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
			//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
			//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
			return nReturn;
		}

		FsEnDecryptData((PCHAR)CompressDataBuffer, CompressDataLength, Password, PwdBuffer);

		FileHeaderBuffer->_0_Magic = Flag5;
		EncryptDataOffset = FileHeaderLength - CompressDataLength;
		FileHeaderBuffer->_4_EncryptDataOffset = (WORD)EncryptDataOffset;
		FileHeaderBuffer->_6_EncryptDataLength = (WORD)CompressDataLength;
		memcpy((CHAR*)FileHeaderDataPointer, (CHAR*)FileHeaderBuffer, EncryptDataOffset);
		memcpy((char *)FileHeaderDataPointer + EncryptDataOffset, CompressDataBuffer, CompressDataLength);
		//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, CompressDataBuffer);
		//ExFreeToNPagedLookasideList(&g_NBuffer440F83EBBB8, FileHeaderBuffer);
		ExFreePoolWithTag(CompressDataBuffer, 'XXXX');
		ExFreePoolWithTag(FileHeaderBuffer, 'XXXX');
		//NdisFreeMemory(CompressDataBuffer, 2048, 'xxxx');
		//NdisFreeMemory(FileHeaderBuffer, 2048, 'xxxx');
		return 1;
	}

	return nReturn;
}

BOOLEAN
FsWriteBlockEncrypt(
					 IN PUCHAR Data, 
					 IN ULONG DataLength, 
					 IN ULONGLONG Offset,
					 IN PUCHAR Password, 
					 IN PUCHAR PwdBuffer
					 )
{
	BOOLEAN bRnt = FALSE;
	if (Offset == 0)
	{
		if (DataLength <= 512)
		{
			bRnt = FsWriteEncryptHeader(
				Data, 
				DataLength, 
				Password, 
				PwdBuffer
				);
			//ASSERT(bRnt == TRUE);
			if (bRnt == FALSE)
			{
				KdPrint(("---readfile encrypt error offset=%d length=%d\n", Offset, DataLength));
			}
			
		}
		else
		{
			bRnt = FsWriteEncryptHeader(
				Data, 
				512, 
				Password, 
				PwdBuffer
				);
			if (bRnt)
			{
				if (FsEnDecryptDataBlockMore(Data+512,
					DataLength - 512,
					Password,
					PwdBuffer))
				{
					bRnt = FALSE;
				}
				else
				{
					bRnt = TRUE;
				}
			}
		}
		
	}
	else
	{
		//ASSERT(Offset % 512 == 0);
		if (Offset % 512 != 0)
		{
			KdPrint(("----error read file offset%512 != 0\n"));
		}
		if (FsEnDecryptDataBlockMore(Data,
			DataLength,
			Password,
			PwdBuffer))
		{
			bRnt = FALSE;
		}
		else
		{
			bRnt = TRUE;
		}
	}

	return bRnt;
}


BOOLEAN
FsReadBlockDecrypt(
					IN PUCHAR Data, 
					IN ULONG DataLength, 
					IN ULONGLONG Offset,
					IN PUCHAR Password, 
					IN PUCHAR PwdBuffer
					)
{
	BOOLEAN bRnt = FALSE;
	ULONG DecryptLength;
	if (Offset == 0)
	{
		if (DataLength <= 512)
		{
			DecryptLength = DataLength;
			bRnt = FsReadDecryptHeader(
				Data, 
				&DecryptLength, 
				Password, 
				PwdBuffer
				);
			//ASSERT(bRnt == TRUE);
			if (bRnt == FALSE)
			{
				KdPrint(("---writefile decrypt error offset=%d length=%d\n", Offset, DataLength));
			}
			
		}
		else
		{
			DecryptLength = 512;
			bRnt = FsReadDecryptHeader(
				Data, 
				&DecryptLength, 
				Password, 
				PwdBuffer
				);
			if (bRnt)
			{
				if (FsEnDecryptDataBlockMore(Data+512,
					DataLength - 512,
					Password,
					PwdBuffer))
				{
					bRnt = FALSE;
				}
				else
				{
					bRnt = TRUE;
				}
			}
		}

	}
	else
	{
		//ASSERT(Offset % 512 == 0);
		if (Offset%512 != 0)
		{
			KdPrint(("----error write file offset%512 != 0\n"));
		}
		
		if (FsEnDecryptDataBlockMore(Data,
			DataLength,
			Password,
			PwdBuffer))
		{
			bRnt = FALSE;
		}
		else
		{
			bRnt = TRUE;
		}
	}

	return bRnt;
}

//Offset必须大于等于512，不适用于文件头
BOOLEAN
FsRandomEndecrypt(
				   IN PUCHAR Data, 
				   IN ULONG DataLength, 
				   IN ULONGLONG Offset,
				   IN PUCHAR Password, 
				   IN PUCHAR PwdBuffer
				   )
{
	BOOLEAN bRnt = FALSE;
	ULONG HeaderLength = 0;
	UINT remainder = Offset % 512;
	UCHAR Buffer[512] = {0};

	ASSERT(Offset >= 512);

	if (remainder > 0)
	{
		HeaderLength = DataLength<(512-remainder)?DataLength:(512-remainder);
		ASSERT(HeaderLength < 512);
		RtlCopyMemory(Buffer+remainder, Data, HeaderLength);
		FsEnDecryptDataBlockMore(Buffer,
			512,
			Password,
			PwdBuffer);
		RtlCopyMemory(Data, Buffer+remainder, HeaderLength);

		if (DataLength > HeaderLength)
		{
			FsEnDecryptDataBlockMore(Data + HeaderLength,
				DataLength - HeaderLength,
				Password,
				PwdBuffer);
		}
	}
	else
	{
		FsEnDecryptDataBlockMore(Data,
			DataLength,
			Password,
			PwdBuffer);
	}

	return TRUE;
}