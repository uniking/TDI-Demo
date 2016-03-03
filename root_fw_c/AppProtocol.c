#include "AppProtocol.h"
#include "TdiBase.h"
#include "NetBIOS.h"
#include "Http.h"
#include "ObjListManager.h"
#include <ntddk.h>
#include <ntdef.h>

BOOLEAN g_ProtocolThreadStart = FALSE;
LIST_ENTRY g_DataList = {0};
KSPIN_LOCK  g_DataListSpinLock;
KMUTEX g_SMBKMutex;
KMUTEX g_HttpKMutex;

VOID WaitMicroSecond(ULONG ulMircoSecond)
{
	KEVENT kEvent;
	LARGE_INTEGER timeout;

	KeInitializeEvent(&kEvent,SynchronizationEvent,FALSE);

	timeout = RtlConvertLongToLargeInteger(-10*ulMircoSecond);

	KeWaitForSingleObject(&kEvent,
		Executive,
		KernelMode,
		FALSE,
		&timeout);
}

VOID AppSendData(PDATA_INFO pDataNode)
{
	switch(pDataNode->RemoteIpPort)
	{
	case 139:
	case 445:
		pDataNode->uStatus = netbios_send_ndis_packet(&pDataNode->ConnectItem->AppContext);
		break;
	default:
		;
	}
}

VOID AppReceiveData(PDATA_INFO pDataNode)
{
	switch(pDataNode->RemoteIpPort)
	{
	case 139:
	case 445:
		pDataNode->uStatus = netbios_receive_ndis_packet(&pDataNode->ConnectItem->AppContext);
		break;
	default:
		;
	}
}

//本线程导致串行处理
VOID ProtocolThread(
				 PVOID StartContext
				 )
{
	KIRQL      OldIrql;
	PLIST_ENTRY pTemp;
	PDATA_INFO pDataNode = NULL;

	while(g_ProtocolThreadStart)
	{
		KeAcquireSpinLock(
			&g_DataListSpinLock,
			&OldIrql
			);

		while(!IsListEmpty(&g_DataList))
		{
			pTemp = RemoveHeadList(&g_DataList);
			pDataNode = CONTAINING_RECORD(pTemp,DATA_INFO,node);
			if (pDataNode->isSendData)
			{
				AppSendData(pDataNode);
			}
			else
			{
				AppReceiveData(pDataNode);
			}

			//激活等待的线程
			KeSetEvent (&pDataNode->WaitEvent, IO_NO_INCREMENT, FALSE);
		}

		KeReleaseSpinLock(
			&g_DataListSpinLock,
			OldIrql
			);

		WaitMicroSecond(100);
	}
}

VOID AppInsertData(PLIST_ENTRY entry)
{
	KIRQL      OldIrql;

	KeAcquireSpinLock(
		&g_DataListSpinLock,
		&OldIrql
		);

	InsertTailList(&g_DataList, entry);

	KeReleaseSpinLock(
		&g_DataListSpinLock,
		OldIrql
		);
}



PDATA_INFO AppFindThreadContext(HANDLE ThreadId, PLIST_ENTRY root)
{
	PDATA_INFO pRnt = NULL;
	PDATA_INFO pDataNode = NULL;
	PLIST_ENTRY pTemp = NULL;
	INT i = 0;

	pTemp = root->Flink;

	for (;pTemp != root;pTemp = pTemp->Flink)
	{
		pDataNode = CONTAINING_RECORD(pTemp,DATA_INFO,node);
		if (pDataNode->ThreadId == ThreadId)
		{
			pRnt = pDataNode;
			break;
		}
	}

	return pRnt;
}

UINT AppSend(PUCHAR buffer, 
			 PULONG length, 
			 PFILE_OBJECT ConnectFileObject)
{
	UINT iStatus = 0;
	ULONG LocalIpAddress;
	USHORT LocalIpPort;
	ULONG RemoteIpAddress;
	USHORT RemoteIpPort;
	PETHREAD ThreadId = NULL;
	PCONNECTION_ITEM ConnectItem;
	PDATA_INFO pDataNode = NULL;

	ConnectItem = FindConnectItemByFileObj(ConnectFileObject);
	if (ConnectItem == NULL ||
		ConnectItem->AddressItemPtr == NULL)
	{
		return iStatus;
	}

	//if (!TdiGetIpAddressAndPortByTAAddress(
	//	(PTA_ADDRESS)ConnectItem->AddressItemPtr->LocalAddress,
	//	&LocalIpAddress,
	//	&LocalIpPort
	//	))
	//{
	//	return iStatus;
	//}

	if (!TdiGetIpAddressAndPortByTAAddress(
		(PTA_ADDRESS)ConnectItem->RemoteAddress,
		&RemoteIpAddress,
		&RemoteIpPort
		))
	{
		return iStatus;
	}

	//根据端口号调用相应协议
	switch(RemoteIpPort)
	{
	case 139:
	case 445:
		ThreadId = PsGetCurrentThread();

		do 
		{
			pDataNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(DATA_INFO), 'xxxx');
		} while (pDataNode == NULL);
		
		RtlZeroMemory(pDataNode, sizeof(DATA_INFO));
		InitializeListHead(&pDataNode->node);
		KeInitializeEvent( &pDataNode->WaitEvent, NotificationEvent, FALSE );
		pDataNode->isSendData = TRUE;
		pDataNode->ThreadId = ThreadId;
		pDataNode->RemoteIpPort = RemoteIpPort;
		pDataNode->ConnectItem = ConnectItem;

		//区域锁串行化
		{
			KeWaitForSingleObject(&g_SMBKMutex,
				Executive, KernelMode, FALSE, NULL);
			ConnectItem->AppContext.sendBuffer.Buffer = buffer;
			ConnectItem->AppContext.sendBuffer.length = *length;
			ConnectItem->AppContext.sendBuffer.MaximumLength = *length;
			ConnectItem->AppContext.sendBuffer.Offset = 0;
			iStatus = netbios_send_ndis_packet(&ConnectItem->AppContext);

			if (FLAG_ON_UINT(iStatus, CHANGE_PACKET_DATA))
			{//修改发送数据的长度
				*length = pDataNode->ConnectItem->AppContext.sendBuffer.length;
				
			}

			KeReleaseMutex(&g_SMBKMutex, FALSE);

			ExFreePoolWithTag(pDataNode, 'xxxx');
		}

		break;

	case 80:
	case 8080:
		ThreadId = PsGetCurrentThread();

		do 
		{
			pDataNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(DATA_INFO), 'xxxx');
		} while (pDataNode == NULL);

		RtlZeroMemory(pDataNode, sizeof(DATA_INFO));
		InitializeListHead(&pDataNode->node);
		KeInitializeEvent( &pDataNode->WaitEvent, NotificationEvent, FALSE );
		pDataNode->isSendData = TRUE;
		pDataNode->ThreadId = ThreadId;
		pDataNode->RemoteIpPort = RemoteIpPort;
		pDataNode->ConnectItem = ConnectItem;

		//区域锁串行化
		{
			KeWaitForSingleObject(&g_HttpKMutex,
				Executive, KernelMode, FALSE, NULL);
			ConnectItem->AppContext.sendBuffer.Buffer = buffer;
			ConnectItem->AppContext.sendBuffer.length = *length;
			ConnectItem->AppContext.sendBuffer.MaximumLength = *length;
			ConnectItem->AppContext.sendBuffer.Offset = 0;
			iStatus = http_send_ndis_packet(&ConnectItem->AppContext);

			if (FLAG_ON_UINT(iStatus, CHANGE_PACKET_DATA))
			{//修改发送数据的长度
				*length = pDataNode->ConnectItem->AppContext.sendBuffer.length;

			}

			KeReleaseMutex(&g_HttpKMutex, FALSE);

			ExFreePoolWithTag(pDataNode, 'xxxx');
		}
		break;
	default:
		break;
	}



	return iStatus;
}

UINT AppReceive(
				PUCHAR buffer, 
				PULONG length, 
				PFILE_OBJECT AddressFileObject)
{
	ULONG LocalIpAddress;
	USHORT LocalIpPort;
	ULONG RemoteIpAddress;
	USHORT RemoteIpPort;
	UINT iStatus = 0;
	PETHREAD ThreadId = NULL;
	PADDRESS_ITEM AddressItem;
	PCONNECTION_ITEM ConnectItem;
	PDATA_INFO pDataNode = NULL;

	AddressItem = FindAddressItemByFileObj(AddressFileObject);

	if (AddressItem == NULL ||
		AddressItem->ConnectItem == NULL)
	{
		return 0;
	}

	ConnectItem = AddressItem->ConnectItem;

	//if (!TdiGetIpAddressAndPortByTAAddress(
	//	(PTA_ADDRESS)AddressItem->LocalAddress,
	//	&LocalIpAddress,
	//	&LocalIpPort
	//	))
	//{
	//	return 0;
	//}

	if (!TdiGetIpAddressAndPortByTAAddress(
		(PTA_ADDRESS)AddressItem->ConnectItem->RemoteAddress,
		&RemoteIpAddress,
		&RemoteIpPort
		))
	{
		return 0;
	}

	

	//根据端口号调用相应协议
	switch(RemoteIpPort)
	{
	case 139:
	case 445:
		ThreadId = PsGetCurrentThread();
		do 
		{
			pDataNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(DATA_INFO), 'xxxx');
		} while (pDataNode == NULL);
		
		RtlZeroMemory(pDataNode, sizeof(DATA_INFO));
		InitializeListHead(&pDataNode->node);
		KeInitializeEvent( &pDataNode->WaitEvent, NotificationEvent, FALSE );
		pDataNode->isSendData = FALSE;
		pDataNode->ThreadId = ThreadId;
		pDataNode->RemoteIpPort = RemoteIpPort;
		pDataNode->ConnectItem = ConnectItem;

		//pDataNode->threadData.receiveBuffer.Buffer = buffer;
		//pDataNode->threadData.receiveBuffer.length = *length;
		//pDataNode->threadData.receiveBuffer.MaximumLength = *length;
		//pDataNode->threadData.receiveBuffer.Offset = 0;
		ConnectItem->AppContext.receiveBuffer.Buffer = buffer;
		ConnectItem->AppContext.receiveBuffer.length = *length;
		ConnectItem->AppContext.receiveBuffer.MaximumLength = *length;
		ConnectItem->AppContext.receiveBuffer.Offset = 0;

		{
			//AppInsertData(&pDataNode->node);
			//KeWaitForSingleObject(&pDataNode->WaitEvent,
			//	Executive, // WaitReason
			//	KernelMode, // must be Kernelmode to prevent the stack getting paged out
			//	FALSE,
			//	NULL // indefinite wait
			//	);
			//iStatus = pDataNode->uStatus;
		}
		{
			iStatus = netbios_receive_ndis_packet(&ConnectItem->AppContext);
		}

		ExFreePoolWithTag(pDataNode, 'xxxx');

		break;

	case 80:
	case 8080:
		ThreadId = PsGetCurrentThread();
		do 
		{
			pDataNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(DATA_INFO), 'xxxx');
		} while (pDataNode == NULL);

		RtlZeroMemory(pDataNode, sizeof(DATA_INFO));
		InitializeListHead(&pDataNode->node);
		KeInitializeEvent( &pDataNode->WaitEvent, NotificationEvent, FALSE );
		pDataNode->isSendData = FALSE;
		pDataNode->ThreadId = ThreadId;
		pDataNode->RemoteIpPort = RemoteIpPort;
		pDataNode->ConnectItem = ConnectItem;

		//pDataNode->threadData.receiveBuffer.Buffer = buffer;
		//pDataNode->threadData.receiveBuffer.length = *length;
		//pDataNode->threadData.receiveBuffer.MaximumLength = *length;
		//pDataNode->threadData.receiveBuffer.Offset = 0;
		ConnectItem->AppContext.receiveBuffer.Buffer = buffer;
		ConnectItem->AppContext.receiveBuffer.length = *length;
		ConnectItem->AppContext.receiveBuffer.MaximumLength = *length;
		ConnectItem->AppContext.receiveBuffer.Offset = 0;

		{
			//AppInsertData(&pDataNode->node);
			//KeWaitForSingleObject(&pDataNode->WaitEvent,
			//	Executive, // WaitReason
			//	KernelMode, // must be Kernelmode to prevent the stack getting paged out
			//	FALSE,
			//	NULL // indefinite wait
			//	);
			//iStatus = pDataNode->uStatus;
		}
		{
			iStatus = http_receive_ndis_packet(&ConnectItem->AppContext);
		}

		ExFreePoolWithTag(pDataNode, 'xxxx');
	default:
		
		break;
	}

	if (FLAG_ON_UINT(iStatus, CHANGE_PACKET_DATA))
	{//修改发送数据的长度
		//*length = pDataNode->ConnectItem->AppContext.receiveBuffer.MaximumLength;
		*length = ConnectItem->AppContext.receiveBuffer.length;
	}

	return iStatus;
}

UINT AppProtocolAnalyze(
						BOOLEAN SendData,
						PUCHAR buffer, 
						PULONG length, 
						PFILE_OBJECT AddressFileObject,
						PFILE_OBJECT ConnectFileObject
						)
{
	UINT iRnt = 0;

	if (SendData)
	{
		iRnt =  AppSend(buffer, 
			length, 
			ConnectFileObject);
	}
	else
	{
		iRnt = AppReceive(
			buffer,
			length,
			AddressFileObject);
	}

	return iRnt;
}

VOID AppClearReceiveBuffer(PFILE_OBJECT AddressFileObject)
{
	PADDRESS_ITEM AddressItem;
	AddressItem = FindAddressItemByFileObj(AddressFileObject);

	netbios_clear_receive_buffer(&AddressItem->ConnectItem->AppContext);
}