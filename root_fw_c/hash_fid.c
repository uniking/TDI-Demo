#include "hash_fid.h"

FID_NET_FILE g_FidHash[HASH_BUFFER_LENGTH] = {0};

BOOLEAN hfInsertFid(PFID_NET_FILE FidNode)
{
	BOOLEAN bRtn = FALSE;
	int index = FidNode->Fid %HASH_MAGIC;

	while(index < HASH_BUFFER_LENGTH)
	{
		if (!g_FidHash[index].valide)
		{
			g_FidHash[index].valide = TRUE;
			g_FidHash[index].Fid = FidNode->Fid;
			g_FidHash[index].FileName.MaximumLength = FidNode->FileName.MaximumLength;
			g_FidHash[index].FileName.Length = FidNode->FileName.Length;
			g_FidHash[index].FileName.Buffer = FidNode->FileName.Buffer;
			//g_FidHash[index].isEncryptFile = TRUE;
			g_FidHash[index].read.isFirstRead = TRUE; //ppt右键新建，解密偏移0xfff，导致乱码，阅读文件，说明不是新建的

			bRtn = TRUE;
			break;
		}
		index++;
	}

	return bRtn;
}

PFID_NET_FILE hfFindFid(USHORT Fid)
{
	PFID_NET_FILE pRtn = NULL;
	int index = Fid %HASH_MAGIC;

	while(index < HASH_BUFFER_LENGTH)
	{
		if (g_FidHash[index].valide &&
			g_FidHash[index].Fid == Fid)
		{
			pRtn = &g_FidHash[index];
			break;
		}
		index++;
	}

	return pRtn;
}

BOOLEAN hfDeleteFid(USHORT Fid)
{
	BOOLEAN bRtn = FALSE;
	int index = Fid %HASH_MAGIC;

	while(index < HASH_BUFFER_LENGTH)
	{
		if (g_FidHash[index].Fid == Fid)
		{
			g_FidHash[index].valide = FALSE;
			//KdPrint(("delete fid=%x\n", g_FidHash[index].Fid));
			//删除文件名
			if (g_FidHash[index].FileName.Buffer)
			{
				ExFreePoolWithTag(g_FidHash[index].FileName.Buffer, 'xxxx');
				g_FidHash[index].FileName.Buffer = NULL;
			}
			bRtn = TRUE;
			break;
		}
		index++;
	}

	return bRtn;
}